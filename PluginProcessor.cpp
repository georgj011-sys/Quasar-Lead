#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params.h"
#include "Presets.h"

QuasarProcessor::QuasarProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", Params::createLayout())
{
    noteStack.ensureStorageAllocated (128);
}

void QuasarProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto& voice : voices)
        voice.prepare (sampleRate);

    fx.prepare (sampleRate, samplesPerBlock);
    noteStack.clearQuick();
    lastNote = -1;
}

bool QuasarProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void QuasarProcessor::readParams (LeadParams& lp, FxParams& fp) const
{
    for (int i = 0; i < 2; ++i)
    {
        auto& o = lp.osc[i];
        o.type   = vi (Params::osc (i, "type"));
        o.shape  = v (Params::osc (i, "shape"));
        o.color  = v (Params::osc (i, "color"));
        o.octave = v (Params::osc (i, "octave"));
        o.semi   = v (Params::osc (i, "semi"));
        o.fine   = v (Params::osc (i, "fine"));
        o.level  = v (Params::osc (i, "level"));
        o.pan    = v (Params::osc (i, "pan"));
    }

    lp.subType    = vi ("sub_type");
    lp.subLevel   = v ("sub_level");
    lp.ring       = v ("ring");
    lp.filterType = vi ("filter_type");
    lp.cutoff     = v ("cutoff");
    lp.reso       = v ("reso");
    lp.envAmt     = v ("fenv_amt");
    lp.keytrack   = v ("keytrack");
    lp.drive      = v ("filter_drive");
    lp.ampEnv     = { v ("amp_attack"), v ("amp_decay"), v ("amp_sustain"), v ("amp_release") };
    lp.fltEnv     = { v ("flt_attack"), v ("flt_decay"), v ("flt_sustain"), v ("flt_release") };
    lp.lfoWave    = vi ("lfo_wave");
    lp.lfoRate    = v ("lfo_rate");
    lp.lfoPitch   = v ("lfo_pitch");
    lp.lfoCutoff  = v ("lfo_cutoff");
    lp.lfoOsc     = v ("lfo_osc");
    lp.penvAmt    = v ("penv_amt");
    lp.penvDecay  = v ("penv_decay");
    lp.glide      = v ("glide");

    fp.distType    = vi ("dist_type");
    fp.distAmt     = v ("dist_amt");
    fp.phaserRate  = v ("phaser_rate");
    fp.phaserDepth = v ("phaser_depth");
    fp.phaserMix   = v ("phaser_mix");
    fp.chorusRate  = v ("chorus_rate");
    fp.chorusDepth = v ("chorus_depth");
    fp.chorusMix   = v ("chorus_mix");
    fp.delayTime   = v ("delay_time");
    fp.delayFb     = v ("delay_fb");
    fp.delayTone   = v ("delay_tone");
    fp.delayMix    = v ("delay_mix");
    fp.reverbSize  = v ("reverb_size");
    fp.reverbMix   = v ("reverb_mix");
    fp.panRate     = v ("pan_rate");
    fp.panDepth    = v ("pan_depth");
    fp.haasMs      = v ("haas");
    fp.width       = v ("width");
    fp.master      = v ("master");
}

void QuasarProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    LeadParams lp;
    FxParams fp;
    readParams (lp, fp);

    mode = juce::jlimit (0, 2, vi ("mode"));
    if (mode != lastMode)
    {
        if (lastMode != -1)
        {
            releaseAll();
            for (size_t i = 1; i < voices.size(); ++i)
                voices[i].kill();
        }
        lastMode = mode;
    }

    if (buffer.getNumChannels() < 2)
        return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    int pos = 0;
    for (const auto metadata : midi)
    {
        const int t = juce::jlimit (0, numSamples, metadata.samplePosition);
        renderVoices (L, R, pos, t - pos, lp);
        pos = t;
        handleMidi (metadata.getMessage(), lp);
    }
    renderVoices (L, R, pos, numSamples - pos, lp);

    fx.process (buffer, fp);
}

void QuasarProcessor::renderVoices (float* L, float* R, int start, int num, const LeadParams& lp)
{
    if (num <= 0)
        return;

    const float bendSemis = bend * (float) vi ("bend");
    const int count = mode == Poly ? (int) voices.size() : 1;

    for (int i = 0; i < count; ++i)
        voices[(size_t) i].render (L, R, start, num, lp, bendSemis, modWheel);
}

