#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace SMEmu {

const int kChordNumNotes = 4;
const int kChordNumVoices = kChordNumNotes + 1;

const int kChordNumOriginalChords = 11;
const int kChordNumJonChords = 17;
const int kChordNumJoeChords = 18;
const int kChordNumChords = kChordNumJonChords + kChordNumJoeChords;
const int kChordNumHarmonics = 3;

// [saw_8', sq_8', saw_4', sq_4', saw_2', sq_2']
const int kRegistrationTableSize = 11;
const float kRegistrations[kRegistrationTableSize][kChordNumHarmonics * 2] = {
    { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },    // Saw
    { 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f },    // Saw + saw
    { 0.4f, 0.0f, 0.2f, 0.0f, 0.4f, 0.0f },    // Full saw
    { 0.3f, 0.0f, 0.0f, 0.3f, 0.0f, 0.4f },    // Full saw + square hybrid
    { 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f },    // Saw + high square harmo
    { 0.2f, 0.0f, 0.0f, 0.2f, 0.0f, 0.6f },    // Weird hybrid
    { 0.0f, 0.2f, 0.1f, 0.0f, 0.2f, 0.5f },    // Sawsquare high harmo
    { 0.0f, 0.3f, 0.0f, 0.3f, 0.0f, 0.4f },    // Square high harmo
    { 0.0f, 0.4f, 0.0f, 0.3f, 0.0f, 0.3f },    // Full square
    { 0.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f },    // Square + Square
    { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },    // Square
};

inline float ThisBlepSample(float t) { return 0.5f * t * t; }
inline float NextBlepSample(float t) { t = 1.0f - t; return -0.5f * t * t; }

const int originalChordMapping[kChordNumOriginalChords] = {
    0,   // OCT
    1,   // 5
    9,   // sus4
    2,   // m
    3,   // m7
    4,   // m9
    5,   // m11
    10,  // 69
    8,   // M9
    7,   // M7
    6,   // M
};

inline float SemitonesToRatio(float semitones) {
    return std::pow(2.0f, semitones / 12.0f);
}

inline float Interpolate(const float* table, float index, int size) {
    int i = static_cast<int>(index);
    float f = index - static_cast<float>(i);
    i = std::max(0, std::min(i, size - 2));
    return table[i] + (table[i + 1] - table[i]) * f;
}

class HysteresisQuantizer {
public:
    void Init(int num_steps, float hysteresis, bool symmetric = false) {
        num_steps_ = num_steps;
        hysteresis_ = hysteresis;
        scale_ = static_cast<float>(symmetric ? num_steps - 1 : num_steps);
        offset_ = symmetric ? 0.0f : -0.5f;
        quantized_value_ = 0;
    }

    int Process(float value) {
        value *= scale_;
        value += offset_;

        float hysteresis_sign = value > static_cast<float>(quantized_value_) ? -1.0f : 1.0f;
        int q = static_cast<int>(value + hysteresis_sign * hysteresis_ + 0.5f);
        if (q < 0) q = 0;
        if (q > num_steps_ - 1) q = num_steps_ - 1;
        quantized_value_ = q;
        return q;
    }

    int quantized_value() const { return quantized_value_; }

private:
    int num_steps_;
    float hysteresis_;
    float scale_;
    float offset_;
    int quantized_value_;
};

