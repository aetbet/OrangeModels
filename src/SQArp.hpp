#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace SQArp {

const int kChordNumNotes = 4;
const int kChordNumVoices = kChordNumNotes + 1;

const int kChordNumOriginalChords = 11;
const int kChordNumJonChords = 17;
const int kChordNumJoeChords = 18;
const int kChordNumChords = kChordNumJonChords + kChordNumJoeChords;

const float kMaxFrequency = 0.25f;

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

inline float ThisBlepSample(float t) { return 0.5f * t * t; }
inline float NextBlepSample(float t) { t = 1.0f - t; return -0.5f * t * t; }
inline float ThisIntegratedBlepSample(float t) {
    return 0.5f * t * t * (1.0f - t * 0.6666667f);
}
inline float NextIntegratedBlepSample(float t) {
    t = 1.0f - t;
    return -0.5f * t * t * (1.0f - t * 0.6666667f);
}

inline float SemitonesToRatio(float semitones) {
    return std::pow(2.0f, semitones / 12.0f);
}

class HysteresisQuantizer2 {
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

using HysteresisQuantizer = HysteresisQuantizer2;

class ChordBank {
public:
    void Init() {
        // Jon Butler chords (indices 0-16)
        SetChord(0,  0.00f, 0.01f, 11.99f, 12.00f);   // Octave
        SetChord(1,  0.00f, 7.00f,  7.01f, 12.00f);   // Fifth
        SetChord(2,  0.00f, 3.00f,  7.00f, 12.00f);   // Minor
        SetChord(3,  0.00f, 3.00f,  7.00f, 10.00f);   // Minor 7th
        SetChord(4,  0.00f, 3.00f, 10.00f, 14.00f);   // Minor 9th
        SetChord(5,  0.00f, 3.00f, 10.00f, 17.00f);   // Minor 11th
        SetChord(6,  0.00f, 4.00f,  7.00f, 12.00f);   // Major
        SetChord(7,  0.00f, 4.00f,  7.00f, 11.00f);   // Major 7th
        SetChord(8,  0.00f, 4.00f, 11.00f, 14.00f);   // Major 9th
        SetChord(9,  0.00f, 5.00f,  7.00f, 12.00f);   // Sus4
        SetChord(10, 0.00f, 2.00f,  9.00f, 16.00f);   // 69
        SetChord(11, 0.00f, 4.00f,  7.00f,  9.00f);   // 6th
        SetChord(12, 0.00f, 7.00f, 16.00f, 23.00f);   // 10th
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
        num_notes_ = kChordNumNotes;

        ComputeNoteCounts();
    }

    void SetChordWithSemitones(int index, float n0, float n1, float n2, float n3) {
        semitones_[index][0] = n0;
        semitones_[index][1] = n1;
        semitones_[index][2] = n2;
        semitones_[index][3] = n3;
        ratios_[index][0] = SemitonesToRatio(n0);
        ratios_[index][1] = SemitonesToRatio(n1);
        ratios_[index][2] = SemitonesToRatio(n2);
        ratios_[index][3] = SemitonesToRatio(n3);
    }

    void SetChord(int index, float n0, float n1, float n2, float n3) {
        SetChordWithSemitones(index, n0, n1, n2, n3);
    }

