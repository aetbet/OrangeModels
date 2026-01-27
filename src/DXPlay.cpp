#include "DXPlay.hpp"

namespace DXPlay {

float lut_sine[kSineLUTSize + 1];

void InitSineLUT() {
    for (size_t i = 0; i <= kSineLUTSize; ++i) {
        lut_sine[i] = std::sin(2.0f * M_PI * float(i) / float(kSineLUTSize));
    }
}

static const float lut_coarse[32] = {
    -12.0f, 0.0f, 12.0f, 19.02f, 24.0f, 27.86f, 31.02f, 33.69f,
    36.0f, 38.04f, 39.86f, 41.51f, 43.02f, 44.41f, 45.69f, 46.88f,
    48.0f, 49.05f, 50.04f, 50.98f, 51.86f, 52.71f, 53.51f, 54.28f,
    55.02f, 55.73f, 56.41f, 57.07f, 57.69f, 58.30f, 58.88f, 59.45f
};

static const float lut_amp_mod_sensitivity[4] = {
    0.0f, 0.2588f, 0.4274f, 1.0f
};

static const float lut_pitch_mod_sensitivity[8] = {
    0.0f, 0.078125f, 0.15625f, 0.2578125f,
    0.4296875f, 0.71875f, 1.1953125f, 2.0f
};

static const float lut_cube_root[17] = {
    0.0f, 0.5f, 0.63f, 0.72f, 0.79f, 0.85f, 0.89f, 0.93f, 0.96f,
    0.99f, 1.01f, 1.04f, 1.06f, 1.08f, 1.10f, 1.12f, 1.14f
};

void Patch::Unpack(const uint8_t* data) {
    for (int i = 0; i < 6; ++i) {
        Operator* o = &op[i];
        const uint8_t* op_data = &data[i * 17];

        for (int j = 0; j < 4; ++j) {
            o->envelope.rate[j] = std::min<int>(op_data[j] & 0x7f, 99);
            o->envelope.level[j] = std::min<int>(op_data[4 + j] & 0x7f, 99);
        }

        o->keyboard_scaling.break_point = std::min<int>(op_data[8] & 0x7f, 99);
        o->keyboard_scaling.left_depth = std::min<int>(op_data[9] & 0x7f, 99);
        o->keyboard_scaling.right_depth = std::min<int>(op_data[10] & 0x7f, 99);
        o->keyboard_scaling.left_curve = op_data[11] & 0x3;
        o->keyboard_scaling.right_curve = (op_data[11] >> 2) & 0x3;

        o->rate_scaling = op_data[12] & 0x7;
        o->amp_mod_sensitivity = op_data[13] & 0x3;
        o->velocity_sensitivity = (op_data[13] >> 2) & 0x7;
        o->level = std::min<int>(op_data[14] & 0x7f, 99);
        o->mode = op_data[15] & 0x1;
        o->coarse = (op_data[15] >> 1) & 0x1f;
        o->fine = std::min<int>(op_data[16] & 0x7f, 99);
        o->detune = std::min<int>((op_data[12] >> 3) & 0xf, 14);
    }

    for (int j = 0; j < 4; ++j) {
        pitch_envelope.rate[j] = std::min<int>(data[102 + j] & 0x7f, 99);
        pitch_envelope.level[j] = std::min<int>(data[106 + j] & 0x7f, 99);
    }

    algorithm = data[110] & 0x1f;
    feedback = data[111] & 0x7;
    reset_phase = (data[111] >> 3) & 0x1;

    modulations.rate = std::min<int>(data[112] & 0x7f, 99);
    modulations.delay = std::min<int>(data[113] & 0x7f, 99);
    modulations.pitch_mod_depth = std::min<int>(data[114] & 0x7f, 99);
    modulations.amp_mod_depth = std::min<int>(data[115] & 0x7f, 99);
    modulations.reset_phase = data[116] & 0x1;
    modulations.waveform = std::min<int>((data[116] >> 1) & 0x7, 5);
    modulations.pitch_mod_sensitivity = data[116] >> 4;

    transpose = std::min<int>(data[117] & 0x7f, 48);

    for (size_t i = 0; i < sizeof(name); ++i) {
        name[i] = data[118 + i] & 0x7f;
    }
    active_operators = 0x3f;
}

void Patch::Clear() {
    std::memset(this, 0, sizeof(Patch));
    algorithm = 0;
    feedback = 0;
    active_operators = 0x3f;
    for (int i = 0; i < 6; ++i) {
        op[i].coarse = 1;
        for (int j = 0; j < 4; ++j) {
            op[i].envelope.rate[j] = 99;
        }
    }
}

int OperatorLevel(int level) {
    int tlc = level;
    if (level < 20) {
        tlc = tlc < 15 ? (tlc * (36 - tlc)) >> 3 : 27 + tlc;
    } else {
        tlc += 28;
    }
    return tlc;
}

float OperatorEnvelopeIncrement(int rate) {
    int rate_scaled = (rate * 41) >> 6;
    int mantissa = 4 + (rate_scaled & 3);
    int exponent = 2 + (rate_scaled >> 2);
    return float(mantissa << exponent) / float(1 << 24);
}

float PitchEnvelopeLevel(int level) {
    float l = (float(level) - 50.0f) / 32.0f;
    float tail = std::max(std::abs(l + 0.02f) - 1.0f, 0.0f);
    return l * (1.0f + tail * tail * 5.3056f);
}

float PitchEnvelopeIncrement(int rate) {
    float r = float(rate) * 0.01f;
    return (1.0f + 192.0f * r * (r * r * r * r + 0.3333f)) / (21.3f * 44100.0f);
}

float NormalizeVelocity(float velocity) {
    int index = static_cast<int>(velocity * 16.0f);
    index = std::min(std::max(index, 0), 16);
    float frac = velocity * 16.0f - float(index);
    float cube_root = lut_cube_root[index];
    if (index < 16) {
        cube_root += (lut_cube_root[index + 1] - cube_root) * frac;
    }
    return 16.0f * (cube_root - 0.918f);
}

float RateScaling(float note, int rate_scaling) {
    return Pow2Fast<1>(float(rate_scaling) * (note * 0.33333f - 7.0f) * 0.03125f);
}

float KeyboardScaling(float note, const Patch::KeyboardScaling& ks) {
    const float x = note - float(ks.break_point) - 15.0f;
    const int curve = x > 0.0f ? ks.right_curve : ks.left_curve;

    float t = std::abs(x);
    if (curve == 1 || curve == 2) {
        t = std::min(t * 0.010467f, 1.0f);
        t = t * t * t;
        t *= 96.0f;
    }
    if (curve < 2) {
        t = -t;
    }

    float depth = float(x > 0.0f ? ks.right_depth : ks.left_depth);
    return t * depth * 0.02677f;
}

float FrequencyRatio(const Patch::Operator& op) {
    const float detune = op.mode == 0 && op.fine
        ? 1.0f + 0.01f * float(op.fine)
        : 1.0f;

    float base = op.mode == 0
        ? lut_coarse[op.coarse]
        : float(int(op.coarse & 3) * 100 + op.fine) * 0.39864f;
    base += (float(op.detune) - 7.0f) * 0.015f;

    return SemitonesToRatio(base) * detune;
}

float AmpModSensitivity(int amp_mod_sensitivity) {
    return lut_amp_mod_sensitivity[amp_mod_sensitivity & 3];
}

void OperatorEnvelope::Init(float scale) {
    scale_ = scale;
    stage_ = 3;
    phase_ = 1.0f;
    start_ = 0.0f;
    for (int i = 0; i < 4; ++i) {
        increment_[i] = 0.001f;
        level_[i] = 0.0f;
    }
}

void OperatorEnvelope::Set(const uint8_t rate[4], const uint8_t level[4], int op_level) {
    for (int i = 0; i < 4; ++i) {
        int level_scaled = OperatorLevel(level[i]);
        level_scaled = (level_scaled & ~1) + op_level - 133;
        level_[i] = 0.125f * (level_scaled < 1 ? 0.5f : float(level_scaled));

        increment_[i] = OperatorEnvelopeIncrement(rate[i]) * scale_;
    }

    for (int i = 0; i < 4; ++i) {
        float from = level_[(i - 1 + 4) % 4];
        float to = level_[i];
        float increment = OperatorEnvelopeIncrement(rate[i]);

        if (from == to) {
            increment *= 0.6f;
            if (i == 0 && !level[i]) {
                increment *= 20.0f;
            }
        } else if (from < to) {
            from = std::max(6.7f, from);
            to = std::max(6.7f, to);
            if (from == to) {
                increment = 1.0f;
            } else {
                increment *= 7.2f / (to - from);
            }
        } else {
            increment *= 1.0f / (from - to);
        }
        increment_[i] = increment * scale_;
    }
}

void OperatorEnvelope::Reset() {
    float current = value();
    stage_ = 3;
    phase_ = 0.0f;
    start_ = current;
}

float OperatorEnvelope::value(int stage, float phase, float start) {
    float s = start < -50.0f ? level_[(stage - 1 + 4) % 4] : start;
    float target = level_[stage];
    phase = std::min(phase, 1.0f);

    if (s < target) {
        s = std::max(6.7f, s);
        target = std::max(6.7f, target);
        phase *= (2.5f - phase) * 0.666667f;
    }

    return phase * (target - s) + s;
}

float OperatorEnvelope::value() {
    return value(stage_, phase_, start_);
}

float OperatorEnvelope::Render(bool gate, float rate, float ad_scale, float r_scale) {
    if (gate) {
        if (stage_ == 3) {
            start_ = value();
            stage_ = 0;
            phase_ = 0.0f;
        }
    } else {
        if (stage_ != 3) {
            start_ = value();
            stage_ = 3;
            phase_ = 0.0f;
        }
    }

    phase_ += increment_[stage_] * rate * (stage_ == 3 ? r_scale : ad_scale);
    if (phase_ >= 1.0f) {
        if (stage_ >= 2) {
            phase_ = 1.0f;
        } else {
            phase_ = 0.0f;
            ++stage_;
            start_ = -100.0f;
        }
    }
    return value();
}

float OperatorEnvelope::RenderAtSample(float t, float gate_duration) {
    if (t > gate_duration) {
        const float phase = (t - gate_duration) * increment_[3];
        return phase >= 1.0f
            ? level_[3]
            : value(3, phase, RenderAtSample(gate_duration, gate_duration));
    }

    int stage = 0;
    for (; stage < 3; ++stage) {
        const float stage_duration = 1.0f / increment_[stage];
        if (t < stage_duration) {
            break;
        }
        t -= stage_duration;
    }

    if (stage == 3) {
        t -= gate_duration;
        if (t <= 0.0f) {
            return level_[2];
        } else if (t * increment_[3] > 1.0f) {
            return level_[3];
        }
    }
    return value(stage, t * increment_[stage], -100.0f);
}

void PitchEnvelope::Init(float scale) {
    scale_ = scale;
    stage_ = 3;
    phase_ = 1.0f;
    start_ = 0.0f;
    for (int i = 0; i < 4; ++i) {
        increment_[i] = 0.001f;
        level_[i] = 0.0f;
    }
}

void PitchEnvelope::Set(const uint8_t rate[4], const uint8_t level[4]) {
    for (int i = 0; i < 4; ++i) {
        level_[i] = PitchEnvelopeLevel(level[i]);
    }

    for (int i = 0; i < 4; ++i) {
        float from = level_[(i - 1 + 4) % 4];
        float to = level_[i];
        float increment = PitchEnvelopeIncrement(rate[i]);
        if (from != to) {
            increment *= 1.0f / std::fabs(from - to);
        } else if (i != 3) {
            increment = 0.2f;
        }
        increment_[i] = increment * scale_;
    }
}

void PitchEnvelope::Reset() {
    float current = value();
    stage_ = 3;
    phase_ = 0.0f;
    start_ = current;
}

float PitchEnvelope::value(int stage, float phase, float start) {
    float s = start < -50.0f ? level_[std::max(0, stage - 1)] : start;
    float target = level_[stage];
    phase = std::min(phase, 1.0f);
    return s + (target - s) * phase;
}

float PitchEnvelope::value() {
    return value(stage_, phase_, start_);
}

float PitchEnvelope::Render(bool gate, float rate, float ad_scale, float r_scale) {
    if (gate) {
        if (stage_ == 3) {
            start_ = value();
            stage_ = 0;
            phase_ = 0.0f;
        }
    } else {
        if (stage_ != 3) {
            start_ = value();
            stage_ = 3;
            phase_ = 0.0f;
        }
    }

    phase_ += increment_[stage_] * rate * (stage_ == 3 ? r_scale : ad_scale);
    if (phase_ >= 1.0f) {
        if (stage_ >= 2) {
            phase_ = 1.0f;
        } else {
            phase_ = 0.0f;
            ++stage_;
            start_ = -100.0f;
        }
    }
    return value();
}

float PitchEnvelope::RenderAtSample(float t, float gate_duration) {
    if (t > gate_duration) {
        const float phase = (t - gate_duration) * increment_[3];
        return phase >= 1.0f
            ? level_[3]
            : value(3, phase, RenderAtSample(gate_duration, gate_duration));
    }

    int stage = 0;
    for (; stage < 3; ++stage) {
        const float stage_duration = 1.0f / increment_[stage];
        if (t < stage_duration) {
            break;
        }
        t -= stage_duration;
    }

    if (stage == 3) {
        return level_[2];
    }
    return value(stage, t * increment_[stage], -100.0f);
}

static const float kMinLFOFrequency = 0.005865f;

static float LFOFrequency(int rate) {
    int rate_scaled = rate == 0 ? 1 : (rate * 165) >> 6;
    rate_scaled *= rate_scaled < 160 ? 11 : (11 + ((rate_scaled - 160) >> 4));
    return float(rate_scaled) * kMinLFOFrequency;
}

static void LFODelay(int delay, float increments[2]) {
    if (delay == 0) {
        increments[0] = increments[1] = 100000.0f;
    } else {
        int d = 99 - delay;
        d = (16 + (d & 15)) << (1 + (d >> 4));
        increments[0] = float(d) * kMinLFOFrequency;
        increments[1] = float(std::max(0x80, d & 0xff80)) * kMinLFOFrequency;
    }
}

void Lfo::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    phase_ = 0.0f;
    frequency_ = 1.0f;
    delay_phase_ = 0.0f;
    delay_increment_[0] = delay_increment_[1] = 100000.0f;
    pitch_mod_depth_ = 0.0f;
    amp_mod_depth_ = 0.0f;
    waveform_ = 0;
    reset_phase_ = false;
    random_value_ = 0.0f;
    pitch_mod_ = 0.0f;
    amp_mod_ = 0.0f;
}

