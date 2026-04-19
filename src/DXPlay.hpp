#pragma once

#include "SC_PlugIn.hpp"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace DXPlay {

constexpr int kNumOperators = 6;
constexpr int kNumAlgorithms = 32;
constexpr int kNumPatches = 32;
constexpr int kPatchSize = 128;
constexpr int kNumVoices = 2;
constexpr size_t kSineLUTSize = 512;
constexpr size_t kSineLUTBits = 9;

extern float lut_sine[kSineLUTSize + 1];

void InitSineLUT();

inline float SinePM(uint32_t phase, float pm) {
    const float max_uint32 = 4294967296.0f;
    const int max_index = 32;
    const float offset = float(max_index);
    const float scale = max_uint32 / float(max_index * 2);

    phase += static_cast<uint32_t>((pm + offset) * scale) * max_index * 2;

    uint32_t integral = phase >> (32 - kSineLUTBits);
    float fractional = static_cast<float>(phase << kSineLUTBits) / max_uint32;
    float a = lut_sine[integral];
    float b = lut_sine[integral + 1];
    return a + (b - a) * fractional;
}

template<int order>
inline float Pow2Fast(float x) {
    union {
        float f;
        int32_t w;
    } r;

    if (order == 1) {
        r.w = float(1 << 23) * (127.0f + x);
        return r.f;
    }

    int32_t x_integral = static_cast<int32_t>(x);
    if (x < 0.0f) {
        --x_integral;
    }
    x -= static_cast<float>(x_integral);

    if (order == 2) {
        r.f = 1.0f + x * (0.6565f + x * 0.3435f);
    } else if (order == 3) {
        r.f = 1.0f + x * (0.6958f + x * (0.2251f + x * 0.0791f));
    }
    r.w += x_integral << 23;
    return r.f;
}

inline float SemitonesToRatio(float semitones) {
    return Pow2Fast<2>(semitones * (1.0f / 12.0f));
}

struct Patch {
    struct Envelope {
        uint8_t rate[4];
        uint8_t level[4];
    };

    struct KeyboardScaling {
        uint8_t break_point;
        uint8_t left_depth;
        uint8_t right_depth;
        uint8_t left_curve;
        uint8_t right_curve;
    };

    struct Operator {
        Envelope envelope;
        KeyboardScaling keyboard_scaling;
        uint8_t rate_scaling;
        uint8_t amp_mod_sensitivity;
        uint8_t velocity_sensitivity;
        uint8_t level;
        uint8_t mode;
        uint8_t coarse;
        uint8_t fine;
        uint8_t detune;
    } op[6];

    Envelope pitch_envelope;
    uint8_t algorithm;
    uint8_t feedback;
    uint8_t reset_phase;

    struct ModulationParameters {
        uint8_t rate;
        uint8_t delay;
        uint8_t pitch_mod_depth;
        uint8_t amp_mod_depth;
        uint8_t reset_phase;
        uint8_t waveform;
        uint8_t pitch_mod_sensitivity;
    } modulations;

    uint8_t transpose;
    uint8_t name[10];
    uint8_t active_operators;

    void Unpack(const uint8_t* data);
    void Clear();
};

int OperatorLevel(int level);
float OperatorEnvelopeIncrement(int rate);
float PitchEnvelopeLevel(int level);
float PitchEnvelopeIncrement(int rate);
float NormalizeVelocity(float velocity);
float RateScaling(float note, int rate_scaling);
float KeyboardScaling(float note, const Patch::KeyboardScaling& ks);
float FrequencyRatio(const Patch::Operator& op);
float AmpModSensitivity(int amp_mod_sensitivity);

class OperatorEnvelope {
public:
    void Init(float scale = 1.0f);
    void Set(const uint8_t rate[4], const uint8_t level[4], int op_level);
    void Reset();
    float Render(bool gate, float rate, float ad_scale, float r_scale);
    float RenderAtSample(float t, float gate_duration);

private:
    float value(int stage, float phase, float start);
    float value();

