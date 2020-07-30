# Teensy Audio Project (Name TBD)

This project is a momentary glitch machine, consisting of four triggerable granular delay effects, each with the random possibility of triggering auxilliary effects, and a wavetable LFO modulation source.

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
- [ ] Clock/quantized input
- [ ] CV DAC output

#### Hardware
- [ ] Voltage regulator circuit
- [ ] Audio level input
- [ ] Audio level output
- [ ] CV DAC output

## Ideas
- Additional mode makes grain repeat random as well
- Latching option
