#include "SC_PlugIn.hpp"
#include "SMEmu.hpp"

static InterfaceTable* ft;

class SMEmuUGen : public SCUnit {
public:
    SMEmuUGen();
    ~SMEmuUGen();

private:
    void next(int nSamples);

    SMEmu::StringMachineEngine engine_;
    float* aux_buffer_;
};

SMEmuUGen::SMEmuUGen() {
    engine_.Init(static_cast<float>(sampleRate()));

    aux_buffer_ = (float*)RTAlloc(mWorld, sizeof(float) * bufferSize());
    if (!aux_buffer_) {
        Print("SMEmu: Could not allocate aux buffer\n");
    }

    mCalcFunc = make_calc_function<SMEmuUGen, &SMEmuUGen::next>();
    next(1);
}

SMEmuUGen::~SMEmuUGen() {
    if (aux_buffer_) {
        RTFree(mWorld, aux_buffer_);
    }
}

void SMEmuUGen::next(int nSamples) {

    const float* freq_in = in(0);
    const float* fx_in = in(1);
    const float* wave_in = in(2);
    const float* chord_in = in(3);
    const float* bank_in = in(4);

    float* out_main = out(0);
    float* out_aux = numOutputs() > 1 ? out(1) : aux_buffer_;

    float freq = freq_in[0];
    float fx = std::min(std::max(fx_in[0], 0.0f), 1.0f);
    float wave = std::min(std::max(wave_in[0], 0.0f), 1.0f);
    float chord = std::min(std::max(chord_in[0], 0.0f), 1.0f);
    int bank = static_cast<int>(bank_in[0]);
    bank = std::max(0, std::min(bank, 2));

    engine_.Render(
        freq,
        fx,
        wave,
        chord,
        bank,
        out_main,
        out_aux,
        nSamples);
}

PluginLoad(SMEmuUGens) {
    ft = inTable;
    registerUnit<SMEmuUGen>(ft, "SMEmuUGen", false);
}
