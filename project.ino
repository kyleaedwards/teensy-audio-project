#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <Wire.h>

#include "control.h"
#include "circular.h"
#include "lfo.h"

#define GRANULAR_DELAY 20000
#define READ_AVERAGE 8
#define READ_RESOLUTION 12
#define WRITE_RESOLUTION 12
#define TOTAL_MIXERS 4

// GUItool: begin automatically generated code
AudioInputI2S i2s2;                  // xy=66,126
AudioEffectGranular granular_l;      // xy=185,79
GrainScrubEffectCircular scrub_l;    // CUSTOM
AudioSynthWaveformSine sine_l;       // xy=311,126
AudioEffectMultiply mult_l;          // xy=430,99
AudioEffectBitcrusher bitcrusher_l;  // xy=668,119
AudioFilterStateVariable vcf_l;      // xy=906,140
AudioMixer4 mixers[TOTAL_MIXERS];    // CUSTOM
AudioOutputI2S i2s1;                 // xy=1159,161
AudioConnection patchCord1(i2s2, 0, scrub_l, 0);  // CUSTOM
AudioConnection patchCord2(i2s2, 0, mixers[0], 0);
AudioConnection patchCord3(scrub_l, 0, mixers[0], 1);  // CUSTOM
AudioConnection patchCord4(mixers[0], 0, mult_l, 0);
AudioConnection patchCord5(mixers[0], 0, mixers[1], 0);
AudioConnection patchCord6(sine_l, 0, mult_l, 1);
AudioConnection patchCord7(mult_l, 0, mixers[1], 1);
AudioConnection patchCord8(mixers[1], bitcrusher_l);
AudioConnection patchCord9(mixers[1], 0, mixers[2], 0);
AudioConnection patchCord10(bitcrusher_l, 0, mixers[2], 1);
AudioConnection patchCord11(mixers[2], 0, vcf_l, 0);
AudioConnection patchCord12(mixers[2], 0, mixers[3], 0);
AudioConnection patchCord13(vcf_l, 0, mixers[3], 1);
AudioConnection patchCord14(vcf_l, 1, mixers[3], 2);
AudioConnection patchCord15(vcf_l, 2, mixers[3], 3);
AudioConnection patchCord16(mixers[3], 0, i2s1, 0);
AudioConnection patchCord17(mixers[3], 0, i2s1, 1);
AudioControlSGTL5000 sgtl5000_1;  // xy=84,233
// GUItool: end automatically generated code

/**
 * Pins
 */

// Potentiometers
const int PIN_POT1 = A0;
const int PIN_POT2 = A1;
const int PIN_POT3 = A2;
const int PIN_POT4 = A3;

// Buttons/Trigs
const int PIN_TRIG1 = 1;
const int PIN_TRIG2 = 2;
const int PIN_TRIG3 = 3;
const int PIN_TRIG4 = 4;
const int PIN_MODE_BTN = 12;

// LEDs
const int PIN_TRIG_LED1 = 5;
const int PIN_TRIG_LED2 = 6;
const int PIN_TRIG_LED3 = 7;
const int PIN_TRIG_LED4 = 8;
const int PIN_MODE_LED = 10;

// AUDIO/CV
const int PIN_CV = A14;

/**
 * UI
 */
ControlState ctrl;
float ctrl_offset = 0.0;
float ctrl_waveshape = 0.0;
float ctrl_speed = 0.0;
float ctrl_depth = 0.0;
float lfo_mod = 0.0;
float freeze_speed = 1.0;
float mod = 0.0;

int crush_sample_rate = 44100;
int filter_frequency = 15000;
float amp_mod_frequency = 440.0;

int16_t del_l[GRANULAR_DELAY];

/**
 * Modulation
 */
WavetableLFO square_lfo(500, TBL_SQUARE_LEN, TBL_SQUARE);
WavetableLFO ramp_lfo(500, TBL_RAMP_LEN, TBL_RAMP);
WavetableLFO wobble_lfo(500, TBL_WOBBLE_LEN, TBL_WOBBLE);
WavetableLFO tri_lfo(500, TBL_TRI_LEN, TBL_TRI);
WavetableLFO rev_wobble_lfo(500, TBL_REV_WOBBLE_LEN, TBL_REV_WOBBLE);
WavetableLFO saw_lfo(500, TBL_SAW_LEN, TBL_SAW);

#define MATRIX_LFO_LEN 6
WavetableLFO *MATRIX_LFO[MATRIX_LFO_LEN] = {
    &ramp_lfo, &rev_wobble_lfo, &tri_lfo, &wobble_lfo, &saw_lfo, &square_lfo};

WavetableMatrixLFO matrix_lfo(500, MATRIX_LFO_LEN, MATRIX_LFO);

/**
 * FX
 */
bool mod_start = false;
bool mod_length = false;
bool mod_speed = false;
bool reset_on_trig = false;

#define NUM_EFFECTS 5
int fx[4] = {-1, -1, -1, -1};
bool fx_enabled[NUM_EFFECTS];
enum EffectType { LOWPASS, BANDPASS, AMPLITUDE_MODULATION, MIX, SAMPLE_RATE };

