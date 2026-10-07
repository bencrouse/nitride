#include "StudyEngine.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
constexpr double sampleRate = 48000.0;

void integer(std::ofstream& file, std::uint32_t value, int bytes)
{
    for (int i = 0; i < bytes; ++i) file.put(static_cast<char>((value >> (8 * i)) & 0xff));
}

void writeWave(const std::filesystem::path& path, const std::vector<float>& audio, double gain)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) throw std::runtime_error("Could not create " + path.string());
    const auto dataSize = static_cast<std::uint32_t>(audio.size() * 6);
    file.write("RIFF", 4); integer(file, dataSize + 36, 4); file.write("WAVEfmt ", 8);
    integer(file, 16, 4); integer(file, 1, 2); integer(file, 2, 2);
    integer(file, 48000, 4); integer(file, 48000 * 6, 4); integer(file, 6, 2); integer(file, 24, 2);
    file.write("data", 4); integer(file, dataSize, 4);
    for (const auto sample : audio)
    {
        const auto value = static_cast<std::int32_t>(std::llround(std::clamp(static_cast<double>(sample) * gain, -1.0, 1.0) * 8388607.0));
        integer(file, static_cast<std::uint32_t>(value), 3);
        integer(file, static_cast<std::uint32_t>(value), 3);
    }
    if (!file) throw std::runtime_error("Failed writing " + path.string());
}

double rms(const std::vector<float>& audio)
{
    double sum = 0.0;
    for (const auto sample : audio) sum += static_cast<double>(sample) * sample;
    return std::sqrt(sum / static_cast<double>(audio.size()));
}

std::vector<float> render(SoundStudies::Pattern pattern, SoundStudies::Treatment mode, float amount,
                          SoundStudies::Source source = SoundStudies::Source::simple,
                          bool adaptive = true, bool sweepEnabled = true)
{
    SoundStudies::Engine engine;
    engine.setTreatment(mode);
    engine.setSource(source);
    engine.setNetworkAdaptive(adaptive);
    engine.setAmountSweepEnabled(sweepEnabled);
    engine.setAmount(amount);
    engine.prepare(sampleRate);
    engine.start(pattern);
    const auto length = static_cast<size_t>(std::llround(SoundStudies::Engine::patternSeconds(pattern) * sampleRate));
    std::vector<float> left(length), right(length);
    engine.render(left.data(), right.data(), static_cast<int>(length));
    if (engine.getNumericalFaults() != 0) throw std::runtime_error("Non-finite DSP state in render");
    return left;
}

