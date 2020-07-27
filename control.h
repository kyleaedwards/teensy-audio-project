// control.h

#pragma once

#ifndef M_CONTROL_H_
#define M_CONTROL_H_

#define CONTROL_RATE 10

#define BTN_DEBOUNCE 50
#define BTN_CLICK_MEDIUM 500
#define BTN_CLICK_LONG 2000

/**
 * The ControlState class holds an array of component pointers.
 * These values are arbitrary and can be changed to reflect the
 * actual components used by the project.
 */
#define MAX_BUTTONS 6
#define MAX_POTS 6
#define MAX_DIGITAL_LEDS 6
#define MAX_GTLS 6

#define POT_SMOOTH_SAMPLES 32
#define POT_SMOOTH_OUTLIER_LOW (32 * 20) / 100
#define POT_SMOOTH_OUTLIER_HIGH ((32 * 80) / 100) + 1

/**
 * LED abstraction with some blinking functionality.
 */
class DigitalLed {
   public:
    DigitalLed(int pin);
    void blink(int ms);
    void toggle(void);
    void on(void);
    void off(void);
    void setup(void);
    void loop(unsigned long ms);

   private:
    unsigned long _last_ms;
    int _pin;
    int _period;
    bool _state;
    bool _last_state;
};

/**
 * Potentiometer class with John-Mike's smoothing method built in.
 */
class Potentiometer {
   public:
    int value;
    Potentiometer(int pin);
    void loop(unsigned long ms);

   private:
    int _pin;
    int _smooth;
    int _samples[POT_SMOOTH_SAMPLES];
};

/**
 * This could possibly be replaced with the Bounce library, but
 * it may be nice to have some functionality of different length
 * button presses to toggle various modes of operation.
 */
class Button {
   public:
    bool pressed;
    bool released;
    bool down;

    // Helper flags for click lengths.
    bool click;
    bool short_click;
    bool medium_click;
    bool long_click;

    Button(int pin);
    void setup(void);
    void loop(unsigned long ms);

   private:
    int _pin;
    int _state;
    int _last_state;
    unsigned long _last_ms;
};

/**
 * Combination of an input pullup pin and an LED.
 *
 * TODO: Make a latch option, so rather than trigger, a high signal toggles
 *       the gate on or off.
 */
class GateTrigger {
   public:
    bool high;
    bool low;
    bool gate;
    bool led_override;

    GateTrigger(int pin_input, int pin_led);
    void setup(void);
    void loop(unsigned long ms);

   private:
    int _input_pin;
    int _state;
    int _last_state;
    DigitalLed* _led;
    unsigned long _last_ms;
};

/**
 * Overkill, but this keeps track of all of the control elements
 * and updates their state after a provided control interval. The
 * loop() method returns a boolean to indicate the interval is
 * reached and new values are read from the input pins.
 */
class ControlState {
   public:
    ControlState();
    bool loop(void);
    void register_button(int index, int pin);
    void register_pot(int index, int pin);
    void register_led(int index, int pin);
    void register_gtl(int index, int input_pin, int led_pin);
    Button* get_button(int index);
    Potentiometer* get_potentiometer(int index);
    DigitalLed* get_led(int index);
    GateTrigger* get_gtl(int index);

   private:
    unsigned long _last_ms;
    unsigned long _ms;
    Button* _buttons[MAX_BUTTONS];
    Potentiometer* _pots[MAX_POTS];
    DigitalLed* _leds[MAX_DIGITAL_LEDS];
    GateTrigger* _gtls[MAX_GTLS];
};

#endif