    void ComputeNoteCounts() {
        for (int i = 0; i < kChordNumChords; ++i) {
            int count = 0;
            for (int j = 0; j < kChordNumNotes; ++j) {
                float s = semitones_[i][j];
                if (s != 0.01f && s != 7.01f && s != 11.99f && s != 12.00f) {
                    ++count;
                }
            }
            note_count_[i] = count > 0 ? count : 1;
        }
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

    float GetRatio(int note) const {
        return ratios_[GetChordIndex()][note];
    }

    void Sort() {
        int idx = GetChordIndex();
        for (int i = 0; i < kChordNumNotes; ++i) {
            float r = ratios_[idx][i];
            while (r > 2.0f) {
                r *= 0.5f;
            }
            sorted_ratios_[i] = r;
        }
        for (int i = 0; i < kChordNumNotes - 1; ++i) {
            for (int j = 0; j < kChordNumNotes - 1 - i; ++j) {
                if (sorted_ratios_[j] > sorted_ratios_[j + 1]) {
                    float tmp = sorted_ratios_[j];
                    sorted_ratios_[j] = sorted_ratios_[j + 1];
                    sorted_ratios_[j + 1] = tmp;
                }
            }
        }
        num_notes_ = note_count_[idx];
    }

    float sorted_ratio(int note) const {
        return sorted_ratios_[note];
    }

    int num_notes() const { return num_notes_; }

    void ComputeChordInversion(float inversion, float* ratios, float* amplitudes) {
        const float* base_ratio = ratios_[GetChordIndex()];
        inversion = inversion * float(kChordNumNotes * kChordNumVoices);

        int inversion_integral = static_cast<int>(inversion);
        float inversion_fractional = inversion - static_cast<float>(inversion_integral);

        int num_rotations = inversion_integral / kChordNumNotes;
        int rotated_note = inversion_integral % kChordNumNotes;

        const float kBaseGain = 0.25f;

        for (int i = 0; i < kChordNumNotes; ++i) {
            float transposition = 0.25f * static_cast<float>(
                1 << ((kChordNumNotes - 1 + inversion_integral - i) / kChordNumNotes));
            int target_voice = (i - num_rotations + kChordNumVoices) % kChordNumVoices;
            int previous_voice = (target_voice - 1 + kChordNumVoices) % kChordNumVoices;

            if (i == rotated_note) {
                ratios[target_voice] = base_ratio[i] * transposition;
                ratios[previous_voice] = ratios[target_voice] * 2.0f;
                amplitudes[previous_voice] = kBaseGain * inversion_fractional;
                amplitudes[target_voice] = kBaseGain * (1.0f - inversion_fractional);
            } else if (i < rotated_note) {
                ratios[previous_voice] = base_ratio[i] * transposition;
                amplitudes[previous_voice] = kBaseGain;
            } else {
                ratios[target_voice] = base_ratio[i] * transposition;
                amplitudes[target_voice] = kBaseGain;
            }
        }
    }

private:
    float semitones_[kChordNumChords][kChordNumNotes];
    float ratios_[kChordNumChords][kChordNumNotes];
    float sorted_ratios_[kChordNumNotes];
    int note_count_[kChordNumChords];
    HysteresisQuantizer quantizer_[3];
    int bank_;
    int num_notes_;
};

enum ArpeggiatorMode {
    ARPEGGIATOR_MODE_UP,
    ARPEGGIATOR_MODE_DOWN,
    ARPEGGIATOR_MODE_UP_DOWN,
    ARPEGGIATOR_MODE_RANDOM,
    ARPEGGIATOR_MODE_LAST
};

class Arpeggiator {
public:
    void Init() {
        mode_ = ARPEGGIATOR_MODE_UP;
        range_ = 1;
        note_ = 0;
        octave_ = 0;
        direction_ = 1;
        rng_state_ = 12345;
    }

    void Reset() {
        note_ = 0;
        octave_ = 0;
        direction_ = 1;
    }

    void set_mode(ArpeggiatorMode mode) { mode_ = mode; }
    void set_range(int range) { range_ = range; }

    int note() const { return note_; }
    int octave() const { return octave_; }

    uint32_t Random() {
        rng_state_ = rng_state_ * 1664525 + 1013904223;
        return rng_state_;
    }

