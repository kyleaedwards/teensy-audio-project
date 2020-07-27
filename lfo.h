// env.h

#include <Arduino.h>

#pragma once

#ifndef M_LFO_H_
#define M_LFO_H_

/**
 * Wavetable-based LFO. Interpolates the position within the wavetable, so the
 * larger the wavetable is, the more accurate it will be.
 */
class WavetableLFO {
   public:
    WavetableLFO(int duration, int length, int* tbl);
    void loop(unsigned long ms);
    void reset(void);
    void set_time(int duration);
    float value;
    int byte_value;

   private:
    unsigned long _last_ms;
    uint32_t _time;
    uint32_t _current_time;
    uint32_t _increment;
    int _index;
    int* _tbl;
    uint32_t _length;
};

/**
 * Two-dimensional wavetable LFO. The shape of the wavetable determines the
 * interpolation between the current index and the next, so the shapes mix with
 * each other.
 */
class WavetableMatrixLFO {
   public:
    WavetableMatrixLFO(int duration, byte matrix_length, WavetableLFO** matrix);
    float value;
    void loop(unsigned long ms);
    void reset(void);
    void set_time(int duration);
    void set_shape(float position) {
        if (position <= 0.0) {
            position = 0.0;
        }
        if (position >= 1.0) {
            position = 1.0;
        }
        float pos = position * (_matrix_length - 0.8);
        _index = pos;
        _ratio = pos - _index;
    }

   private:
    unsigned int _index;
    float _ratio;
    WavetableLFO** _matrix;
    byte _matrix_length;
};

#define TBL_SQUARE_LEN 60
extern int TBL_SQUARE[TBL_SQUARE_LEN];
#define TBL_RAMP_LEN 6
extern int TBL_RAMP[TBL_RAMP_LEN];
#define TBL_WOBBLE_LEN 60
extern int TBL_WOBBLE[TBL_WOBBLE_LEN];
#define TBL_TRI_LEN 11
extern int TBL_TRI[TBL_TRI_LEN];
#define TBL_REV_WOBBLE_LEN 60
extern int TBL_REV_WOBBLE[TBL_REV_WOBBLE_LEN];
#define TBL_SAW_LEN 6
extern int TBL_SAW[TBL_SAW_LEN];

#endif