/**
 * Make auxilliary effects trigger randomly.
 */
int fx_probabilities[NUM_EFFECTS];

void enable_random_fx(int index) {
    if (fx[index] > -1) {
        return;
    }
    int effect = random(NUM_EFFECTS);
    int probability = random(100);
    Serial.print("Probability: ");
    Serial.print(fx_probabilities[effect]);
    Serial.print(" > ");
    Serial.println(probability);
    if (fx_enabled[effect] || fx_probabilities[effect] <= probability) {
        Serial.print("Effect: ");
        Serial.println(effect);
        Serial.print("Enabled already: ");
        Serial.println(fx_enabled[effect]);
        Serial.print("Probability: ");
        Serial.println(probability);
        return;
    }
    fx_enabled[effect] = true;
    bool bandpass_enabled = fx_enabled[EffectType::BANDPASS];
    bool lowpass_enabled = fx_enabled[EffectType::LOWPASS];
    switch (effect) {
        case EffectType::LOWPASS:
            Serial.println("Lowpass ON");
            mixers[3].gain(0, 0);
            mixers[3].gain(1, bandpass_enabled ? 0.475 : 0.95);
            if (bandpass_enabled) {
                mixers[3].gain(2, 0.475);
            }
            break;
        case EffectType::BANDPASS:
            Serial.println("Bandpass ON");
            mixers[3].gain(0, 0);
            mixers[3].gain(2, lowpass_enabled ? 0.475 : 0.95);
            if (lowpass_enabled) {
                mixers[3].gain(1, 0.475);
            }
            break;
        case EffectType::AMPLITUDE_MODULATION:
            Serial.println("AM ON");
            mixers[1].gain(0, 0);
            mixers[1].gain(1, 0.95);
            break;
        case EffectType::MIX:
            Serial.println("Mix ON");
            break;
        case EffectType::SAMPLE_RATE:
            Serial.println("Sample Rate ON");
            mixers[2].gain(0, 0);
            mixers[2].gain(1, 0.95);
            break;
    }
    fx[index] = effect;
}

void disable_random_fx(int index) {
    if (fx[index] == -1) {
        return;
    }
    fx_enabled[fx[index]] = false;
    bool bandpass_enabled = fx_enabled[EffectType::BANDPASS];
    bool lowpass_enabled = fx_enabled[EffectType::LOWPASS];
    switch (fx[index]) {
        case EffectType::LOWPASS:
            Serial.println("Lowpass OFF");
            mixers[3].gain(bandpass_enabled ? 2 : 0, 0.95);
            mixers[3].gain(1, 0);
            break;
        case EffectType::BANDPASS:
            Serial.println("Bandpass OFF");
            mixers[3].gain(lowpass_enabled ? 1 : 0, 0.95);
            mixers[3].gain(2, 0);
            break;
        case EffectType::AMPLITUDE_MODULATION:
            Serial.println("AM OFF");
            mixers[1].gain(0, 0.95);
            mixers[1].gain(1, 0);
            break;
        case EffectType::MIX:
            Serial.println("Mix OFF");
            break;
        case EffectType::SAMPLE_RATE:
            Serial.println("Sample Rate OFF");
            mixers[2].gain(0, 0.95);
            mixers[2].gain(1, 0);
            break;
    }
    fx[index] = -1;
}

/**
 * Timers
 */
unsigned long cm;
unsigned long prev[8];

void setup() {
    // If the "AudioMemoryUsageMax()" is reporting a number close or equal
    // to what we have, just increase it.
    AudioMemory(13);

    sgtl5000_1.enable();
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000_1.lineInLevel(13);

    sgtl5000_1.volume(0.4);

    sine_l.amplitude(1);
    sine_l.frequency(220.0);

    vcf_l.frequency(15000);
    vcf_l.resonance(2.5);

    bitcrusher_l.bits(16);
    bitcrusher_l.sampleRate(44100);

    scrub_l.begin(del_l, GRANULAR_DELAY);
    scrub_l.setLengthPos(1.0);

    mixers[0].gain(0, 0.95);
    mixers[0].gain(1, 0);
    mixers[0].gain(2, 0);
    mixers[0].gain(3, 0);
    mixers[1].gain(0, 0.95);
    mixers[1].gain(1, 0);
    mixers[1].gain(2, 0);
    mixers[1].gain(3, 0);
    mixers[2].gain(0, 0.95);
    mixers[2].gain(1, 0);
    mixers[2].gain(2, 0);
    mixers[2].gain(3, 0);
    mixers[3].gain(0, 0.95);
    mixers[3].gain(1, 0);
    mixers[3].gain(2, 0);
    mixers[3].gain(3, 0);

    analogReadResolution(READ_RESOLUTION);
    analogReadAveraging(READ_AVERAGE);
    analogWriteResolution(WRITE_RESOLUTION);

    ctrl.register_button(0, PIN_MODE_BTN);
    ctrl.register_pot(0, PIN_POT1);
    ctrl.register_pot(1, PIN_POT2);
    ctrl.register_pot(2, PIN_POT3);
    ctrl.register_pot(3, PIN_POT4);
    ctrl.register_gtl(0, PIN_TRIG1, PIN_TRIG_LED1);
    ctrl.register_gtl(1, PIN_TRIG2, PIN_TRIG_LED2);
    ctrl.register_gtl(2, PIN_TRIG3, PIN_TRIG_LED3);
    ctrl.register_gtl(3, PIN_TRIG4, PIN_TRIG_LED4);
    ctrl.register_led(0, PIN_MODE_LED);

    fx_probabilities[EffectType::LOWPASS] = 66;
    fx_probabilities[EffectType::BANDPASS] = 40;
    fx_probabilities[EffectType::AMPLITUDE_MODULATION] = 25;
    fx_probabilities[EffectType::MIX] = 50;
    fx_probabilities[EffectType::SAMPLE_RATE] = 20;
}