int renderDetailedStudy(const std::filesystem::path& directory, int study)
{
    const auto third = study == 3;
    const auto fourth = study == 4;
    const auto fifth = study == 5;
    std::ofstream report(directory / "levels.csv", std::ios::trunc);
    std::ofstream notes(directory / "README.txt", std::ios::trunc);
    if (!report || !notes) throw std::runtime_error("Could not write comparison notes");
    report << "file,raw_rms_dbfs,match_gain_db,final_rms_dbfs,peak_dbfs\n";
    notes << (fifth ? "Nitride sound studies 05 / 48 kHz stereo 24-bit PCM\n\n"
                     "A = FM reference. G = current network. H = extended network range.\n"
                     "H 0-50% covers G 0-100%; H 50-100% adds internal stress.\n"
                     "G 50/100% and H 25/50% are paired equivalence references.\n"
                   : fourth ? "Nitride sound studies 04 / 48 kHz stereo 24-bit PCM\n\n"
                      "A = FM reference. F = note-coupled delay at fixed 50%. G = adaptive oscillator coupling.\n"
                      "G is compared at 30/50/70/100%, with adaptive and fixed connections.\n"
                    : third ? "Nitride sound studies 03 / 48 kHz stereo 24-bit PCM\n\n"
                      "A = shared FM reference. E = original. F = note-coupled phase/delay.\n"
                      "Lead and pad have treatments at 30%, 70%, and 100%.\n"
                   : "Nitride sound studies 02 / 48 kHz stereo 24-bit PCM\n\n"
                     "A = shared FM reference. D = nonlinear interaction. E = audio-rate phase/delay feedback.\n"
                     "Lead and pad have treatments at 25%, 50%, and 100%.\n")
          << "Simple source: FM index 1.65. Rich source: FM index 2.70. Sweep moves 0 -> 100 -> 0.\n"
             "Lead includes repeated notes, different velocities, longer gates, glide and pitch bends.\n"
             "Each source/phrase group has matching broadband RMS; this is not LUFS normalization.\n"
             "If headroom requires it, the entire comparison group is reduced together.\n"
             "No external samples, learned models, reverb or stereo widening.\n";
    struct Variant { const char* name; SoundStudies::Treatment treatment; float amount; bool adaptive = true; };
    const std::vector<Variant> variants = fifth ? std::vector<Variant> {
        { "A_fm", SoundStudies::Treatment::fm, 0.0f },
        { "G_current_50", SoundStudies::Treatment::adaptiveNetwork, 0.5f },
        { "G_current_100", SoundStudies::Treatment::adaptiveNetwork, 1.0f },
        { "H_extended_25", SoundStudies::Treatment::stressedNetwork, 0.25f },
        { "H_extended_50", SoundStudies::Treatment::stressedNetwork, 0.5f },
        { "H_extended_60", SoundStudies::Treatment::stressedNetwork, 0.6f },
        { "H_extended_75", SoundStudies::Treatment::stressedNetwork, 0.75f },
        { "H_extended_90", SoundStudies::Treatment::stressedNetwork, 0.9f },
        { "H_extended_100", SoundStudies::Treatment::stressedNetwork, 1.0f },
        { "H_fixed_60", SoundStudies::Treatment::stressedNetwork, 0.6f, false },
        { "H_fixed_75", SoundStudies::Treatment::stressedNetwork, 0.75f, false },
        { "H_fixed_90", SoundStudies::Treatment::stressedNetwork, 0.9f, false },
        { "H_fixed_100", SoundStudies::Treatment::stressedNetwork, 1.0f, false }
    } : fourth ? std::vector<Variant> {
        { "A_fm", SoundStudies::Treatment::fm, 0.0f },
        { "F_reference_50", SoundStudies::Treatment::noteCoupledDelay, 0.5f },
        { "G_adaptive_30", SoundStudies::Treatment::adaptiveNetwork, 0.3f },
        { "G_adaptive_50", SoundStudies::Treatment::adaptiveNetwork, 0.5f },
        { "G_adaptive_70", SoundStudies::Treatment::adaptiveNetwork, 0.7f },
        { "G_adaptive_100", SoundStudies::Treatment::adaptiveNetwork, 1.0f },
        { "G_fixed_30", SoundStudies::Treatment::adaptiveNetwork, 0.3f, false },
        { "G_fixed_50", SoundStudies::Treatment::adaptiveNetwork, 0.5f, false },
        { "G_fixed_70", SoundStudies::Treatment::adaptiveNetwork, 0.7f, false },
        { "G_fixed_100", SoundStudies::Treatment::adaptiveNetwork, 1.0f, false }
    } : third ? std::vector<Variant> {
        { "A_fm", SoundStudies::Treatment::fm, 0.0f },
        { "E_original_30", SoundStudies::Treatment::phaseDelay, 0.3f },
        { "E_original_70", SoundStudies::Treatment::phaseDelay, 0.7f },
        { "E_original_100", SoundStudies::Treatment::phaseDelay, 1.0f },
        { "F_note_coupled_30", SoundStudies::Treatment::noteCoupledDelay, 0.3f },
        { "F_note_coupled_70", SoundStudies::Treatment::noteCoupledDelay, 0.7f },
        { "F_note_coupled_100", SoundStudies::Treatment::noteCoupledDelay, 1.0f }
    } : std::vector<Variant> {
        { "A_fm", SoundStudies::Treatment::fm, 0.0f },
        { "D_interaction_25", SoundStudies::Treatment::interaction, 0.25f },
        { "D_interaction_50", SoundStudies::Treatment::interaction, 0.5f },
        { "D_interaction_100", SoundStudies::Treatment::interaction, 1.0f },
        { "E_phase_delay_25", SoundStudies::Treatment::phaseDelay, 0.25f },
        { "E_phase_delay_50", SoundStudies::Treatment::phaseDelay, 0.5f },
        { "E_phase_delay_100", SoundStudies::Treatment::phaseDelay, 1.0f }
    };
    const std::array<SoundStudies::Pattern, 3> patterns = fourth || fifth
        ? std::array { SoundStudies::Pattern::lead, SoundStudies::Pattern::pad, SoundStudies::Pattern::velocity }
        : std::array { SoundStudies::Pattern::lead, SoundStudies::Pattern::pad, SoundStudies::Pattern::sweep };
    int count = 0;
    for (const auto source : { SoundStudies::Source::simple, SoundStudies::Source::rich })
    for (const auto pattern : patterns)
    {
        struct Comparison { std::string filename; std::vector<float> audio; double rms, gain, peak; };
        std::vector<Comparison> comparisons;
        const auto reference = render(pattern, SoundStudies::Treatment::fm, 0, source);
        const auto referenceRms = rms(reference);
        const auto phrase = pattern == SoundStudies::Pattern::lead ? "lead" : pattern == SoundStudies::Pattern::pad ? "pad"
            : pattern == SoundStudies::Pattern::velocity ? "velocity" : "sweep";
        const auto prefix = std::string(source == SoundStudies::Source::rich ? "rich_" : "simple_") + phrase + "_";
        double maximumPeak = 0.0;
        for (const auto& variant : variants)
        {
            if (pattern == SoundStudies::Pattern::sweep && variant.amount > 0.01f && variant.amount < 0.99f) continue;
            auto audio = variant.treatment == SoundStudies::Treatment::fm ? reference
                : render(pattern, variant.treatment, variant.amount, source, variant.adaptive,
                         !(fourth && variant.treatment == SoundStudies::Treatment::noteCoupledDelay));
            const auto rawRms = rms(audio);
            if (rawRms < 0.0000001) throw std::runtime_error("Comparison unexpectedly silent");
            const auto gain = referenceRms / rawRms;
            double peak = 0;
            for (const auto sample : audio) peak = std::max(peak, std::abs(static_cast<double>(sample)) * gain);
            maximumPeak = std::max(maximumPeak, peak);
            const auto name = pattern != SoundStudies::Pattern::sweep ? variant.name
                : variant.treatment == SoundStudies::Treatment::fm ? "A_fm"
                : third ? variant.treatment == SoundStudies::Treatment::phaseDelay ? "E_original" : "F_note_coupled"
                : variant.treatment == SoundStudies::Treatment::interaction ? "D_interaction" : "E_phase_delay";
            comparisons.push_back({ prefix + name + ".wav", std::move(audio), rawRms, gain, peak });
        }
        const auto headroom = std::min(1.0, 0.88 / maximumPeak);
        for (const auto& comparison : comparisons)
        {
            const auto gain = comparison.gain * headroom;
            writeWave(directory / comparison.filename, comparison.audio, gain);
            report << comparison.filename << ',' << 20 * std::log10(comparison.rms) << ',' << 20 * std::log10(gain)
                   << ',' << 20 * std::log10(comparison.rms * gain) << ',' << 20 * std::log10(comparison.peak * headroom) << '\n';
            std::cout << comparison.filename << "  match " << std::fixed << std::setprecision(2) << 20 * std::log10(gain) << " dB\n";
            ++count;
        }
    }
    std::cout << "Wrote " << count << " Study " << study << " comparisons to " << std::filesystem::absolute(directory) << '\n';
    return 0;
}
}