class ChordBank {
public:
    void Init() {
        // Jon Butler chords (indices 0-16)
        // Fixed Intervals
        SetChord(0,  0.00f, 0.01f, 11.99f, 12.00f);   // Octave
        SetChord(1,  0.00f, 7.00f,  7.01f, 12.00f);   // Fifth
        // Minor
        SetChord(2,  0.00f, 3.00f,  7.00f, 12.00f);   // Minor
        SetChord(3,  0.00f, 3.00f,  7.00f, 10.00f);   // Minor 7th
        SetChord(4,  0.00f, 3.00f, 10.00f, 14.00f);   // Minor 9th
        SetChord(5,  0.00f, 3.00f, 10.00f, 17.00f);   // Minor 11th
        // Major
        SetChord(6,  0.00f, 4.00f,  7.00f, 12.00f);   // Major
        SetChord(7,  0.00f, 4.00f,  7.00f, 11.00f);   // Major 7th
        SetChord(8,  0.00f, 4.00f, 11.00f, 14.00f);   // Major 9th
        // Colour Chords
        SetChord(9,  0.00f, 5.00f,  7.00f, 12.00f);   // Sus4
        SetChord(10, 0.00f, 2.00f,  9.00f, 16.00f);   // 69
        SetChord(11, 0.00f, 4.00f,  7.00f,  9.00f);   // 6th
        SetChord(12, 0.00f, 7.00f, 16.00f, 23.00f);   // 10th (Spread maj7)
        SetChord(13, 0.00f, 4.00f,  7.00f, 10.00f);   // Dominant 7th
        SetChord(14, 0.00f, 7.00f, 10.00f, 13.00f);   // Dominant 7th (b9)
        SetChord(15, 0.00f, 3.00f,  6.00f, 10.00f);   // Half Diminished
        SetChord(16, 0.00f, 3.00f,  6.00f,  9.00f);   // Fully Diminished

        // Joe McMullen chords (indices 17-34)
        SetChord(17,  5.00f, 12.00f, 19.00f, 26.00f); // iv 6/9
        SetChord(18, 14.00f,  8.00f, 19.00f, 24.00f); // iio 7sus4
        SetChord(19, 10.00f, 17.00f, 19.00f, 26.00f); // VII 6
        SetChord(20,  7.00f, 14.00f, 22.00f, 24.00f); // v m11
        SetChord(21, 15.00f, 10.00f, 19.00f, 20.00f); // III add4
        SetChord(22, 12.00f, 19.00f, 20.00f, 27.00f); // i addb13
        SetChord(23,  8.00f, 15.00f, 24.00f, 26.00f); // VI add#11
        SetChord(24, 17.00f, 12.00f, 20.00f, 26.00f); // iv m6
        SetChord(25, 14.00f, 17.00f, 20.00f, 23.00f); // iio
        SetChord(26, 11.00f, 14.00f, 17.00f, 20.00f); // viio
        SetChord(27,  7.00f, 14.00f, 17.00f, 23.00f); // V 7
        SetChord(28,  4.00f,  7.00f, 17.00f, 23.00f); // iii addb9
        SetChord(29, 12.00f,  7.00f, 16.00f, 23.00f); // I maj7
        SetChord(30,  9.00f, 12.00f, 16.00f, 23.00f); // vi m9
        SetChord(31,  5.00f, 12.00f, 19.00f, 21.00f); // IV maj9
        SetChord(32, 14.00f,  9.00f, 17.00f, 24.00f); // ii m7
        SetChord(33, 11.00f,  5.00f, 19.00f, 24.00f); // I maj7sus4/vii
        SetChord(34,  7.00f, 14.00f, 17.00f, 24.00f); // V 7sus4

        quantizer_[0].Init(kChordNumOriginalChords, 0.075f);
        quantizer_[1].Init(kChordNumJonChords, 0.075f);
        quantizer_[2].Init(kChordNumJoeChords, 0.075f);

        bank_ = 0;
    }

    void SetChord(int index, float n0, float n1, float n2, float n3) {
        ratios_[index][0] = SemitonesToRatio(n0);
        ratios_[index][1] = SemitonesToRatio(n1);
        ratios_[index][2] = SemitonesToRatio(n2);
        ratios_[index][3] = SemitonesToRatio(n3);
    }

    void SetBank(int bank) {
        bank_ = std::max(0, std::min(bank, 2));
    }

    void SetChordParameter(float parameter) {
        quantizer_[bank_].Process(parameter * 1.02f);
    }

    int GetChordIndex() const {
        int value = quantizer_[bank_].quantized_value();
        if (bank_ == 0) {
            return originalChordMapping[value];
        } else if (bank_ == 1) {
            return value;
        }
        return kChordNumJonChords + value;
    }

    const float* GetRatios() const {
        return ratios_[GetChordIndex()];
    }

    float GetRatio(int note) const {
        return ratios_[GetChordIndex()][note];
    }

private:
    float ratios_[kChordNumChords][kChordNumNotes];
    HysteresisQuantizer quantizer_[3];
    int bank_;
};

class NaiveSvf {
public:
    void Init() {
        lp_ = bp_ = 0.0f;
        f_ = 0.1f;
        damp_ = 1.0f;
    }

