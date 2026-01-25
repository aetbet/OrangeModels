SQArpUGen : MultiOutUGen {
    *ar { |freq = 440.0, trig = 0, envelope = 0.0, pattern = 0.5, shape = 0.5, chord = 0.0, bank = 0, mul = 1.0, add = 0.0|
        ^this.multiNew('audio', freq, trig, envelope, pattern, shape, chord, bank).madd(mul, add);
    }

    *kr { |freq = 440.0, trig = 0, envelope = 0.0, pattern = 0.5, shape = 0.5, chord = 0.0, bank = 0, mul = 1.0, add = 0.0|
        ^this.multiNew('control', freq, trig, envelope, pattern, shape, chord, bank).madd(mul, add);
    }

    *chords {
        "
=== SQArp Chord Banks ===

Original (bank: 0)
  OCT ------------------- 0.00 - 0.09
  5 --------------------- 0.10 - 0.18
  sus4 ------------------ 0.19 - 0.27
  m --------------------- 0.28 - 0.36
  m7 -------------------- 0.37 - 0.45
  m9 -------------------- 0.46 - 0.54
  m11 ------------------- 0.55 - 0.63
  69 -------------------- 0.64 - 0.71
  M9 -------------------- 0.72 - 0.80
  M7 -------------------- 0.81 - 0.89
  M --------------------- 0.90 - 1.00

Jon Butler (bank: 1)
  Octave ---------------- 0.00 - 0.06
  Fifth ----------------- 0.07 - 0.11
  Minor ----------------- 0.12 - 0.17
  Minor 7th ------------- 0.18 - 0.23
  Minor 9th ------------- 0.24 - 0.29
  Minor 11th ------------ 0.30 - 0.35
  Major ----------------- 0.36 - 0.40
  Major 7th ------------- 0.41 - 0.46
  Major 9th ------------- 0.47 - 0.52
  Sus4 ------------------ 0.53 - 0.58
  69 -------------------- 0.59 - 0.63
  6th ------------------- 0.64 - 0.69
  10th (Spread maj7) ---- 0.70 - 0.75
  Dominant 7th ---------- 0.76 - 0.81
  Dominant 7th (b9) ----- 0.82 - 0.86
  Half Diminished ------- 0.87 - 0.92
  Fully Diminished ------ 0.93 - 1.00

Joe McMullen (bank: 2)
  iv 6/9 ---------------- 0.00 - 0.05
  iio 7sus4 ------------- 0.06 - 0.11
  VII 6 ----------------- 0.12 - 0.16
  v m11 ----------------- 0.17 - 0.22
  III add4 -------------- 0.23 - 0.27
  i addb13 -------------- 0.28 - 0.33
  VI add#11 ------------- 0.34 - 0.38
  iv m6 ----------------- 0.39 - 0.43
  iio ------------------- 0.44 - 0.49
  viio ------------------ 0.50 - 0.54
  V 7 ------------------- 0.55 - 0.60
  iii addb9 ------------- 0.61 - 0.65
  I maj7 ---------------- 0.66 - 0.71
  vi m9 ----------------- 0.72 - 0.76
  IV maj9 --------------- 0.77 - 0.82
  ii m7 ----------------- 0.83 - 0.87
  I maj7sus4/vii -------- 0.88 - 0.93
  V 7sus4 --------------- 0.94 - 1.00
".postln;
    }

    *patterns {
        "
=== SQArp Patterns ===

Up, 1 octave --------- 0.00 - 0.08
Up, 2 octaves -------- 0.09 - 0.17
Up, 4 octaves -------- 0.18 - 0.25
Down, 1 octave ------- 0.26 - 0.33
Down, 2 octaves ------ 0.34 - 0.42
Down, 4 octaves ------ 0.43 - 0.50
Up-Down, 1 octave ---- 0.51 - 0.58
Up-Down, 2 octaves --- 0.59 - 0.67
Up-Down, 4 octaves --- 0.68 - 0.75
Random, 1 octave ----- 0.76 - 0.83
Random, 2 octaves ---- 0.84 - 0.92
Random, 4 octaves ---- 0.93 - 1.00
".postln;
    }

    init { |... theInputs|
        inputs = theInputs;
        ^this.initOutputs(2, rate);
    }

    checkInputs {
        ^this.checkValidInputs;
    }
}