    void Clock(int num_notes) {
        if (num_notes == 1 && range_ == 1) {
            note_ = octave_ = 0;
            return;
        }

        if (mode_ == ARPEGGIATOR_MODE_RANDOM) {
            for (int i = 0; i < 10; ++i) {
                uint32_t w = Random();
                int octave = (w >> 4) % range_;
                int note = (w >> 20) % num_notes;
                if (octave != octave_ || note != note_) {
                    octave_ = octave;
                    note_ = note;
                    break;
                }
            }
            return;
        }

        if (mode_ == ARPEGGIATOR_MODE_UP) direction_ = 1;
        if (mode_ == ARPEGGIATOR_MODE_DOWN) direction_ = -1;

        note_ += direction_;

        bool done = false;
        while (!done) {
            done = true;
            if (note_ >= num_notes || note_ < 0) {
                octave_ += direction_;
                note_ = direction_ > 0 ? 0 : num_notes - 1;
            }
            if (octave_ >= range_ || octave_ < 0) {
                octave_ = direction_ > 0 ? 0 : range_ - 1;
                if (mode_ == ARPEGGIATOR_MODE_UP_DOWN) {
                    direction_ = -direction_;
                    note_ = direction_ > 0 ? 1 : num_notes - 2;
                    octave_ = direction_ > 0 ? 0 : range_ - 1;
                    done = false;
                }
            }
        }
    }

private:
    ArpeggiatorMode mode_;
    int range_;
    int note_;
    int octave_;
    int direction_;
    uint32_t rng_state_;
};

class SuperSquareOscillator {
public:
    void Init() {
        master_phase_ = 0.0f;
        slave_phase_ = 0.0f;
        next_sample_ = 0.0f;
        high_ = false;
        master_frequency_ = 0.0f;
        slave_frequency_ = 0.01f;
    }

    void Render(float frequency, float shape, float* out, int size) {
        float master_frequency = frequency;

        frequency *= shape < 0.5f
            ? (0.51f + 0.98f * shape)
            : 1.0f + 16.0f * (shape - 0.5f) * (shape - 0.5f);

        if (master_frequency >= kMaxFrequency) master_frequency = kMaxFrequency;
        if (frequency >= kMaxFrequency) frequency = kMaxFrequency;

        float inv = 1.0f / (float)size;
        float master_freq_inc = (master_frequency - master_frequency_) * inv;
        float slave_freq_inc = (frequency - slave_frequency_) * inv;

        float master_freq = master_frequency_;
        float slave_freq = slave_frequency_;
        float next_sample = next_sample_;

        for (int i = 0; i < size; ++i) {
            bool reset = false;
            bool transition_during_reset = false;
            float reset_time = 0.0f;

            float this_sample = next_sample;
            next_sample = 0.0f;

            master_freq += master_freq_inc;
            slave_freq += slave_freq_inc;

            master_phase_ += master_freq;
            if (master_phase_ >= 1.0f) {
                master_phase_ -= 1.0f;
                reset_time = master_phase_ / master_freq;

                float slave_phase_at_reset = slave_phase_ + (1.0f - reset_time) * slave_freq;
                reset = true;
                if (slave_phase_at_reset >= 1.0f) {
                    slave_phase_at_reset -= 1.0f;
                    transition_during_reset = true;
                }
                if (!high_ && slave_phase_at_reset >= 0.5f) {
                    transition_during_reset = true;
                }
                float value = slave_phase_at_reset < 0.5f ? 0.0f : 1.0f;
                this_sample -= value * ThisBlepSample(reset_time);
                next_sample -= value * NextBlepSample(reset_time);
            }

            slave_phase_ += slave_freq;
            while (transition_during_reset || !reset) {
                if (!high_) {
                    if (slave_phase_ < 0.5f) break;
                    float t = (slave_phase_ - 0.5f) / slave_freq;
                    this_sample += ThisBlepSample(t);
                    next_sample += NextBlepSample(t);
                    high_ = true;
                }

                if (high_) {
                    if (slave_phase_ < 1.0f) break;
                    slave_phase_ -= 1.0f;
                    float t = slave_phase_ / slave_freq;
                    this_sample -= ThisBlepSample(t);
                    next_sample -= NextBlepSample(t);
                    high_ = false;
                }
            }

            if (reset) {
                slave_phase_ = reset_time * slave_freq;
                high_ = false;
            }

            next_sample += slave_phase_ < 0.5f ? 0.0f : 1.0f;
            out[i] = 2.0f * this_sample - 1.0f;
        }

        master_frequency_ = master_frequency;
        slave_frequency_ = frequency;
        next_sample_ = next_sample;
    }

private:
    float master_phase_;
    float slave_phase_;
    float next_sample_;
    bool high_;
    float master_frequency_;
    float slave_frequency_;
};

template<int num_bits = 5>
class NESTriangleOscillator {
public:
    void Init() {
        phase_ = 0.0f;
        step_ = 0;
        ascending_ = true;
        next_sample_ = 0.0f;
        frequency_ = 0.001f;
    }

