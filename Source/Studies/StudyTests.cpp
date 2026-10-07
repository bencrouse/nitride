#include "StudyEngine.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

std::vector<float> note(SoundStudies::Treatment treatment, float amount, double rate, int blockSize,
                        SoundStudies::Source source = SoundStudies::Source::simple, bool adaptive = true)
{
    SoundStudies::Engine engine;
    engine.setTreatment(treatment);
    engine.setSource(source);
    engine.setNetworkAdaptive(adaptive);
    engine.setAmount(amount);
    engine.prepare(rate);
    engine.noteOn(60, 0.8f, SoundStudies::Articulation::hit);
    std::vector<float> left(static_cast<size_t>(rate * 0.75)), right(left.size());
    for (size_t offset = 0; offset < left.size(); offset += static_cast<size_t>(blockSize))
    {
        const auto size = static_cast<int>(std::min(static_cast<size_t>(blockSize), left.size() - offset));
        engine.render(left.data() + offset, right.data() + offset, size);
    }
    require(engine.getNumericalFaults() == 0, "Numerical failure in note render");
    return left;
}

std::vector<float> lead(SoundStudies::Treatment treatment, int blockSize)
{
    SoundStudies::Engine engine;
    engine.setTreatment(treatment);
    engine.setSource(SoundStudies::Source::rich);
    engine.setAmount(1.0f);
    engine.prepare(48000);
    engine.start(SoundStudies::Pattern::lead);
    std::vector<float> audio(384000), right(audio.size());
    for (size_t offset = 0; offset < audio.size(); offset += static_cast<size_t>(blockSize))
    {
        const auto length = static_cast<int>(std::min(static_cast<size_t>(blockSize), audio.size() - offset));
        engine.render(audio.data() + offset, right.data() + offset, length);
    }
    require(engine.getNumericalFaults() == 0, "Lead phrase caused numerical failure");
    return audio;
}

std::vector<float> dimensionNote(float coupling, float stress, double response, double rate = 48000, int blockSize = 257)
{
    SoundStudies::Engine engine;
    engine.setTreatment(SoundStudies::Treatment::expressiveNetwork);
    engine.setSource(SoundStudies::Source::rich);
    engine.setNetworkDimensions(coupling, stress, response);
    engine.prepare(rate);
    engine.noteOn(60, 0.8f, SoundStudies::Articulation::hit);
    std::vector<float> audio(static_cast<size_t>(rate * 0.75)), right(audio.size());
    for (size_t offset = 0; offset < audio.size(); offset += static_cast<size_t>(blockSize))
    {
        const auto size = static_cast<int>(std::min(static_cast<size_t>(blockSize), audio.size() - offset));
        engine.render(audio.data() + offset, right.data() + offset, size);
    }
    require(engine.getNumericalFaults() == 0, "Independent dimensions caused numerical failure");
    const auto snapshot = engine.getNetworkSnapshot();
    require(snapshot.coherence >= 0 && snapshot.coherence <= 1.00001f, "Invalid network-state feedback");
    for (const auto link : snapshot.links) require(std::isfinite(link) && std::abs(link) <= 1.00001f, "Unbounded link state");
    return audio;
}

int period(const std::vector<float>& audio)
{
    int best = 0;
    double minimum = 1.0e9;
    for (int lag = 120; lag < 240; ++lag)
    {
        double difference = 0.0;
        for (size_t i = audio.size() - 4096; i < audio.size() - 240; ++i)
        {
            const auto delta = static_cast<double>(audio[i]) - audio[i + static_cast<size_t>(lag)];
            difference += delta * delta;
        }
        if (difference < minimum) { minimum = difference; best = lag; }
    }
    return best;
}
}