void loop() {
    if (ctrl.loop()) {
        cm = millis();

        ctrl_offset = ctrl.get_potentiometer(0)->value / 4095.0;
        ctrl_waveshape = ctrl.get_potentiometer(1)->value / 4095.0;
        ctrl_speed = (4095.0 - ctrl.get_potentiometer(2)->value) / 4.095;

        ctrl_depth = ctrl.get_potentiometer(3)->value / 4095.0;
        if (ctrl_depth < 0.05) {
          ctrl_depth = 0.0;
        }

        matrix_lfo.set_shape(ctrl_waveshape);
        matrix_lfo.set_time((int)ctrl_speed + 50);
        lfo_mod = matrix_lfo.value;

        float mod = ctrl_offset + (lfo_mod - 0.5) * ctrl_depth;
        mod = mod < 0.0 ? 0.0 : (mod > 1.0 ? 1.0 : mod);

        crush_sample_rate = 2500 + 22500 * mod;
        filter_frequency = 50 + 14950 * mod * mod;
        amp_mod_frequency = 1.0 + mod * 200.0;

        bitcrusher_l.sampleRate(crush_sample_rate);
        vcf_l.frequency(filter_frequency);
        sine_l.frequency(amp_mod_frequency);

        if (mod_start) {
            scrub_l.setStartPos(mod);
        }
        if (mod_length) {
            scrub_l.setLengthPos(mod);
        }
        if (mod_speed) {
            scrub_l.setSpeed(pow(2.0, (mod - 0.5) * 6.0));
        }

        Button *btn = ctrl.get_button(0);
        GateTrigger *trig1 = ctrl.get_gtl(0);
        GateTrigger *trig2 = ctrl.get_gtl(1);
        GateTrigger *trig3 = ctrl.get_gtl(2);
        GateTrigger *trig4 = ctrl.get_gtl(3);

        if (btn->long_click) {
            reset_on_trig = !reset_on_trig;
        }

        bool start_freeze = false;
        bool stop_freeze = false;

        if (trig1->high) {
            start_freeze = true;
            mod_start = true;
            enable_random_fx(0);
        } else if (trig1->low) {
            stop_freeze = true;
            mod_start = false;
            scrub_l.setStartPos(0.0);
            disable_random_fx(0);
        }

        if (trig2->high) {
            start_freeze = true;
            mod_length = true;
            enable_random_fx(1);
        } else if (trig2->low) {
            stop_freeze = true;
            mod_length = false;
            scrub_l.setLengthPos(0.5);
            disable_random_fx(1);
        }

        if (trig3->high) {
            start_freeze = true;
            scrub_l.reverse();
            enable_random_fx(2);
        } else if (trig3->low) {
            stop_freeze = true;
            scrub_l.forward();
            disable_random_fx(2);
        }

        if (trig4->high) {
            start_freeze = true;
            mod_speed = true;
            enable_random_fx(3);
        } else if (trig4->low) {
            stop_freeze = true;
            mod_speed = false;
            scrub_l.setSpeed(1.0);
            disable_random_fx(3);
        }
        
        bool trig_on = trig1->gate || trig2->gate || trig3->gate || trig4->gate;

        if (start_freeze) {
            scrub_l.start();
            mixers[0].gain(0, 0);
            mixers[0].gain(1, 0.95);
            if (reset_on_trig && !trig_on) {
                matrix_lfo.reset();
            }
        }

        if (stop_freeze && !trig_on) {
            scrub_l.stop();
            mixers[0].gain(0, 0.95);
            mixers[0].gain(1, 0);
        }

        bool mix_enabled = fx_enabled[EffectType::MIX];
        if (mix_enabled) {
            mixers[0].gain(1, mod);
        }

        matrix_lfo.loop(cm);
        // scrub_l.debug();

        if (cm - prev[0] > 500) {
            prev[0] = cm;
            Serial.print("processor: ");
            Serial.print(AudioProcessorUsageMax());
            Serial.print("%    Memory: ");
            Serial.print(AudioMemoryUsageMax());
            Serial.println();
            AudioProcessorUsageMaxReset();
            AudioMemoryUsageMaxReset();
        }
    }
}