void Lfo::Set(const Patch::ModulationParameters& mod) {
    frequency_ = LFOFrequency(mod.rate) / sample_rate_;
    LFODelay(mod.delay, delay_increment_);
    for (int i = 0; i < 2; ++i) {
        delay_increment_[i] /= sample_rate_;
    }
    int pms = std::min(std::max(int(mod.pitch_mod_sensitivity), 0), 7);
    pitch_mod_depth_ = float(mod.pitch_mod_depth) * 0.01f * lut_pitch_mod_sensitivity[pms];
    amp_mod_depth_ = float(mod.amp_mod_depth) * 0.01f;
    waveform_ = mod.waveform;
    reset_phase_ = mod.reset_phase != 0;
}

void Lfo::Reset() {
    if (reset_phase_) {
        phase_ = 0.0f;
    }
    delay_phase_ = 0.0f;
}

void Lfo::Step(float num_samples) {
    phase_ += frequency_ * num_samples;
    if (phase_ >= 1.0f) {
        phase_ -= 1.0f;
        random_value_ = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }

    delay_phase_ += num_samples * delay_increment_[(delay_phase_ < 0.5f) ? 0 : 1];
    if (delay_phase_ >= 1.0f) {
        delay_phase_ = 1.0f;
    }

    ComputeModValues();
}

