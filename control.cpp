#include "control.h"

#include <Arduino.h>

/**
 * Control-level Timers
 */
ControlState::ControlState() {
    _ms = millis();
    _last_ms = _ms;
    for (int i = 0; i < MAX_BUTTONS; i++) {
        _buttons[i] = NULL;
    }
    for (int i = 0; i < MAX_DIGITAL_LEDS; i++) {
        _leds[i] = NULL;
    }
    for (int i = 0; i < MAX_POTS; i++) {
        _pots[i] = NULL;
    }
    for (int i = 0; i < MAX_GTLS; i++) {
        _gtls[i] = NULL;
    }
};

bool ControlState::loop() {
    _ms = millis();
    if (_ms - _last_ms >= CONTROL_RATE) {
        _last_ms = _ms;
        for (int i = 0; i < MAX_BUTTONS; i++) {
            if (_buttons[i] != NULL) {
                _buttons[i]->loop(_ms);
            }
        }
        for (int i = 0; i < MAX_POTS; i++) {
            if (_pots[i] != NULL) {
                _pots[i]->loop(_ms);
            }
        }
        for (int i = 0; i < MAX_DIGITAL_LEDS; i++) {
            if (_leds[i] != NULL) {
                _leds[i]->loop(_ms);
            }
        }
        for (int i = 0; i < MAX_GTLS; i++) {
            if (_gtls[i] != NULL) {
                _gtls[i]->loop(_ms);
            }
        }
        return true;
    }
    return false;
};

void ControlState::register_button(int index, int pin) {
    if (index >= MAX_BUTTONS) {
        return;
    }
    Button* btn = new Button(pin);
    _buttons[index] = btn;
    btn->setup();
};

void ControlState::register_pot(int index, int pin) {
    if (index >= MAX_POTS) {
        return;
    }
    Potentiometer* pot = new Potentiometer(pin);
    _pots[index] = pot;
};

void ControlState::register_led(int index, int pin) {
    if (index >= MAX_DIGITAL_LEDS) {
        return;
    }
    DigitalLed* led = new DigitalLed(pin);
    _leds[index] = led;
    led->setup();
};

void ControlState::register_gtl(int index, int input_pin, int led_pin) {
    if (index >= MAX_GTLS) {
        return;
    }
    GateTrigger* gtl = new GateTrigger(input_pin, led_pin);
    _gtls[index] = gtl;
    gtl->setup();
};

Button* ControlState::get_button(int index) {
    if (index >= MAX_BUTTONS) {
        return NULL;
    }
    return _buttons[index];
};

Potentiometer* ControlState::get_potentiometer(int index) {
    if (index >= MAX_POTS) {
        return NULL;
    }
    return _pots[index];
};

DigitalLed* ControlState::get_led(int index) {
    if (index >= MAX_DIGITAL_LEDS) {
        return NULL;
    }
    return _leds[index];
};

GateTrigger* ControlState::get_gtl(int index) {
    if (index >= MAX_GTLS) {
        return NULL;
    }
    return _gtls[index];
};

Button::Button(int pin) {
    _pin = pin;
    _state = 1;
    _last_state = 1;
};

void Button::setup() { pinMode(_pin, INPUT_PULLUP); };

void Button::loop(unsigned long ms) {
    int state = digitalRead(_pin);
    unsigned long duration = ms - _last_ms;

    if (_state == 0 && state == 1 && duration < BTN_DEBOUNCE) {
        return;
    }

    _last_state = _state;
    _state = state;

    down = _state == 0;
    pressed = _last_state == 1 && down;
    if (pressed) {
        _last_ms = ms;
    }
    released = _last_state == 0 && _state == 1;

    click = false;
    short_click = false;
    medium_click = false;
    long_click = false;
    if (released) {
        click = true;
        if (duration > BTN_CLICK_LONG) {
            long_click = true;
        } else if (duration > BTN_CLICK_MEDIUM) {
            medium_click = true;
        } else {
            short_click = true;
        }
    }
};

Potentiometer::Potentiometer(int pin) {
    _pin = pin;
    value = 0;
    _smooth = 0;
    for (int i = 0; i < POT_SMOOTH_SAMPLES; i++) {
        _samples[i] = 0;
    }
};

void Potentiometer::loop(unsigned long ms) {
    int curr = analogRead(_pin);
    int j, k, temp, top, bottom;
    long total;
    static int sorted[POT_SMOOTH_SAMPLES];
    boolean done;

    _smooth = (_smooth + 1) % POT_SMOOTH_SAMPLES;
    _samples[_smooth] = curr;

    for (j = 0; j < POT_SMOOTH_SAMPLES; j++) {
        sorted[j] = _samples[j];
    }

    done = false;
    while (!done) {
        done = true;
        for (j = 0; j < (POT_SMOOTH_SAMPLES - 1); j++) {
            if (sorted[j] > sorted[j + 1]) {
                temp = sorted[j + 1];
                sorted[j + 1] = sorted[j];
                sorted[j] = temp;
                done = false;
            }
        }
    }

    bottom = max(POT_SMOOTH_OUTLIER_LOW, 1);
    top = min(POT_SMOOTH_OUTLIER_HIGH, POT_SMOOTH_SAMPLES - 1);
    k = 0;
    total = 0;

    for (j = bottom; j < top; j++) {
        total += sorted[j];
        k++;
    }

    value = total / k;
    value = map(value, 50, 4000, 0, 4095);
    if (value < 0) {
        value = 0;
    } else if (value > 4095) {
        value = 4095;
    }
};

DigitalLed::DigitalLed(int pin) {
    _pin = pin;
    _state = false;
    _last_state = false;
    _last_ms = 0;
    _period = 0;
};

void DigitalLed::setup() { pinMode(_pin, OUTPUT); };

void DigitalLed::loop(unsigned long ms) {
    if (_state != _last_state) {
        _last_state = _state;
        digitalWrite(_pin, _state);
    }

    if (_period > 0) {
        if (_last_ms + _period < ms) {
            _last_ms = ms;
            _state = !_state;
        }
    }
}

void DigitalLed::on() {
    _period = 0;
    _state = true;
};

void DigitalLed::off() {
    _period = 0;
    _state = false;
};

void DigitalLed::toggle() {
    _period = 0;
    _state = !_state;
};

void DigitalLed::blink(int ms) { _period = ms >> 1; };

GateTrigger::GateTrigger(int input_pin, int led_pin) {
    high = false;
    low = false;
    gate = false;
    led_override = true;
    _state = false;
    _last_state = false;
    _input_pin = input_pin;
    _led = new DigitalLed(led_pin);
    _last_ms = 0;
};

void GateTrigger::setup() {
    pinMode(_input_pin, INPUT_PULLUP);
    _led->setup();
}

void GateTrigger::loop(unsigned long ms) {
    int state = digitalRead(_input_pin);
    unsigned long duration = ms - _last_ms;

    if (_state == 0 && state == 1 && duration < BTN_DEBOUNCE) {
        return;
    }

    _last_state = _state;
    _state = state;

    gate = _state == 0;
    high = _last_state == 1 && gate;
    if (high) {
        if (!led_override) {
            _led->on();
        }
        _last_ms = ms;
    }

    low = _last_state == 0 && _state == 1;
    if (low) {
        if (!led_override) {
            _led->off();
        }
    }

    _led->loop(ms);
};
