#include "SC_PlugIn.hpp"
#include "SQArp.hpp"

static InterfaceTable* ft;

class SQArpUGen : public SCUnit {
public:
    SQArpUGen() {
        engine_.Init(sampleRate());

        trigger_patched_ = (inRate(1) != calc_ScalarRate);

        int blockSize = bufferSize();
        out_main_ = new float[blockSize]();
        out_aux_ = new float[blockSize]();

        mCalcFunc = make_calc_function<SQArpUGen, &SQArpUGen::next>();
        next(1);
    }

    ~SQArpUGen() {
        delete[] out_main_;
        delete[] out_aux_;
    }

private:
    void next(int nSamples) {
        const float* freqIn = in(0);
        const float* trigIn = in(1);
        const float* envelopeIn = in(2);
        const float* patternIn = in(3);
        const float* shapeIn = in(4);
        const float* chordIn = in(5);
        const float* bankIn = in(6);

        float* outMain = out(0);
        float* outAux = out(1);

        float freq = freqIn[0];

        bool trig = false;
        for (int i = 0; i < nSamples; ++i) {
            if (trigIn[i] > 0.5f) {
                trig = true;
                break;
            }
        }

        float envelope = envelopeIn[0];
        float pattern = patternIn[0];
        float shape = shapeIn[0];
        float chord = chordIn[0];
        int bank = static_cast<int>(bankIn[0]);

        freq = std::min(std::max(freq, 1.0f), 20000.0f);
        envelope = std::min(std::max(envelope, -1.0f), 1.0f);
        pattern = std::min(std::max(pattern, 0.0f), 1.0f);
        shape = std::min(std::max(shape, 0.0f), 1.0f);
        chord = std::min(std::max(chord, 0.0f), 1.0f);
        bank = std::min(std::max(bank, 0), 2);

        engine_.Render(freq, trigger_patched_, trig, envelope, pattern, shape, chord, bank,
                       out_main_, out_aux_, nSamples);

        for (int i = 0; i < nSamples; ++i) {
            outMain[i] = out_main_[i];
            outAux[i] = out_aux_[i];
        }
    }

    SQArp::ChiptuneEngine engine_;
    float* out_main_;
    float* out_aux_;
    bool trigger_patched_;
};

PluginLoad(SQArpUGens) {
    ft = inTable;
    registerUnit<SQArpUGen>(ft, "SQArpUGen", false);
}