void Lfo::ComputeModValues() {
    float value = 0.0f;
    switch (waveform_) {
        case 0:  // Triangle - peaks at 0.0 and 1.0 phase
            value = 2.0f * (phase_ < 0.5f ? 0.5f - phase_ : phase_ - 0.5f);
            break;
        case 1:  // Ramp down
            value = 1.0f - phase_;
            break;
        case 2:  // Ramp up
            value = phase_;
            break;
        case 3:  // Square
            value = phase_ < 0.5f ? 0.0f : 1.0f;
            break;
        case 4:  // Sine
            value = 0.5f + 0.5f * lut_sine[static_cast<size_t>(std::fmod(phase_ + 0.5f, 1.0f) * kSineLUTSize)];
            break;
        case 5:  // Sample & Hold
            value = random_value_;
            break;
    }

    float delay_ramp = delay_phase_ < 0.5f ? 0.0f : (delay_phase_ - 0.5f) * 2.0f;

    pitch_mod_ = (value - 0.5f) * delay_ramp * pitch_mod_depth_;
    amp_mod_ = (1.0f - value) * delay_ramp * amp_mod_depth_;
}

void Lfo::Scrub(float position) {
    float phase = position * frequency_;
    phase_ = std::fmod(phase, 1.0f);

    delay_phase_ = position * delay_increment_[0];
    if (delay_phase_ > 0.5f) {
        float remaining = position - 0.5f / delay_increment_[0];
        delay_phase_ = 0.5f + remaining * delay_increment_[1];
        if (delay_phase_ >= 1.0f) {
            delay_phase_ = 1.0f;
        }
    }

    float value = 0.0f;
    switch (waveform_) {
        case 0: value = 2.0f * (phase_ < 0.5f ? 0.5f - phase_ : phase_ - 0.5f); break;
        case 1: value = 1.0f - phase_; break;
        case 2: value = phase_; break;
        case 3: value = phase_ < 0.5f ? 0.0f : 1.0f; break;
        case 4: value = 0.5f + 0.5f * lut_sine[static_cast<size_t>(std::fmod(phase_ + 0.5f, 1.0f) * kSineLUTSize)]; break;
        default: value = 2.0f * (phase_ < 0.5f ? 0.5f - phase_ : phase_ - 0.5f); break;
    }

    float delay_ramp = delay_phase_ < 0.5f ? 0.0f : (delay_phase_ - 0.5f) * 2.0f;
    pitch_mod_ = (value - 0.5f) * delay_ramp * pitch_mod_depth_;
    amp_mod_ = (1.0f - value) * delay_ramp * amp_mod_depth_;
}