SQArp {
    *ar { |freq = 440.0, trig = 0, envelope = 0.0, pattern = 0.5, shape = 0.5, chord = 0.0, bank = 0, mul = 1.0, add = 0.0|
        var freqArr, trigArr, envArr, patArr, shapeArr, chordArr, bankArr, mulArr, addArr;
        var numChannels, results;

        freqArr = if(freq.isArray) { freq } { [freq] };
        trigArr = if(trig.isArray) { trig } { [trig] };
        envArr = if(envelope.isArray) { envelope } { [envelope] };
        patArr = if(pattern.isArray) { pattern } { [pattern] };
        shapeArr = if(shape.isArray) { shape } { [shape] };
        chordArr = if(chord.isArray) { chord } { [chord] };
        bankArr = if(bank.isArray) { bank } { [bank] };
        mulArr = if(mul.isArray) { mul } { [mul] };
        addArr = if(add.isArray) { add } { [add] };

        numChannels = [freqArr, trigArr, envArr, patArr, shapeArr, chordArr, bankArr, mulArr, addArr]
            .collect(_.size).maxItem;

        results = numChannels.collect { |i|
            SQArpUGen.ar(
                freqArr.wrapAt(i),
                trigArr.wrapAt(i),
                envArr.wrapAt(i),
                patArr.wrapAt(i),
                shapeArr.wrapAt(i),
                chordArr.wrapAt(i),
                bankArr.wrapAt(i),
                mulArr.wrapAt(i),
                addArr.wrapAt(i)
            )
        };

        if(numChannels == 1) {
            ^results[0]
        };

        ^[
            results.collect { |r| r[0] },
            results.collect { |r| r[1] }
        ]
    }

    *kr { |freq = 440.0, trig = 0, envelope = 0.0, pattern = 0.5, shape = 0.5, chord = 0.0, bank = 0, mul = 1.0, add = 0.0|
        var freqArr, trigArr, envArr, patArr, shapeArr, chordArr, bankArr, mulArr, addArr;
        var numChannels, results;

        freqArr = if(freq.isArray) { freq } { [freq] };
        trigArr = if(trig.isArray) { trig } { [trig] };
        envArr = if(envelope.isArray) { envelope } { [envelope] };
        patArr = if(pattern.isArray) { pattern } { [pattern] };
        shapeArr = if(shape.isArray) { shape } { [shape] };
        chordArr = if(chord.isArray) { chord } { [chord] };
        bankArr = if(bank.isArray) { bank } { [bank] };
        mulArr = if(mul.isArray) { mul } { [mul] };
        addArr = if(add.isArray) { add } { [add] };

        numChannels = [freqArr, trigArr, envArr, patArr, shapeArr, chordArr, bankArr, mulArr, addArr]
            .collect(_.size).maxItem;

        results = numChannels.collect { |i|
            SQArpUGen.kr(
                freqArr.wrapAt(i),
                trigArr.wrapAt(i),
                envArr.wrapAt(i),
                patArr.wrapAt(i),
                shapeArr.wrapAt(i),
                chordArr.wrapAt(i),
                bankArr.wrapAt(i),
                mulArr.wrapAt(i),
                addArr.wrapAt(i)
            )
        };

        if(numChannels == 1) {
            ^results[0]
        };

        ^[
            results.collect { |r| r[0] },
            results.collect { |r| r[1] }
        ]
    }

    *chords {
        SQArpUGen.chords;
    }

    *patterns {
        SQArpUGen.patterns;
    }
}