    void Render(float frequency, float* out, int size) {
        const int num_steps = 1 << num_bits;
        const int half = num_steps / 2;
        const int top = num_steps != 2 ? num_steps - 1 : 2;
        const float num_steps_f = static_cast<float>(num_steps);
        const float scale = num_steps != 2
            ? 4.0f / static_cast<float>(top - 1)
            : 2.0f;

        frequency = std::min(frequency, 0.25f);

        float freq_inc = (frequency - frequency_) / (float)size;
        float freq = frequency_;
        float next_sample = next_sample_;

        for (int i = 0; i < size; ++i) {
            freq += freq_inc;
            phase_ += freq;

            float fade_to_tri = (freq - 0.5f / num_steps_f) * 2.0f * num_steps_f;
            if (fade_to_tri < 0.0f) fade_to_tri = 0.0f;
            if (fade_to_tri > 1.0f) fade_to_tri = 1.0f;

            const float nes_gain = 1.0f - fade_to_tri;
            const float tri_gain = fade_to_tri * 2.0f / scale;

            float this_sample = next_sample;
            next_sample = 0.0f;

            if (ascending_ && phase_ >= 0.5f) {
                float discontinuity = 4.0f * freq * tri_gain;
                if (discontinuity != 0.0f) {
                    float t = (phase_ - 0.5f) / freq;
                    this_sample -= ThisIntegratedBlepSample(t) * discontinuity;
                    next_sample -= NextIntegratedBlepSample(t) * discontinuity;
                }
                ascending_ = false;
            }

            int next_step = static_cast<int>(phase_ * num_steps_f);
            if (next_step != step_) {
                bool wrap = false;
                if (next_step >= num_steps) {
                    phase_ -= 1.0f;
                    next_step -= num_steps;
                    wrap = true;
                }

                float discontinuity = next_step < half ? 1.0f : -1.0f;
                if (num_steps == 2) {
                    discontinuity = -discontinuity;
                } else {
                    if (next_step == 0 || next_step == half) {
                        discontinuity = 0.0f;
                    }
                }

                discontinuity *= nes_gain;
                if (discontinuity != 0.0f) {
                    float frac = (phase_ * num_steps_f - static_cast<float>(next_step));
                    float t = frac / (freq * num_steps_f);
                    this_sample += ThisBlepSample(t) * discontinuity;
                    next_sample += NextBlepSample(t) * discontinuity;
                }

                if (wrap) {
                    float discontinuity = 4.0f * freq * tri_gain;
                    if (discontinuity != 0.0f) {
                        float t = phase_ / freq;
                        this_sample += ThisIntegratedBlepSample(t) * discontinuity;
                        next_sample += NextIntegratedBlepSample(t) * discontinuity;
                    }
                    ascending_ = true;
                }
            }
            step_ = next_step;

            next_sample += nes_gain * static_cast<float>(step_ < half ? step_ : top - step_);

            next_sample += tri_gain * (phase_ < 0.5f ? 2.0f * phase_ : 2.0f - 2.0f * phase_);

            out[i] = this_sample * scale - 1.0f;
        }

        frequency_ = frequency;
        next_sample_ = next_sample;
    }

private:
    float phase_;
    float next_sample_;
    int step_;
    bool ascending_;
    float frequency_;
};

class ChiptuneEngine {
public:
    ChiptuneEngine() {}
    ~ChiptuneEngine() {}

    static constexpr float NO_ENVELOPE = 2.0f;