    void set_f_q(float f, float resonance) {
        f = std::min(f, 0.158f);
        f_ = 2.0f * 3.14159265f * f;
        damp_ = 1.0f / resonance;
    }

    float Process(float in) {
        float bp_normalized = bp_ * damp_;
        float notch = in - bp_normalized;
        lp_ += f_ * bp_;
        float hp = notch - lp_;
        bp_ += f_ * hp;
        return lp_;
    }

private:
    float f_, damp_;
    float lp_, bp_;
};

class Ensemble {
public:
    Ensemble() : line_l_(nullptr), line_r_(nullptr) {}

    void Init(float sample_rate) {
        sample_rate_ = sample_rate;
        line_l_ = new float[kLineSize]();
        line_r_ = new float[kLineSize]();
        write_ptr_ = 0;
        phase_1_ = 0;
        phase_2_ = 0;
        amount_ = depth_ = 0.0f;
        mod_1_ = mod_2_ = mod_3_ = 0.0f;
        mod_1_inc_ = mod_2_inc_ = mod_3_inc_ = 0.0f;
    }

    ~Ensemble() {
        if (line_l_) delete[] line_l_;
        if (line_r_) delete[] line_r_;
    }

    void set_amount(float amount) { amount_ = amount; }
    void set_depth(float depth) { depth_ = depth; }

    inline float FastSine(uint32_t phase) {
        int32_t s = (int32_t)phase;
        float x = (float)s * (1.0f / 2147483648.0f);
        float ax = x < 0 ? -x : x;
        float y = 4.0f * x * (1.0f - ax);
        return y * (0.775f + 0.225f * (y < 0 ? -y : y));
    }

    void Process(float* left, float* right, int size) {
        const uint32_t one_third = 1431655765UL;
        const uint32_t two_third = 2863311530UL;

        float lfo_scale = 48000.0f / sample_rate_;
        float delay_scale = sample_rate_ / 48000.0f;

        uint32_t inc_slow = (uint32_t)(67289.0f * lfo_scale);
        uint32_t inc_fast = (uint32_t)(589980.0f * lfo_scale);

        float slow_0 = FastSine(phase_1_);
        float slow_120 = FastSine(phase_1_ + one_third);
        float slow_240 = FastSine(phase_1_ + two_third);
        float fast_0 = FastSine(phase_2_);
        float fast_120 = FastSine(phase_2_ + one_third);
        float fast_240 = FastSine(phase_2_ + two_third);

        float a = depth_ * 160.0f * delay_scale;
        float b = depth_ * 16.0f * delay_scale;

        float mod_1_target = slow_0 * a + fast_0 * b;
        float mod_2_target = slow_120 * a + fast_120 * b;
        float mod_3_target = slow_240 * a + fast_240 * b;

        float inv_size = 1.0f / (float)size;
        mod_1_inc_ = (mod_1_target - mod_1_) * inv_size;
        mod_2_inc_ = (mod_2_target - mod_2_) * inv_size;
        mod_3_inc_ = (mod_3_target - mod_3_) * inv_size;

        float base = 192.0f * delay_scale;
        float dry_amount = 1.0f - amount_ * 0.5f;

        for (int i = 0; i < size; ++i) {
            line_l_[write_ptr_] = left[i];
            line_r_[write_ptr_] = right[i];

            float d1 = mod_1_ + base;
            float d2 = mod_2_ + base;
            float d3 = mod_3_ + base;

            float wet_l = (ReadInterpolated(line_l_, d1)
                        + ReadInterpolated(line_l_, d2)
                        + ReadInterpolated(line_r_, d3)) * 0.333f;

            float wet_r = (ReadInterpolated(line_r_, d1)
                        + ReadInterpolated(line_r_, d2)
                        + ReadInterpolated(line_l_, d3)) * 0.333f;

            left[i] = wet_l * amount_ + left[i] * dry_amount;
            right[i] = wet_r * amount_ + right[i] * dry_amount;

            mod_1_ += mod_1_inc_;
            mod_2_ += mod_2_inc_;
            mod_3_ += mod_3_inc_;
            write_ptr_ = (write_ptr_ + 1) & kLineMask;
        }

        phase_1_ += inc_slow * size;
        phase_2_ += inc_fast * size;
    }

private:
    inline float ReadInterpolated(float* line, float delay) {
        if (delay < 0.0f) delay = 0.0f;
        else if (delay > kLineSize - 2) delay = kLineSize - 2;
        int rp = write_ptr_ - (int)delay;
        if (rp < 0) rp += kLineSize;
        int p0 = rp & kLineMask;
        int p1 = (rp - 1) & kLineMask;
        float frac = delay - (int)delay;
        return line[p0] + (line[p1] - line[p0]) * frac;
    }

