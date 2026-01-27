# OrangeModels

SuperCollider UGens based on some of the models from Mutable Instruments Plaits Orange Mode ([Firmware v1.2](https://pichenettes.github.io/mutable-instruments-documentation/modules/plaits/firmware/)).

Original code by: Emilie Gillet (https://github.com/pichenettes/eurorack.git)

### -- DXPlay --
Orange Model #3~5: 2-voice, 6-operator FM synth with 32 presets.

This model plays a DX7 (and the 6-OP family) preset in a Eurorack way.

Plaits built-in banks are in a `syx` folder.

The bank editor from Emilie is [here](https://pichenettes.github.io/plaits-editor/), for creating the custom bank.

### -- SMEmu --
Orange Model #7: String machine emulation with stereo filter and chorus.

### -- SQArp --
Orange Model #8: Four variable square voices for chords or arpeggios.

## Notes
- Additional chord tables (by Jon Butler and Joe McMullen) from an [alternative firmware](https://github.com/lylepmills/eurorack/tree/master/alt_firmwares) are included.
- Internal Low-Pass Gate and Decay Envelope are excluded. Apply them externally.
- According to [Dexed](https://github.com/asb2m10/dexed.git) Mk I engine, the feedback value for algorithm 32 is adjusted in DXPlay.
- Original Plaits runs at 48kHz. And VCV Rack Plaits does sample rate conversion to make the signal 48kHz internally. UGens here are working at the SuperCollider server sample rate, so their signal would match best at 48kHz. Usually no need to care about this.

## Installation
- Download the pre-build binary from the [release page](https://github.com/aetbet/OrangeModels/releases).
- Extract the archive and place the contents in the SuperCollider Extensions folder.

**The macOS binary is not signed**, so it may be necessary to remove the quarantine attribute.
```bash
xattr -rd com.apple.quarantine /path/to/OrangeModels/
```
## Building
- Get [the SuperCollider source code](https://github.com/supercollider/supercollider).
- Clone this repository.
```bash
git clone https://github.com/aetbet/OrangeModels.git
```
- Build.
```bash
cd OrangeModels
mkdir build && cd build
cmake .. -DSC_PATH="/path/to/SC/sources/" -DCMAKE_BUILD_TYPE="Release"
make install
```
For example, `-DSC_PATH="/usr/local/include/supercollider"`, the folder `OrangeModels` would be created in the `build` folder. Specify an install directory with `-DCMAKE_INSTALL_PREFIX="/path/to/install/"` if necessary.