int main()
{
    try
    {
        // Analytic sidebands must reconstruct the same two-operator FM source.
        for (const auto source : { SoundStudies::Source::simple, SoundStudies::Source::rich })
        for (int i = 0; i < 1000; ++i)
        {
            const auto index = SoundStudies::Engine::sourceIndex(source);
            const auto phase = static_cast<double>(i) / 1000.0 * std::numbers::pi * 2.0;
            double reconstructed = 0.0;
            for (int n = 0; n < 12; ++n)
                reconstructed += (SoundStudies::Engine::bessel(n, index)
                    + (n % 2 == 0 ? 1.0 : -1.0) * SoundStudies::Engine::bessel(n + 1, index))
                    * std::sin(phase * static_cast<double>(2 * n + 1));
            require(std::abs(reconstructed - std::sin(phase + index * std::sin(phase * 2.0))) < 0.000001,
                    "Spectral source does not reconstruct the reference FM voice");
        }
        for (const auto source : { SoundStudies::Source::simple, SoundStudies::Source::rich })
        for (const auto rate : { 44100.0, 48000.0, 96000.0 })
        {
            const auto reference = note(SoundStudies::Treatment::fm, 0.0f, rate, 257, source);
            for (const auto mode : { SoundStudies::Treatment::body, SoundStudies::Treatment::spectral,
                                     SoundStudies::Treatment::interaction, SoundStudies::Treatment::phaseDelay,
                                    SoundStudies::Treatment::noteCoupledDelay, SoundStudies::Treatment::adaptiveNetwork,
                                    SoundStudies::Treatment::stressedNetwork })
            {
                const auto zero = note(mode, 0.0f, rate, 257, source);
                double difference = 0.0;
                for (size_t i = 0; i < zero.size(); ++i) difference = std::max(difference, std::abs(static_cast<double>(zero[i] - reference[i])));
                require(difference < 0.000001, "Zero treatment changed the source");
                const auto full = note(mode, 1.0f, rate, 1, source);
                const auto blocked = note(mode, 1.0f, rate, 511, source);
                double changeEnergy = 0.0;
                for (size_t i = 0; i < full.size(); ++i)
                {
                    require(std::isfinite(full[i]) && std::abs(full[i]) < 0.8f, "Non-finite or unbounded audio");
                    require(std::abs(full[i] - blocked[i]) < 0.000001f, "DSP depends on audio block size");
                    changeEnergy += static_cast<double>(full[i] - reference[i]) * (full[i] - reference[i]);
                }
                require(changeEnergy > 0.05, "Treatment did not meaningfully change the rendered signal");
            }
        }
        for (const auto treatment : { SoundStudies::Treatment::body, SoundStudies::Treatment::spectral,
                                     SoundStudies::Treatment::interaction, SoundStudies::Treatment::phaseDelay,
                                    SoundStudies::Treatment::noteCoupledDelay, SoundStudies::Treatment::adaptiveNetwork,
                                    SoundStudies::Treatment::stressedNetwork })
        {
            SoundStudies::Engine reference, excursion;
            excursion.setTreatment(treatment);
            excursion.setAmount(1.0f);
            reference.prepare(48000);
            excursion.prepare(48000);
            reference.noteOn(60, 0.7f);
            excursion.noteOn(60, 0.7f);
            std::vector<float> baseline(48000), altered(48000), right(48000);
            reference.render(baseline.data(), right.data(), 48000);
            excursion.render(altered.data(), right.data(), 48000);
            excursion.setAmount(0.0f);
            reference.render(baseline.data(), right.data(), 48000);
            excursion.render(altered.data(), right.data(), 48000);
            double neutralDifference = 0.0;
            for (size_t i = baseline.size() - 1024; i < baseline.size(); ++i)
                neutralDifference = std::max(neutralDifference, std::abs(static_cast<double>(baseline[i] - altered[i])));
            require(neutralDifference < 0.000001, "Amount did not return to the reference after deformation");
        }
        for (const auto treatment : { SoundStudies::Treatment::interaction, SoundStudies::Treatment::phaseDelay,
                                    SoundStudies::Treatment::noteCoupledDelay, SoundStudies::Treatment::adaptiveNetwork,
                                    SoundStudies::Treatment::stressedNetwork })
        {
            SoundStudies::Engine stressed;
            stressed.setTreatment(treatment);
            stressed.setSource(SoundStudies::Source::rich);
            stressed.setAmount(1.0f);
            stressed.prepare(48000);
            for (const auto pitch : { 24, 36, 48, 60, 72, 84, 96, 108, 112, 116, 120, 124 }) stressed.noteOn(pitch, 1.0f);
            std::vector<float> left(144000), right(left.size());
            stressed.render(left.data(), right.data(), static_cast<int>(left.size()));
            stressed.setTreatment(SoundStudies::Treatment::spectral);
            stressed.setAmount(0.0f);
            stressed.stop();
            stressed.render(left.data(), right.data(), static_cast<int>(left.size()));
            require(stressed.getNumericalFaults() == 0, "High coupling/polyphony caused numerical failure");
            double tail = 0.0;
            for (size_t i = left.size() - 1024; i < left.size(); ++i) tail = std::max(tail, std::abs(static_cast<double>(left[i])));
            require(tail < 0.000001, "Stopping left a stuck note or feedback tail");

            const auto single = lead(treatment, 257);
            const auto blocked = lead(treatment, 1024);
            for (size_t i = 0; i < single.size(); ++i)
                require(std::abs(single[i] - blocked[i]) < 0.000001f, "Lead gates/bends depend on block size");
            for (size_t i = single.size() - 4096; i < single.size(); ++i)
                require(std::abs(single[i]) < 0.000001f, "Lead phrase left a stuck gate");
        }
        const auto adaptive = note(SoundStudies::Treatment::adaptiveNetwork, 0.5f, 48000, 257, SoundStudies::Source::rich);
        const auto fixedLinks = note(SoundStudies::Treatment::adaptiveNetwork, 0.5f, 48000, 257, SoundStudies::Source::rich, false);
        double adaptationDifference = 0.0;
        for (size_t i = 0; i < adaptive.size(); ++i)
            adaptationDifference += static_cast<double>(adaptive[i] - fixedLinks[i]) * (adaptive[i] - fixedLinks[i]);
        require(adaptationDifference > 0.01, "Adaptive links did not affect the generated voice");

        for (const auto source : { SoundStudies::Source::simple, SoundStudies::Source::rich })
        for (const auto amount : { 0.3f, 0.5f, 0.7f, 1.0f })
        {
            const auto original = note(SoundStudies::Treatment::adaptiveNetwork, amount, 48000, 257, source);
            const auto extended = note(SoundStudies::Treatment::stressedNetwork, amount * 0.5f, 48000, 257, source);
            for (size_t i = 0; i < original.size(); ++i)
                require(std::abs(original[i] - extended[i]) < 0.000001f, "H's lower half changed G's existing behavior");
        }
        const auto boundary = note(SoundStudies::Treatment::stressedNetwork, 0.5f, 48000, 257, SoundStudies::Source::rich);
        const auto overdriven = note(SoundStudies::Treatment::stressedNetwork, 1.0f, 48000, 257, SoundStudies::Source::rich);
        double stressDifference = 0.0;
        for (size_t i = 0; i < boundary.size(); ++i)
            stressDifference += static_cast<double>(boundary[i] - overdriven[i]) * (boundary[i] - overdriven[i]);
        require(stressDifference > 0.05, "Internal stress did not change the generated voice");

        const auto separateG = dimensionNote(0.5f, 0, 0.025);
        const auto originalG = note(SoundStudies::Treatment::adaptiveNetwork, 0.5f, 48000, 257, SoundStudies::Source::rich);
        const auto separateH = dimensionNote(1, 0.5f, 0.025);
        const auto originalH = note(SoundStudies::Treatment::stressedNetwork, 0.75f, 48000, 257, SoundStudies::Source::rich);
        for (size_t i = 0; i < separateG.size(); ++i)
        {
            require(std::abs(separateG[i] - originalG[i]) < 0.000001f, "Dimension mapping changed the G benchmark");
            require(std::abs(separateH[i] - originalH[i]) < 0.000001f, "Dimension mapping changed the H benchmark");
        }
        const auto neutralDimensions = dimensionNote(0, 0, 0.025);
        const auto plainDimensions = note(SoundStudies::Treatment::fm, 0, 48000, 257, SoundStudies::Source::rich);
        const auto moreCoupling = dimensionNote(0.8f, 0, 0.025);
        const auto moreStress = dimensionNote(0.5f, 0.6f, 0.025);
        double couplingDifference = 0, separateStressDifference = 0;
        for (size_t i = 0; i < neutralDimensions.size(); ++i)
        {
            require(std::abs(neutralDimensions[i] - plainDimensions[i]) < 0.000001f, "Neutral field position changed the FM reference");
            couplingDifference += static_cast<double>(moreCoupling[i] - separateG[i]) * (moreCoupling[i] - separateG[i]);
            separateStressDifference += static_cast<double>(moreStress[i] - separateG[i]) * (moreStress[i] - separateG[i]);
        }
        require(couplingDifference > 0.05 && separateStressDifference > 0.05, "Independent axes did not affect the generated voice");
        for (const auto rate : { 44100.0, 48000.0, 96000.0 })
        {
            const auto single = dimensionNote(1, 1, 0.01, rate, 1);
            const auto blocked = dimensionNote(1, 1, 0.01, rate, 511);
            for (size_t i = 0; i < single.size(); ++i)
                require(std::isfinite(single[i]) && std::abs(single[i] - blocked[i]) < 0.000001f,
                        "Independent dimensions depend on block size");
        }
        const auto quick = dimensionNote(0.75f, 0.3f, 0.025);
        const auto slow = dimensionNote(0.75f, 0.3f, 0.6);
        double responseDifference = 0;
        for (size_t i = 0; i < quick.size(); ++i) responseDifference += static_cast<double>(quick[i] - slow[i]) * (quick[i] - slow[i]);
        require(responseDifference > 0.05, "Response changed visuals but not the generated sound");
        {
            SoundStudies::Engine shaped;
            shaped.setTreatment(SoundStudies::Treatment::expressiveNetwork);
            shaped.setNetworkDimensions(0,0,.025);
            shaped.setFmIndex(0);
            shaped.setAmplitudeEnvelope(.02,.1,.8f,.3);
            shaped.prepare(48000);
            shaped.noteOn(60,.8f);
            std::vector<float> plain(4800), rich(plain.size()), right(plain.size());
            shaped.render(plain.data(),right.data(),4800);
            const auto heldLevel=shaped.getNetworkSnapshot().level;
            shaped.setFmIndex(3.5);
            shaped.render(rich.data(),right.data(),4800);
            double sourceChange=0;
            for(size_t i=0;i<plain.size();++i)sourceChange+=static_cast<double>(plain[i]-rich[i])*(plain[i]-rich[i]);
            require(sourceChange>.01&&heldLevel>.2f,"Source/envelope review controls did not affect the voice");
            shaped.noteOff(60);
            std::vector<float> tail(9600), tailRight(tail.size());
            shaped.render(tail.data(),tailRight.data(),4800);
            require(shaped.getNetworkSnapshot().activeVoices==1,"Custom release ended prematurely");
            shaped.render(tail.data(),tailRight.data(),9600);
            require(shaped.getNetworkSnapshot().activeVoices==0&&shaped.getNumericalFaults()==0,"Custom release did not finish safely");
        }
        {
            SoundStudies::Engine extremeDimensions;
            extremeDimensions.setTreatment(SoundStudies::Treatment::expressiveNetwork);
            extremeDimensions.setNetworkDimensions(1, 1, 0.01);
            extremeDimensions.prepare(48000);
            for (const auto pitch : {24, 36, 48, 60, 72, 84, 96, 108, 112, 116, 120, 124}) extremeDimensions.noteOn(pitch, 1);
            std::vector<float> left(144000), right(left.size());
            extremeDimensions.render(left.data(), right.data(), static_cast<int>(left.size()));
            extremeDimensions.allNotesOff();
            extremeDimensions.render(left.data(), right.data(), static_cast<int>(left.size()));
            require(extremeDimensions.getNumericalFaults() == 0 && extremeDimensions.getNetworkSnapshot().activeVoices == 0,
                    "Fast-response polyphony failed to remain finite or release");
        }

        SoundStudies::Engine benchmark;
        benchmark.setTreatment(SoundStudies::Treatment::noteCoupledDelay);
        benchmark.setAmount(0.5f);
        benchmark.setAmountSweepEnabled(false);
        benchmark.prepare(48000);
        benchmark.start(SoundStudies::Pattern::sweep);
        std::vector<float> held(288000), heldRight(held.size());
        benchmark.render(held.data(), heldRight.data(), static_cast<int>(held.size()));
        require(std::abs(benchmark.getAmount() - 0.5f) < 0.000001f, "Fixed F benchmark followed the G amount sweep");
        SoundStudies::Engine pitchTest;
        pitchTest.prepare(48000);
        pitchTest.noteOn(60, 0.7f);
        std::vector<float> original(96000), bent(original.size()), right(original.size());
        pitchTest.render(original.data(), right.data(), static_cast<int>(original.size()));
        pitchTest.setPitchBend(2.0f);
        pitchTest.render(bent.data(), right.data(), static_cast<int>(bent.size()));
        const auto ratio = static_cast<double>(period(original)) / static_cast<double>(period(bent));
        require(std::abs(ratio - std::exp2(2.0 / 12.0)) < 0.01, "Pitch bend failed to move the audible fundamental");
        {
            const auto lowRate = note(SoundStudies::Treatment::fm,0,11025,257);
            int bestLag = 0; double minimumDifference = 1.0e9;
            for(int lag=30;lag<60;++lag)
            {
                double difference=0;
                for(size_t i=lowRate.size()-2048;i<lowRate.size()-60;++i)
                {const auto delta=static_cast<double>(lowRate[i])-lowRate[i+static_cast<size_t>(lag)];difference+=delta*delta;}
                if(difference<minimumDifference){minimumDifference=difference;bestLag=lag;}
            }
            require(bestLag>0&&std::abs(11025.0/bestLag-261.625565)<4.0,"Low host sample rate changed MIDI pitch");
        }
        std::cout << "Sound studies passed: both FM sources, neutral return, block invariance, polyphony, lead gates and pitch bends.\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