    static const int kLineSize = 1024;
    static const int kLineMask = kLineSize - 1;

    float sample_rate_;
    float* line_l_;
    float* line_r_;
    int write_ptr_;
    uint32_t phase_1_;
    uint32_t phase_2_;
    float amount_;
    float depth_;
    float mod_1_, mod_2_, mod_3_;
    float mod_1_inc_, mod_2_inc_, mod_3_inc_;
};

class StringSynthOscillator {
public:
    void Init() {
        phase_ = 0.0f;
        next_sample_ = 0.0f;
        segment_ = 0;
        frequency_ = 0.001f;
        saw_8_gain_ = saw_4_gain_ = saw_2_gain_ = saw_1_gain_ = 0.0f;
    }

    void Render(float frequency, const float* reg, float gain, float* out, int size) {
        frequency *= 8.0f;

        int shift = 0;
        while (frequency > 0.5f && shift < 6) { shift += 2; frequency *= 0.5f; }
        if (shift >= 8) return;

        float registration[7];
        std::fill(registration, registration + 7, 0.0f);
        for (int i = 0; i < 7 - shift; ++i) {
            registration[i + shift] = (i < 6) ? reg[i] : 0.0f;
        }

        float t_saw_8 = (registration[0] + 2.0f * registration[1]) * gain;
        float t_saw_4 = (registration[2] - registration[1] + 2.0f * registration[3]) * gain;
        float t_saw_2 = (registration[4] - registration[3] + 2.0f * registration[5]) * gain;
        float t_saw_1 = (registration[6] - registration[5]) * gain;

        float inv = 1.0f / (float)size;
        float saw_8_inc = (t_saw_8 - saw_8_gain_) * inv;
        float saw_4_inc = (t_saw_4 - saw_4_gain_) * inv;
        float saw_2_inc = (t_saw_2 - saw_2_gain_) * inv;
        float saw_1_inc = (t_saw_1 - saw_1_gain_) * inv;
        float freq_inc = (frequency - frequency_) * inv;

        float phase = phase_, next_sample = next_sample_, freq = frequency_;
        float saw_8 = saw_8_gain_, saw_4 = saw_4_gain_, saw_2 = saw_2_gain_, saw_1 = saw_1_gain_;
        int segment = segment_;

        for (int i = 0; i < size; ++i) {
            float this_sample = next_sample;
            next_sample = 0.0f;

            freq += freq_inc;
            saw_8 += saw_8_inc; saw_4 += saw_4_inc; saw_2 += saw_2_inc; saw_1 += saw_1_inc;

            phase += freq;
            int next_segment = (int)phase;

            if (next_segment != segment) {
                float disc = 0.0f;
                if (next_segment >= 8) { phase -= 8.0f; next_segment -= 8; disc -= saw_8; }
                if ((next_segment & 3) == 0) disc -= saw_4;
                if ((next_segment & 1) == 0) disc -= saw_2;
                disc -= saw_1;
                if (disc != 0.0f) {
                    float frac = phase - (float)next_segment;
                    float t = frac / freq;
                    this_sample += ThisBlepSample(t) * disc;
                    next_sample += NextBlepSample(t) * disc;
                }
            }
            segment = next_segment;

            next_sample += (phase - 4.0f) * saw_8 * 0.125f;
            next_sample += (phase - (float)(segment & 4) - 2.0f) * saw_4 * 0.25f;
            next_sample += (phase - (float)(segment & 6) - 1.0f) * saw_2 * 0.5f;
            next_sample += (phase - (float)(segment & 7) - 0.5f) * saw_1;

            out[i] += 2.0f * this_sample;
        }

        phase_ = phase; next_sample_ = next_sample; segment_ = segment;
        frequency_ = frequency;
        saw_8_gain_ = t_saw_8; saw_4_gain_ = t_saw_4; saw_2_gain_ = t_saw_2; saw_1_gain_ = t_saw_1;
    }

private:
    float phase_, next_sample_, frequency_;
    float saw_8_gain_, saw_4_gain_, saw_2_gain_, saw_1_gain_;
    int segment_;
};