#define MOD(n) ((n) << 4)
#define ADD(n) ((n) | 0x04)
#define DST(n) (n)
#define FB_SRC 0x40
#define FB_DST MOD(3)
#define FB (FB_SRC | FB_DST)
#define NO_MOD MOD(0)
#define OUTPUT ADD(0)

// Op order: 6, 5, 4, 3, 2, 1
static const uint8_t kAlgorithmOpcodes[kNumAlgorithms][kNumOperators] = {
    // Algorithm 1
    { FB | DST(1), MOD(1) | DST(1), MOD(1) | DST(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | ADD(0) },
    // Algorithm 2
    { NO_MOD | DST(1), MOD(1) | DST(1), MOD(1) | DST(1), MOD(1) | OUTPUT, FB | DST(1), MOD(1) | ADD(0) },
    // Algorithm 3
    { FB | DST(1), MOD(1) | DST(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | DST(1), MOD(1) | ADD(0) },
    // Algorithm 4
    { FB_DST | NO_MOD | DST(1), MOD(1) | DST(1), FB_SRC | MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | DST(1), MOD(1) | ADD(0) },
    // Algorithm 5
    { FB | DST(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | ADD(0), NO_MOD | DST(1), MOD(1) | ADD(0) },
    // Algorithm 6
    { FB_DST | NO_MOD | DST(1), FB_SRC | MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | ADD(0), NO_MOD | DST(1), MOD(1) | ADD(0) },
    // Algorithm 7
    { FB | DST(1), MOD(1) | DST(1), NO_MOD | ADD(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | ADD(0) },
    // Algorithm 8
    { NO_MOD | DST(1), MOD(1) | DST(1), FB | ADD(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | ADD(0) },
    // Algorithm 9
    { NO_MOD | DST(1), MOD(1) | DST(1), NO_MOD | ADD(1), MOD(1) | OUTPUT, FB | DST(1), MOD(1) | ADD(0) },
    // Algorithm 10
    { NO_MOD | DST(1), NO_MOD | ADD(1), MOD(1) | OUTPUT, FB | DST(1), MOD(1) | DST(1), MOD(1) | ADD(0) },
    // Algorithm 11
    { FB | DST(1), NO_MOD | ADD(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | DST(1), MOD(1) | ADD(0) },
    // Algorithm 12
    { NO_MOD | DST(1), NO_MOD | ADD(1), NO_MOD | ADD(1), MOD(1) | OUTPUT, FB | DST(1), MOD(1) | ADD(0) },
    // Algorithm 13
    { FB | DST(1), NO_MOD | ADD(1), NO_MOD | ADD(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | ADD(0) },
    // Algorithm 14
    { FB | DST(1), NO_MOD | ADD(1), MOD(1) | DST(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | ADD(0) },
    // Algorithm 15
    { NO_MOD | DST(1), NO_MOD | ADD(1), MOD(1) | DST(1), MOD(1) | OUTPUT, FB | DST(1), MOD(1) | ADD(0) },
    // Algorithm 16
    { FB | DST(1), MOD(1) | DST(1), NO_MOD | DST(2), MOD(2) | ADD(1), NO_MOD | ADD(1), MOD(1) | OUTPUT },
    // Algorithm 17
    { NO_MOD | DST(1), MOD(1) | DST(1), NO_MOD | DST(2), MOD(2) | ADD(1), FB | ADD(1), MOD(1) | OUTPUT },
    // Algorithm 18
    { NO_MOD | DST(1), MOD(1) | DST(1), MOD(1) | DST(1), FB | ADD(1), NO_MOD | ADD(1), MOD(1) | OUTPUT },
    // Algorithm 19
    { FB | DST(1), MOD(1) | OUTPUT, MOD(1) | ADD(0), NO_MOD | DST(1), MOD(1) | DST(1), MOD(1) | ADD(0) },
    // Algorithm 20
    { NO_MOD | DST(1), NO_MOD | ADD(1), MOD(1) | OUTPUT, FB | DST(1), MOD(1) | ADD(0), MOD(1) | ADD(0) },
    // Algorithm 21
    { NO_MOD | DST(1), MOD(1) | OUTPUT, MOD(1) | ADD(0), FB | DST(1), MOD(1) | ADD(0), MOD(1) | ADD(0) },
    // Algorithm 22
    { FB | DST(1), MOD(1) | OUTPUT, MOD(1) | ADD(0), MOD(1) | ADD(0), NO_MOD | DST(1), MOD(1) | ADD(0) },
    // Algorithm 23
    { FB | DST(1), MOD(1) | OUTPUT, MOD(1) | ADD(0), NO_MOD | DST(1), MOD(1) | ADD(0), NO_MOD | ADD(0) },
    // Algorithm 24
    { FB | DST(1), MOD(1) | OUTPUT, MOD(1) | ADD(0), MOD(1) | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0) },
    // Algorithm 25
    { FB | DST(1), MOD(1) | OUTPUT, MOD(1) | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0) },
    // Algorithm 26
    { FB | DST(1), NO_MOD | ADD(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | ADD(0), NO_MOD | ADD(0) },
    // Algorithm 27
    { NO_MOD | DST(1), NO_MOD | ADD(1), MOD(1) | OUTPUT, FB | DST(1), MOD(1) | ADD(0), NO_MOD | ADD(0) },
    // Algorithm 28
    { NO_MOD | OUTPUT, FB | DST(1), MOD(1) | DST(1), MOD(1) | ADD(0), NO_MOD | DST(1), MOD(1) | ADD(0) },
    // Algorithm 29
    { FB | DST(1), MOD(1) | OUTPUT, NO_MOD | DST(1), MOD(1) | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0) },
    // Algorithm 30
    { NO_MOD | OUTPUT, FB | DST(1), MOD(1) | DST(1), MOD(1) | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0) },
    // Algorithm 31
    { FB | DST(1), MOD(1) | OUTPUT, NO_MOD | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0) },
    // Algorithm 32
    { FB | OUTPUT, NO_MOD | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0), NO_MOD | ADD(0) },
};

void AlgorithmRenderer::Init() {
}

bool AlgorithmRenderer::isCarrier(int algorithm, int op) const {
    algorithm = std::min(std::max(algorithm, 0), kNumAlgorithms - 1);
    uint8_t opcode = kAlgorithmOpcodes[algorithm][op];
    return (opcode & 0x03) == 0;
}

void AlgorithmRenderer::Render(
    int algorithm,
    Operator* ops,
    const float* frequencies,
    const float* amplitudes,
    float* feedback_state,
    int feedback_amount,
    float sample_rate,
    float* out,
    size_t size) {

    algorithm = std::min(std::max(algorithm, 0), kNumAlgorithms - 1);
    const uint8_t* opcodes = kAlgorithmOpcodes[algorithm];

    int effective_feedback = feedback_amount;
    if (algorithm == 31 && feedback_amount > 0) {
        effective_feedback = std::max(feedback_amount - 2, 0);
    }
    const float fb_scale = effective_feedback ? float(1 << effective_feedback) / 512.0f : 0.0f;

    uint32_t frequency[kNumOperators];
    float amplitude[kNumOperators];
    float amplitude_increment[kNumOperators];
    const float scale = 1.0f / float(size);

    for (int i = 0; i < kNumOperators; ++i) {
        frequency[i] = static_cast<uint32_t>(std::min(frequencies[i], 0.5f) * 4294967296.0f);
        amplitude[i] = ops[i].amplitude;
        amplitude_increment[i] = (std::min(amplitudes[i], 4.0f) - amplitude[i]) * scale;
    }

    float previous_0 = feedback_state[0];
    float previous_1 = feedback_state[1];

    for (size_t s = 0; s < size; ++s) {
        float buffer[3] = {0.0f, 0.0f, 0.0f};

        for (int i = 0; i < kNumOperators; ++i) {
            uint8_t opcode = opcodes[i];

            int source_mask = opcode & 0x30;
            float pm = 0.0f;

            if (source_mask == 0x30) {
                pm = (previous_0 + previous_1) * fb_scale;
            } else if (source_mask > 0) {
                pm = buffer[source_mask >> 4];
            }

            ops[i].phase += frequency[i];
            float op_out = SinePM(ops[i].phase, pm) * amplitude[i];
            amplitude[i] += amplitude_increment[i];

            if (opcode & FB_SRC) {
                previous_1 = previous_0;
                previous_0 = op_out;
            }

            int dest = opcode & 0x03;
            if (opcode & 0x04) {
                buffer[dest] += op_out;
            } else {
                buffer[dest] = op_out;
            }
        }

        out[s] = buffer[0];
    }

    for (int i = 0; i < kNumOperators; ++i) {
        ops[i].amplitude = amplitude[i];
    }
    feedback_state[0] = previous_0;
    feedback_state[1] = previous_1;
}

void FMVoice::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    one_hz_ = 1.0f / sample_rate;
    a0_ = 55.0f / sample_rate;

    const float native_sr = 44100.0f;
    const float envelope_scale = native_sr / sample_rate;

    for (int i = 0; i < kNumOperators; ++i) {
        operators_[i].Reset();
        op_envelopes_[i].Init(envelope_scale);
    }
    pitch_envelope_.Init(envelope_scale);
    lfo_.Init(sample_rate);

    feedback_state_[0] = feedback_state_[1] = 0.0f;

    patch_ = nullptr;
    dirty_ = true;
    gate_ = false;
    note_ = 48.0f;
    normalized_velocity_ = 10.0f;

    algorithm_renderer_.Init();
}

void FMVoice::SetPatch(const Patch* patch) {
    if (patch == patch_) return;
    patch_ = patch;
    dirty_ = true;

    if (patch_) {
        lfo_.Set(patch_->modulations);
    }
}

void FMVoice::Retrigger() {
    Setup();

    for (int i = 0; i < kNumOperators; ++i) {
        op_envelopes_[i].Reset();
    }
    pitch_envelope_.Reset();

    gate_ = false;
}

void FMVoice::Setup() {
    if (!dirty_ || !patch_) return;

    pitch_envelope_.Set(patch_->pitch_envelope.rate, patch_->pitch_envelope.level);

    for (int i = 0; i < kNumOperators; ++i) {
        const Patch::Operator& op = patch_->op[i];

        int level = OperatorLevel(op.level);
        op_envelopes_[i].Set(op.envelope.rate, op.envelope.level, level);

        level_headroom_[i] = float(127 - level);

        float sign = op.mode == 0 ? 1.0f : -1.0f;
        ratios_[i] = sign * FrequencyRatio(op);
    }
    dirty_ = false;
}

void FMVoice::Render(
    bool sustain,
    bool gate,
    float note,
    float velocity,
    float brightness,
    float envelope_control,
    float pitch_mod,
    float amp_mod,
    float* out,
    size_t size) {

    if (!patch_) {
        std::fill(out, out + size, 0.0f);
        return;
    }

    Setup();

    const float envelope_rate = float(size);
    const float ad_scale = Pow2Fast<1>((0.5f - envelope_control) * 8.0f);
    const float r_scale = Pow2Fast<1>(-std::abs(envelope_control - 0.3f) * 8.0f);
    const float gate_duration = 1.5f * sample_rate_;
    const float envelope_sample = gate_duration * envelope_control;

    const float pitch_envelope = sustain
        ? pitch_envelope_.RenderAtSample(envelope_sample, gate_duration)
        : pitch_envelope_.Render(gate, envelope_rate, ad_scale, r_scale);

    const float total_pitch_mod = pitch_envelope + pitch_mod;
    const float f0 = a0_ * 0.25f * SemitonesToRatio(note - 9.0f + total_pitch_mod * 12.0f);

    const bool note_on = gate && !gate_;
    gate_ = gate;
    if (note_on || sustain) {
        normalized_velocity_ = NormalizeVelocity(velocity);
        note_ = note;
    }

    if (note_on && patch_->reset_phase) {
        for (int i = 0; i < kNumOperators; ++i) {
            operators_[i].phase = 0;
        }
    }

    float f[kNumOperators];
    float a[kNumOperators];

    for (int i = 0; i < kNumOperators; ++i) {
        const Patch::Operator& op = patch_->op[i];

        f[i] = ratios_[i] * (ratios_[i] < 0.0f ? -one_hz_ : f0);

        const float rate_scaling = RateScaling(note_, op.rate_scaling);
        float level = sustain
            ? op_envelopes_[i].RenderAtSample(envelope_sample, gate_duration)
            : op_envelopes_[i].Render(gate, envelope_rate * rate_scaling, ad_scale, r_scale);

        const float kb_scaling = KeyboardScaling(note_, op.keyboard_scaling);
        const float velocity_scaling = normalized_velocity_ * float(op.velocity_sensitivity);

        bool is_modulator = !algorithm_renderer_.isCarrier(patch_->algorithm, i);
        float brightness_mod = is_modulator ? (brightness - 0.5f) * 32.0f : 0.0f;

        level += 0.125f * std::min(kb_scaling + velocity_scaling + brightness_mod, level_headroom_[i]);

        const float sensitivity = AmpModSensitivity(op.amp_mod_sensitivity);
        const float log_level_mod = sensitivity * amp_mod - 1.0f;
        const float level_mod = 1.0f - Pow2Fast<2>(6.4f * log_level_mod);
        a[i] = Pow2Fast<2>(-14.0f + level * level_mod);
    }

    algorithm_renderer_.Render(
        patch_->algorithm,
        operators_,
        f,
        a,
        feedback_state_,
        patch_->feedback,
        sample_rate_,
        out,
        size);
}

PresetBank::PresetBank() : loaded_(false) {
    Clear();
}

void PresetBank::Clear() {
    for (int i = 0; i < kNumPatches; ++i) {
        patches_[i].Clear();
    }
    loaded_ = false;
}

bool PresetBank::LoadSysEx(const uint8_t* data, size_t size) {
    if (size < 4104) return false;

    if (data[0] != 0xF0 || data[1] != 0x43) {
        return false;
    }
    if (data[3] != 0x09) {
        return false;
    }

    return LoadPackedBank(data + 6, 4096);
}

bool PresetBank::LoadPackedBank(const uint8_t* data, size_t size) {
    size_t offset = 0;
    if (size >= 4104 && data[0] == 0xF0 && data[1] == 0x43) {
        offset = 6;
        size -= 6;
    }

    if (size < kNumPatches * kPatchSize) return false;

    for (int i = 0; i < kNumPatches; ++i) {
        patches_[i].Unpack(data + offset + i * kPatchSize);
    }
    loaded_ = true;
    return true;
}

const Patch& PresetBank::GetPatch(int index) const {
    index = std::min(std::max(index, 0), kNumPatches - 1);
    return patches_[index];
}

DXPlayEngine::DXPlayEngine() : active_voice_(kNumVoices - 1), rendered_voice_(0) {
    std::memset(acc_buffer_, 0, sizeof(acc_buffer_));
    std::memset(gate_samples_, 0, sizeof(gate_samples_));
}

void DXPlayEngine::Init(float sample_rate) {
    sample_rate_ = sample_rate;
    for (int i = 0; i < kNumVoices; ++i) {
        voices_[i].Init(sample_rate);
        gate_samples_[i] = 0;
    }
    active_voice_ = kNumVoices - 1;
    rendered_voice_ = 0;

    const float cutoff_hz = 16000.0f;
    lpf_coeff_ = 1.0f - std::exp(-2.0f * 3.14159265f * cutoff_hz / sample_rate);
    lpf_state_ = 0.0f;
}

void DXPlayEngine::LoadBank(const uint8_t* data, size_t size) {
    if (!bank_.LoadSysEx(data, size)) {
        bank_.LoadPackedBank(data, size);
    }
}

inline float DXSoftClip(float x) {
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

void DXPlayEngine::Render(
    float freq_hz,
    bool trig_connected,
    bool trig_high,
    bool trig_rising,
    float velocity,
    float timbre,
    float morph,
    int preset,
    float* out,
    size_t size) {

    const Patch& patch = bank_.GetPatch(preset);

    float note = 12.0f * std::log2(freq_hz / 440.0f) + 69.0f;

    if (!trig_connected) {
        voices_[0].SetPatch(&patch);
        voices_[0].lfo().Scrub(2.0f * sample_rate_ * morph);

        voices_[0].Render(
            true,
            true,
            note,
            velocity,
            timbre,
            morph,
            voices_[0].lfo().pitch_mod(),
            voices_[0].lfo().amp_mod(),
            out,
            size);
    } else {

        if (trig_rising) {
            active_voice_ = (active_voice_ + 1) % kNumVoices;
            voices_[active_voice_].lfo().Reset();
            voices_[active_voice_].Retrigger();
            gate_samples_[active_voice_] = static_cast<int>(sample_rate_ * 0.1f);
        }

        for (int i = 0; i < kNumVoices; ++i) {
            voices_[i].SetPatch(&patch);
        }

        voices_[active_voice_].lfo().Step(float(size));

        float pitch_mod = voices_[active_voice_].lfo().pitch_mod();
        float amp_mod = voices_[active_voice_].lfo().amp_mod();

        float temp_buffer[2048];
        std::fill(&temp_buffer[0], &temp_buffer[size], 0.0f);

        for (int i = 0; i < kNumVoices; ++i) {
            bool voice_gate = (i == active_voice_) && 
                              (trig_high || (gate_samples_[i] > 0));

            if (gate_samples_[i] > 0) {
                gate_samples_[i] -= size;
            }

            float voice_buffer[2048];
            voices_[i].Render(
                false,
                voice_gate,
                note,
                velocity,
                timbre,
                morph,
                pitch_mod,
                amp_mod,
                voice_buffer,
                size);

            for (size_t j = 0; j < size; ++j) {
                temp_buffer[j] += voice_buffer[j];
            }
        }

        for (size_t i = 0; i < size; ++i) {
            out[i] = temp_buffer[i];
        }
    }

    for (size_t i = 0; i < size; ++i) {
        float x = out[i] * 0.25f;
        if (x < -3.0f) x = -1.0f;
        else if (x > 3.0f) x = 1.0f;
        else x = x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
        out[i] = x;
    }
}

}  // namespace DXPlay
