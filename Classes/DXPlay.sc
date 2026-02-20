DXPlay {
    classvar <all;  // Dictionary of loaded banks

    var <key, <buffer, <path, <names, <transposes;

    *initClass {
        all = IdentityDictionary.new;
    }

    *new { |key|
        ^all[key] ?? { Error("DXPlay: bank '%' not loaded. Use DXPlay.load first.".format(key)).throw }
    }

    *load { |key, path, server, action|
        var instance;
        server = server ?? Server.default;
        instance = super.new.prLoad(key, path, server, action);
        all[key] = instance;
        ^instance
    }

    *free { |key|
        all[key] !? { |bank| bank.free };
        all[key] = nil;
    }

    *freeAll {
        all.do(_.free);
        all.clear;
    }

    prLoad { |argKey, argPath, server, action|
        var file, bytes, floats;

        key = argKey;
        path = argPath;
        names = Array.newClear(32);
        transposes = Array.newClear(32);

        // Read SysEx file
        file = File(path, "rb");
        if(file.isOpen.not) {
            Error("DXPlay: Could not open file: %".format(path)).throw;
        };

        bytes = Int8Array.newClear(file.length);
        file.read(bytes);
        file.close;

        // Parse preset names from SysEx data
        this.prParseNames(bytes);

        // Convert to float array for SC buffer
        floats = FloatArray.newClear(bytes.size);
        bytes.do { |byte, i|
            floats[i] = byte.asInteger.bitAnd(0xFF).asFloat
        };

        // Load data into buffer using loadCollection directly (creates buffer)
        buffer = Buffer.loadCollection(server, floats, 1, { |buf|
            action.value(this);
            "DXPlay: loaded '%' from %".format(key, path).postln;
        });
    }

    prParseNames { |bytes|
        var offset;

        // Check for SysEx header (F0 43 00 09)
        // Use unsigned masking because Int8Array entries may be signed (-128..127)
        offset = if((bytes[0].asInteger.bitAnd(0xFF) == 0xF0) and: { bytes[1].asInteger.bitAnd(0xFF) == 0x43 }) { 6 } { 0 };

        // Each voice is 128 bytes, name is at offset 118-127 (10 chars)
        32.do { |i|
            var voiceOffset = offset + (i * 128);
            var nameOffset = voiceOffset + 118;
            var nameArray = Array.fill(10, { |j|
                var char = bytes[nameOffset + j].asInteger.bitAnd(0x7F);
                // Replace all control characters (0-31) and DEL (127) with space
                if((char < 32) or: { char == 127 }) { 32 } { char }
            });
            var name = nameArray.collect(_.asAscii).join;
            names[i] = name.trim; // Remove leading/trailing spaces
            // Transpose byte is at offset 117 within each 128-byte voice.
            // Store as semitone offset: raw 0..48 -> -24..+24
            transposes[i] = ((bytes[voiceOffset + 117].asInteger.bitAnd(0x7F)).clip(0, 48) - 24);
        };
    }

    describe {
        "DXPlay bank: '%'".format(key).postln;
        "Path: %".format(path).postln;
        "Presets:".postln;
        names.do { |name, i|
            "  %: %".format(i.asString.padLeft(2, " "), name).postln;
        };
        ^this
    }

    presetName { |index|
        ^names[index.clip(0, 31)]
    }

    transpose { |index|
        ^transposes[index.clip(0, 31)]
    }

    bufnum {
        ^buffer !? { buffer.bufnum } ?? { -1 }
    }

    free {
        buffer !? { buffer.free };
        buffer = nil;
        transposes = nil;
        all[key] = nil;
    }

    ar { |freq = 440.0, trig = 0, velocity = 0.6, timbre = 0.5, morph = 0.5, preset = 0, mul = 1.0, add = 0.0|
        ^DXPlayUGen.ar(this.bufnum, freq, trig, velocity, timbre, morph, preset, mul, add)
    }

    kr { |freq = 440.0, trig = 0, velocity = 0.6, timbre = 0.5, morph = 0.5, preset = 0, mul = 1.0, add = 0.0|
        ^DXPlayUGen.kr(this.bufnum, freq, trig, velocity, timbre, morph, preset, mul, add)
    }
}

DXPlayUGen : UGen {
    *ar { |bufnum, freq = 440.0, trig = 0, velocity = 0.6, timbre = 0.5, morph = 0.5, preset = 0, mul = 1.0, add = 0.0|
        ^this.multiNew('audio', bufnum, freq, trig, velocity, timbre, morph, preset).madd(mul, add)
    }

    *kr { |bufnum, freq = 440.0, trig = 0, velocity = 0.6, timbre = 0.5, morph = 0.5, preset = 0, mul = 1.0, add = 0.0|
        ^this.multiNew('control', bufnum, freq, trig, velocity, timbre, morph, preset).madd(mul, add)
    }

    checkInputs {
        ^this.checkValidInputs
    }
}