void QuasarProcessor::handleMidi (const juce::MidiMessage& msg, const LeadParams& lp)
{
    if (msg.isNoteOn())
        noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), lp);
    else if (msg.isNoteOff())
        noteOff (msg.getNoteNumber());
    else if (msg.isPitchWheel())
        bend = ((float) msg.getPitchWheelValue() - 8192.0f) / 8192.0f;
    else if (msg.isController() && msg.getControllerNumber() == 1)
        modWheel = (float) msg.getControllerValue() / 127.0f;
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        releaseAll();
}

void QuasarProcessor::noteOn (int note, float velocity, const LeadParams& lp)
{
    noteStack.removeAllInstancesOf (note);
    noteStack.add (note);

    if (mode == Poly)
    {
        LeadVoice* target = nullptr;

        for (auto& voice : voices)
            if (voice.isActive() && voice.getNote() == note)
                target = &voice;

        if (target == nullptr)
            for (auto& voice : voices)
                if (! voice.isActive()) { target = &voice; break; }

        if (target == nullptr)
        {
            // Steal: prefer released voices, then the oldest
            for (auto& voice : voices)
            {
                if (target == nullptr) { target = &voice; continue; }
                const bool better = (! voice.isGateOn() && target->isGateOn())
                                 || (voice.isGateOn() == target->isGateOn() && voice.age < target->age);
                if (better)
                    target = &voice;
            }
        }

        const float from = (lp.glide > 0.0005f && lastNote >= 0) ? (float) lastNote : (float) note;
        target->start (note, velocity, from, lp, ++seed);
        target->age = ++ageCounter;
    }
    else
    {
        auto& voice = voices[0];

        if (mode == Legato)
        {
            if (noteStack.size() > 1 && voice.isActive() && voice.isGateOn())
                voice.glideTo (note);
            else
                voice.start (note, velocity, (float) note, lp, ++seed);
        }
        else
        {
            const float from = voice.isActive() ? voice.getPitch()
                                                : (lastNote >= 0 ? (float) lastNote : (float) note);
            voice.start (note, velocity, from, lp, ++seed);
        }
    }

    lastNote = note;
}

void QuasarProcessor::noteOff (int note)
{
    const bool wasTop = ! noteStack.isEmpty() && noteStack.getLast() == note;
    noteStack.removeAllInstancesOf (note);

    if (mode == Poly)
    {
        for (auto& voice : voices)
            if (voice.isGateOn() && voice.getNote() == note)
                voice.release();
        return;
    }

    auto& voice = voices[0];

    if (noteStack.isEmpty())
    {
        if (voice.isGateOn())
            voice.release();
    }
    else if (wasTop && voice.isGateOn())
    {
        const int previous = noteStack.getLast();
        voice.glideTo (previous);
        lastNote = previous;
    }
}

void QuasarProcessor::releaseAll()
{
    for (auto& voice : voices)
        if (voice.isGateOn())
            voice.release();

    noteStack.clearQuick();
}

juce::AudioProcessorEditor* QuasarProcessor::createEditor()
{
    return new QuasarEditor (*this);
}

int QuasarProcessor::getNumPrograms()
{
    return (int) getFactoryPresets().size();
}

const juce::String QuasarProcessor::getProgramName (int index)
{
    const auto& presets = getFactoryPresets();
    if (index >= 0 && index < (int) presets.size())
        return presets[(size_t) index].name;
    return {};
}

void QuasarProcessor::setCurrentProgram (int index)
{
    const auto& presets = getFactoryPresets();
    if (index < 0 || index >= (int) presets.size())
        return;

    currentProgram = index;
    const auto& preset = presets[(size_t) index];

    for (auto* rawParam : getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (rawParam))
        {
            float normalised = p->getDefaultValue();

            for (const auto& kv : preset.values)
            {
                if (kv.first == p->paramID)
                {
                    normalised = p->convertTo0to1 (kv.second);
                    break;
                }
            }

            p->setValueNotifyingHost (normalised);
        }
    }
}

void QuasarProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void QuasarProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        currentProgram = (int) tree.getProperty ("program", 0);
        apvts.replaceState (tree);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QuasarProcessor();
}