    void Init(float sample_rate) {
        sample_rate_ = sample_rate;

        bass_.Init();
        for (int i = 0; i < kChordNumVoices; ++i) {
            voice_[i].Init();
        }

        chord_bank_.Init();
        chord_bank_.SetChordParameter(0.0f);
        chord_bank_.Sort();

        arpeggiator_.Init();
        pattern_quantizer_.Init(12, 0.075f, false);

        envelope_shape_ = NO_ENVELOPE;
        envelope_state_ = 1.0f;
        aux_envelope_amount_ = 0.0f;

        prev_trig_ = false;
    }

    void Render(
        float frequency,
        bool trigger_patched,
        bool trig,
        float envelope,
        float type,
        float morph,
        float chord,
        int bank,
        float* out_main,
        float* out_aux,
        int size) {

        const float f0 = frequency / sample_rate_;
        const float shape = morph * 0.995f;
        float root_transposition = 1.0f;

        envelope_shape_ = envelope;

        chord_bank_.SetBank(bank);
        chord_bank_.SetChordParameter(chord);

        bool rising_edge = trig && !prev_trig_;
        prev_trig_ = trig;

        if (trigger_patched) {
            if (rising_edge) {
                chord_bank_.Sort();

                int pattern = pattern_quantizer_.Process(type);
                arpeggiator_.set_mode(ArpeggiatorMode(pattern / 3));
                arpeggiator_.set_range(1 << (pattern % 3));
                arpeggiator_.Clock(chord_bank_.num_notes());
                envelope_state_ = 1.0f;
            }

            const float octave = float(1 << arpeggiator_.octave());
            const float note_f0 = f0 * chord_bank_.sorted_ratio(arpeggiator_.note()) * octave;
            root_transposition = octave;
            voice_[0].Render(note_f0, shape, out_main, size);
        } else {
            float ratios[kChordNumVoices];
            float amplitudes[kChordNumVoices];

            chord_bank_.ComputeChordInversion(type, ratios, amplitudes);
            for (int j = 1; j < kChordNumVoices; j += 2) {
                amplitudes[j] = -amplitudes[j];
            }

            std::fill(out_main, out_main + size, 0.0f);
            float temp[2048];

            for (int voice = 0; voice < kChordNumVoices; ++voice) {
                const float voice_f0 = f0 * ratios[voice];
                voice_[voice].Render(voice_f0, shape, temp, size);
                for (int j = 0; j < size; ++j) {
                    out_main[j] += temp[j] * amplitudes[voice];
                }
            }
        }

        bass_.Render(f0 * 0.5f * root_transposition, out_aux, size);

        if (trigger_patched && envelope_shape_ != 0.0f) {
            const float shape_abs = std::fabs(envelope_shape_);
            const float decay = 1.0f - 2.0f / sample_rate_ * SemitonesToRatio(60.0f * shape_abs) * shape_abs;
            float aux_env_target = envelope_shape_ * 20.0f;
            if (aux_env_target < 0.0f) aux_env_target = 0.0f;
            if (aux_env_target > 1.0f) aux_env_target = 1.0f;

            for (int i = 0; i < size; ++i) {
                aux_envelope_amount_ += 0.01f * (aux_env_target - aux_envelope_amount_);
                envelope_state_ *= decay;
                out_main[i] *= envelope_state_;
                out_aux[i] *= 1.0f + aux_envelope_amount_ * (envelope_state_ - 1.0f);
            }
        }

        for (int i = 0; i < size; ++i) {
            out_main[i] *= 0.8f;
            out_aux[i] *= 0.8f;
            if (std::fabs(out_main[i]) < 1e-20f) out_main[i] = 0.0f;
            if (std::fabs(out_aux[i]) < 1e-20f) out_aux[i] = 0.0f;
        }
    }

private:
    float sample_rate_;

    SuperSquareOscillator voice_[kChordNumVoices];
    NESTriangleOscillator<> bass_;

    ChordBank chord_bank_;
    Arpeggiator arpeggiator_;
    HysteresisQuantizer pattern_quantizer_;

    float envelope_shape_;
    float envelope_state_;
    float aux_envelope_amount_;

    bool prev_trig_;
};

}  // namespace SQArp
