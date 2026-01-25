#include "SC_PlugIn.hpp"
#include "DXPlay.hpp"
#include <vector>

static InterfaceTable* ft;

struct DXPlayUGen : public SCUnit {
public:
    DXPlayUGen();
    ~DXPlayUGen();

private:
    void next(int nSamples);

    DXPlay::DXPlayEngine engine_;
    bool trigger_state_;
    bool prev_trigger_state_;
    bool trigger_patched_;
    bool bank_loaded_;

    float* syx_buffer_;
    int syx_bufnum_;
};

DXPlayUGen::DXPlayUGen()
    : trigger_state_(false)
    , prev_trigger_state_(false)
    , trigger_patched_(false)
    , bank_loaded_(false)
    , syx_buffer_(nullptr)
    , syx_bufnum_(-1) {

    static bool lut_initialized = false;
    if (!lut_initialized) {
        DXPlay::InitSineLUT();
        lut_initialized = true;
    }

    trigger_patched_ = (inRate(2) != calc_ScalarRate);

    engine_.Init(sampleRate());

    mCalcFunc = make_calc_function<DXPlayUGen, &DXPlayUGen::next>();
    next(1);
}

DXPlayUGen::~DXPlayUGen() {
}

void DXPlayUGen::next(int nSamples) {
    int bufnum = static_cast<int>(in0(0));
    const float* freq_in = in(1);
    const float* trig_in = in(2);
    const float* velocity_in = in(3);
    const float* timbre_in = in(4);
    const float* morph_in = in(5);
    int preset = static_cast<int>(in0(6));

    float* outbuf = out(0);

    if (bufnum >= 0 && !bank_loaded_) {
        World* world = mWorld;
        SndBuf* buf = World_GetBuf(world, bufnum);
        if (buf && buf->data && buf->frames >= 4096) {
            if (buf->data[0] > 200.0f) {
                std::vector<uint8_t> syx_data(buf->frames);
                for (int i = 0; i < buf->frames; ++i) {
                    syx_data[i] = static_cast<uint8_t>(buf->data[i]);
                }
                engine_.LoadBank(syx_data.data(), syx_data.size());
                bank_loaded_ = true;
                syx_bufnum_ = bufnum;
            }
        }
    }

    if (!bank_loaded_) {
        std::fill(outbuf, outbuf + nSamples, 0.0f);
        return;
    }

    float freq = freq_in[0];
    float timbre = std::min(std::max(timbre_in[0], 0.0f), 1.0f);
    float morph = std::min(std::max(morph_in[0], 0.0f), 1.0f);

    bool trig_rising = false;
    float velocity = 0.0f;
    bool velocity_is_audio = (inRate(3) == calc_FullRate);

    if (trigger_patched_) {
        prev_trigger_state_ = trigger_state_;

        float trigger_value = 0.0f;
        if (inRate(2) == calc_FullRate) {
            for (int i = 0; i < nSamples; ++i) {
                trigger_value += trig_in[i];
                if (trig_in[i] > 0.3f && velocity == 0.0f) {
                    velocity = velocity_is_audio ? velocity_in[i] : velocity_in[0];
                }
            }
            if (velocity == 0.0f) {
                velocity = velocity_in[0];
            }
        } else {
            trigger_value = trig_in[0];
            velocity = velocity_in[0];
        }

        velocity = std::min(std::max(velocity, 0.0f) * (2.0f / 3.0f), 0.6f);

        if (!trigger_state_) {
            if (trigger_value > 0.3f) {
                trigger_state_ = true;
            }
        } else {
            if (trigger_value < 0.1f) {
                trigger_state_ = false;
            }
        }

        trig_rising = trigger_state_ && !prev_trigger_state_;
    } else {
        velocity = std::min(std::max(velocity_in[0], 0.0f) * (2.0f / 3.0f), 0.6f);
    }

    engine_.Render(
        freq,
        trigger_patched_,
        trigger_state_,
        trig_rising,
        velocity,
        timbre,
        morph,
        preset,
        outbuf,
        nSamples);
}

PluginLoad(DXPlayUGens) {
    ft = inTable;
    registerUnit<DXPlayUGen>(ft, "DXPlayUGen", false);
}