int main(int argc, char** argv)
{
    try
    {
        const std::filesystem::path directory = argc > 1 ? argv[1] : "build/sound-studies";
        std::filesystem::create_directories(directory);
        if (argc > 2 && std::string(argv[2]) == "--study-02") return renderDetailedStudy(directory, 2);
        if (argc > 2 && std::string(argv[2]) == "--study-03") return renderDetailedStudy(directory, 3);
        if (argc > 2 && std::string(argv[2]) == "--study-04") return renderDetailedStudy(directory, 4);
        if (argc > 2 && std::string(argv[2]) == "--study-05") return renderDetailedStudy(directory, 5);
        std::ofstream report(directory / "levels.csv", std::ios::trunc);
        std::ofstream notes(directory / "README.txt", std::ios::trunc);
        if (!report || !notes) throw std::runtime_error("Could not write comparison notes");
        report << "file,raw_rms_dbfs,match_gain_db,final_rms_dbfs,peak_dbfs\n";
        notes << "Nitride sound studies / 48 kHz stereo 24-bit PCM\n\n"
                 "A = plain FM reference. B = nonlinear body coupling. C = partial spacing deformation.\n"
                 "The fixed studies use 50% and 100% amounts; sweep studies move 0 -> 100 -> 0.\n"
                 "Each file is one complete phrase. All variants of a phrase have matching broadband RMS.\n"
                 "RMS matching is approximate perceived loudness matching, not LUFS normalization.\n"
                 "No reverb, stereo widening, learned models, or external samples.\n"
                 "Body is a simplified experimental model, not a calibrated physical simulation.\n";
        const std::array<const char*, 4> names { "pad", "hits", "velocity", "sweep" };
        int count = 0;
        for (int index = 0; index < 4; ++index)
        {
            const auto pattern = static_cast<SoundStudies::Pattern>(index);
            const auto reference = render(pattern, SoundStudies::Treatment::fm, 0.0f);
            const auto referenceRms = rms(reference);
            struct Variant { const char* name; SoundStudies::Treatment treatment; float amount; };
            const std::array<Variant, 5> variants {{
                { "A_fm", SoundStudies::Treatment::fm, 0.0f },
                { "B_body_50", SoundStudies::Treatment::body, 0.5f },
                { "B_body_100", SoundStudies::Treatment::body, 1.0f },
                { "C_spectral_50", SoundStudies::Treatment::spectral, 0.5f },
                { "C_spectral_100", SoundStudies::Treatment::spectral, 1.0f }
            }};
            for (const auto& variant : variants)
            {
                if (pattern == SoundStudies::Pattern::sweep && variant.amount == 1.0f) continue;
                const auto audio = variant.treatment == SoundStudies::Treatment::fm
                    ? reference : render(pattern, variant.treatment, variant.amount);
                const auto rawRms = rms(audio);
                const auto gain = referenceRms / std::max(0.0000001, rawRms);
                double peak = 0.0;
                for (const auto sample : audio) peak = std::max(peak, std::abs(static_cast<double>(sample)) * gain);
                if (peak >= 0.95) throw std::runtime_error("RMS matching would exceed comparison headroom");
                auto filename = std::string(names[static_cast<size_t>(index)]) + "_" + variant.name;
                if (pattern == SoundStudies::Pattern::sweep && variant.treatment != SoundStudies::Treatment::fm)
                    filename = std::string(names[static_cast<size_t>(index)]) + (variant.treatment == SoundStudies::Treatment::body ? "_B_body" : "_C_spectral");
                filename += ".wav";
                writeWave(directory / filename, audio, gain);
                report << filename << ',' << 20 * std::log10(rawRms) << ',' << 20 * std::log10(gain)
                       << ',' << 20 * std::log10(rawRms * gain) << ',' << 20 * std::log10(peak) << '\n';
                std::cout << filename << "  match " << std::fixed << std::setprecision(2) << 20 * std::log10(gain) << " dB\n";
                ++count;
            }
        }
        std::cout << "Wrote " << count << " comparisons to " << std::filesystem::absolute(directory) << '\n';
        return 0;
    }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