    float scale_;
    int stage_;
    float phase_;
    float start_;
    float increment_[4];
    float level_[4];
};

class PitchEnvelope {
public:
    void Init(float scale = 1.0f);
    void Set(const uint8_t rate[4], const uint8_t level[4]);
    void Reset();
    float Render(bool gate, float rate, float ad_scale, float r_scale);
    float RenderAtSample(float t, float gate_duration);

private:
    float value(int stage, float phase, float start);
    float value();

    float scale_;
    int stage_;
    float phase_;
    float start_;
    float increment_[4];
    float level_[4];
};

struct Operator {
    uint32_t phase;
    float amplitude;

    void Reset() {
        phase = 0;
        amplitude = 0.0f;
    }
};

class Lfo {
public:
    void Init(float sample_rate);
    void Set(const Patch::ModulationParameters& mod);
    void Reset();
    void Step(float num_samples);
    void Scrub(float position);

    float pitch_mod() const { return pitch_mod_; }
    float amp_mod() const { return amp_mod_; }

private:
    void ComputeModValues();
    float sample_rate_;
    float phase_;
    float frequency_;
    float delay_phase_;
    float delay_increment_[2];
    float pitch_mod_depth_;
    float amp_mod_depth_;
    int waveform_;
    bool reset_phase_;

    float random_value_;

    float pitch_mod_;
    float amp_mod_;
};

class AlgorithmRenderer {
public:
    void Init();

    bool isCarrier(int algorithm, int op) const;

    void Render(
        int algorithm,
        Operator* ops,
        const float* frequencies,
        const float* amplitudes,
        float* feedback_state,
        int feedback_amount,
        float sample_rate,
        float* out,
        size_t size);

private:
    static const uint8_t carrier_mask_[kNumAlgorithms];
    static const uint8_t modulator_connections_[kNumAlgorithms][kNumOperators];
};

class FMVoice {
public:
    void Init(float sample_rate);
    void SetPatch(const Patch* patch);
    void Retrigger();
    void Render(
        bool sustain,
        bool gate,
        float note,
        float velocity,
        float brightness,
        float envelope_control,
        float pitch_mod,
        float amp_mod,
        float* out,
        size_t size);

    const Patch* patch() const { return patch_; }
    Lfo& lfo() { return lfo_; }

private:
    void Setup();

    float sample_rate_;
    float one_hz_;
    float a0_;

    const Patch* patch_;
    bool dirty_;

    Operator operators_[kNumOperators];
    OperatorEnvelope op_envelopes_[kNumOperators];
    PitchEnvelope pitch_envelope_;
    Lfo lfo_;

    float ratios_[kNumOperators];
    float level_headroom_[kNumOperators];
    float feedback_state_[2];

    bool gate_;
    float note_;
    float normalized_velocity_;

    AlgorithmRenderer algorithm_renderer_;
};

class PresetBank {
public:
    PresetBank();

    bool LoadSysEx(const uint8_t* data, size_t size);
    bool LoadPackedBank(const uint8_t* data, size_t size);
    const Patch& GetPatch(int index) const;
    void Clear();

private:
    Patch patches_[kNumPatches];
    bool loaded_;
};

class DXPlayEngine {
public:
    DXPlayEngine();

    void Init(float sample_rate);
    void LoadBank(const uint8_t* data, size_t size);

    void Render(
        float freq_hz,
        bool trig_connected,
        bool trig_high,
        bool trig_rising,
        float velocity,
        float timbre,
        float morph,
        int preset,
        float* out,
        size_t size);

private:
    float sample_rate_;
    PresetBank bank_;
    FMVoice voices_[kNumVoices];
    float acc_buffer_[4096];

    int active_voice_;
    int rendered_voice_;
    int gate_samples_[kNumVoices];

    float lpf_state_;
    float lpf_coeff_;
    float last_valid_freq_hz_;
};

}  // namespace DXPlay
