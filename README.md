# Project (Name TBD)

This project is a momentary glitch machine, consisting of four triggerable momentary granular delay effects, random additional effects, and a wavetable LFO modulation source.

## Event Triggers

The project has 4 gate/trig inputs that momentarily enable effects and/or the modulation's control over the effect.

- Grain freeze. Modulation effects the start position. When not triggered, start position is 0.
- Grain freeze. Modulation effects the grain length. When not triggered, the length is the clock length, or if not clocked, half of the delay buffer.
- Reverse grain freeze. Modulation does nothing.
- Grain freeze. Modulation effects the speed of the freeze. When not triggered, the speed is 1.0 (normal playback speed).

Additionally, each trigger has a slight chance to randomly enable one of these effects:

- Lowpass filter. Modulation effects cutoff frequency.
- Bandpass filter. Modulation effects cutoff frequency.
- Amplitude modulation. Modulation effects the modulation sine wave frequency.
- Audio level. Modulation effects the mix of the grain freeze playback.
- Sample reduction. Modulation effects the degraded sample rate.

## Clock Input

*Not implemented yet.* The clock input would cause both the LFO speed, the grain length, and the grain playback speed to be quantized to a multiplication or division of the clock interval.

## Todos

#### Software
- [ ] Explore feasibility of a stereo signal path
- [ ] Extra modes
- [ ] Clock/quantized input
- [ ] CV DAC output
- [ ] Maybe (???) refactor grain effect to use two circular buffers so it keeps recording while the other buffer continues writing (see note in effect.h)

#### Hardware
- [ ] Voltage regulator circuit
- [ ] Audio level input
- [ ] Audio level output
- [ ] CV DAC output

## Ideas
- Reset LFO on trig in
- Hold mode button causes pots to set other values (filter resonance, bit reduction, default grain length, etc...)
- Additional mode makes grain repeat random as well
- Latching option
