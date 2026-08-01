# Delay-and-Sum Beamformer

A known-direction delay-and-sum beamformer in C++20. Given a multichannel recording and
the known geometry of the array and target source, it time-aligns the channels and sums
them: the target adds coherently while reverberation and off-axis interference do not.

## Pipeline

1. **Scene generation (Python).** `pyroomacoustics` simulates a 10x10x10 m room
   (RT60 target 0.5 s, measured 0.377 s) containing a 4-microphone array, a target
   speaker and an interfering source, and writes a 4-channel WAV at 16 kHz.
2. **Beamforming (C++).** Reads the WAV with `dr_wav`, de-interleaves the channels,
   applies per-channel integer sample delays derived from the array geometry, sums and
   averages across channels, and writes a mono WAV.

## Scene and array geometry

| | Position (x, y, z) m |
|---|---|
| Target source | 5.00, 5.00, 5.00 |
| Interfering source | 5.00, 9.00, 2.00 |
| Mic 1 | 2.00, 2.00, 2.00 |
| Mic 2 | 2.00, 2.05, 2.00 |
| Mic 3 | 2.00, 2.10, 2.00 |
| Mic 4 | 2.00, 2.15, 2.00 |

A uniform linear array of 4 microphones spaced 50 mm apart along the y axis. The target
sits roughly 5.2 m away, so the array is used in the near field and delays are computed
from true source-to-microphone distances rather than a far-field plane-wave
approximation.

For each microphone, time of arrival is `distance / c` with `c = 343 m/s`. Mic 4 is
nearest, so it is taken as the reference with zero delay; every other channel is shifted
forward by its time difference of arrival relative to mic 4, converted to samples by
multiplying by the 16 kHz sample rate and rounding to the nearest integer.

| Mic | Distance | TOA | TDOA vs mic 4 | Delay (samples) |
|---|---|---|---|---|
| 1 | 5.196 m | 15.149 ms | 0.248 ms | 4 |
| 2 | 5.167 m | 15.066 ms | 0.165 ms | 3 |
| 3 | 5.139 m | 14.983 ms | 0.082 ms | 1 |
| 4 | 5.111 m | 14.901 ms | 0.000 ms | 0 |

The delays are small because the array is short relative to the source distance: 50 mm
of spacing at 343 m/s is well under one sample period at 16 kHz for most incidence
angles, so the achievable spatial resolution here is coarse.

## Build and run

Generate the scene:

```bash
python3 -m venv venv && source venv/bin/activate
pip install -r requirements.txt pyroomacoustics
python scripts/simulate.py          # writes data/sim.wav
```

Build and run the beamformer:

```bash
mkdir build && cd build
cmake ..
cmake --build .
./beamformer ../data/sim.wav        # writes ../data/delay_and_sum_sim.wav
```

Built with `-Wall -Wextra -Werror -Wshadow`. `dr_wav` is vendored under `third_party/`
and included as a system header so its warnings do not trip `-Werror`.

## Verification

Two checks, both of which should hold if the delays are correct.

**1. Beamformed output exceeds the best single microphone.** Averaging four aligned
channels reinforces the target while uncorrelated reverberant and interfering energy
partially cancels, so the output should be stronger than any one microphone.

```
beamformed RMS   0.138986
best single mic  0.137014   (mic 4, the reference channel)
```

**2. Aligned summation exceeds unaligned summation.** Summing the raw channels with no
delay compensation is the control. If the delays are doing real work, removing them
should measurably weaken the result.

```
aligned sum RMS  0.555944
raw sum RMS      0.543225
```

The margins are small — about 1.4% and 2.3% respectively. That is expected for this
scene: the inter-microphone delays are only 0 to 4 samples, so the raw channels are
already nearly aligned, and a 0.377 s RT60 means a large share of the energy at each
microphone is reverberant and arrives from no single direction. The test is directional
rather than quantitative: it confirms the alignment is applied in the correct direction
and improves coherence, not that the array provides large gain.

Output was also checked audibly against the clean source.

## Notes

An early version computed delays with the sample rate hardcoded as 1600 instead of
16000, producing shifts an order of magnitude too small. The bug was isolated by
flipping the sign of the shifts: if the delays were correct, reversing them should have
made the output measurably worse. It did not, which showed the shifts were too small to
be doing anything, and pointed at the sample rate rather than the geometry.

## Limitations

- Delays are fixed constants for one known source position, computed offline. There is
  no direction-of-arrival estimation and no steering.
- Delays are integer samples only; no fractional-delay interpolation, so the achievable
  alignment is quantised to 62.5 microseconds at 16 kHz.
- Output sample rate and channel count are hardcoded in `WavWriter`.
- Offline batch processing over a whole file; not a streaming or real-time
  implementation.