class StringMachineEngine {
public:
    StringMachineEngine() : ensemble_(nullptr) {}
    ~StringMachineEngine() {
        if (ensemble_) delete ensemble_;
    }

    void Init(float sample_rate) {
        sample_rate_ = sample_rate;

        chord_bank_.Init();

        for (int i = 0; i < kChordNumNotes; ++i) {
            oscillator_[i].Init();
        }
        filter_[0].Init();
        filter_[1].Init();

        ensemble_ = new Ensemble();
        ensemble_->Init(sample_rate);

        morph_lp_ = 0.0f;
        timbre_lp_ = 0.0f;
    }

    void ComputeRegistration(float registration, float* amplitudes) {
        registration *= (kRegistrationTableSize - 1.001f);
        int idx = std::max(0, std::min((int)registration, kRegistrationTableSize - 2));
        float frac = registration - (float)idx;
        for (int i = 0; i < kChordNumHarmonics * 2; ++i) {
            amplitudes[i] = kRegistrations[idx][i] + 
                (kRegistrations[idx + 1][i] - kRegistrations[idx][i]) * frac;
        }
    }

    void Render(
        float frequency,
        float fx,
        float wave,
        float chord,
        int bank,
        float* out_main,
        float* out_aux,
        int size) {

        const int kMaxBlockSize = 2048;
        if (size > kMaxBlockSize) size = kMaxBlockSize;

        morph_lp_ += 0.1f * (wave - morph_lp_);
        timbre_lp_ += 0.1f * (fx - timbre_lp_);

        chord_bank_.SetBank(bank);
        chord_bank_.SetChordParameter(chord);

        float harmonics[kChordNumHarmonics * 2 + 1];
        ComputeRegistration(std::max(0.0f, morph_lp_), harmonics);
        harmonics[kChordNumHarmonics * 2] = 0.0f;

        std::fill(out_main, out_main + size, 0.0f);
        std::fill(out_aux, out_aux + size, 0.0f);

        const float f0 = frequency / sample_rate_ * 0.998f;

        for (int note = 0; note < kChordNumNotes; ++note) {
            float ratio = chord_bank_.GetRatio(note);
            float note_f0 = f0 * ratio;

            float gain = std::max(0.0f, std::min(4.0f - note_f0 * 32.0f, 1.0f));

            float* target = (note & 1) ? out_aux : out_main;
            oscillator_[note].Render(note_f0, harmonics, 0.25f * gain, target, size);
        }

        float cutoff = 2.2f * f0 * SemitonesToRatio(120.0f * timbre_lp_);
        cutoff = std::max(0.01f, std::min(cutoff, 0.158f));
        filter_[0].set_f_q(cutoff, 1.0f);
        filter_[1].set_f_q(std::min(cutoff * 1.5f, 0.158f), 1.0f);

        for (int i = 0; i < size; ++i) {
            out_main[i] = filter_[0].Process(out_main[i]);
            out_aux[i] = filter_[1].Process(out_aux[i]);
        }

        for (int i = 0; i < size; ++i) {
            float l = out_main[i], r = out_aux[i];
            out_main[i] = 0.66f * l + 0.33f * r;
            out_aux[i] = 0.66f * r + 0.33f * l;
        }

        float amount = std::fabs(timbre_lp_ - 0.5f) * 2.0f;
        float depth = 0.35f + 0.65f * timbre_lp_;
        ensemble_->set_amount(amount);
        ensemble_->set_depth(depth);
        ensemble_->Process(out_main, out_aux, size);

        for (int i = 0; i < size; ++i) {
            out_main[i] *= 1.3f;
            out_aux[i] *= 1.3f;
        }
    }

private:
    float sample_rate_;
    float morph_lp_;
    float timbre_lp_;
    ChordBank chord_bank_;
    StringSynthOscillator oscillator_[kChordNumNotes];
    NaiveSvf filter_[2];
    Ensemble* ensemble_;
};

}  // namespace SMEmu
