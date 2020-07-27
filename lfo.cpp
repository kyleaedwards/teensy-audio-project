#include "lfo.h"

int interpolate(int* input, int len, uint32_t index) {
    int i = index >> 16;
    int weight = index & 65535;
    if (i >= len - 1) {
        return input[len - 1];
    }
    return (input[i] * (65536 - weight) + input[i + 1] * weight) >> 16;
}

WavetableLFO::WavetableLFO(int duration, int length, int* tbl) {
    _tbl = tbl;
    _length = length;
    set_time(duration);
}

void WavetableLFO::set_time(int duration) { _time = duration; }

void WavetableLFO::loop(unsigned long ms) {
    unsigned long duration = ms - _last_ms;
    _last_ms = ms;
    _current_time += duration;
    _increment = _current_time * (_length - 1) * 65536 / _time;
    byte_value = interpolate(_tbl, _length, _increment);
    value = (float)byte_value / 255.0;
    if (_current_time >= _time) {
        _current_time = 0;
    }
}

void WavetableLFO::reset() { _current_time = 0; }

WavetableMatrixLFO::WavetableMatrixLFO(int duration, byte matrix_length,
                                       WavetableLFO** matrix) {
    _matrix = matrix;
    _matrix_length = matrix_length;
    set_time(duration);
}

void WavetableMatrixLFO::set_time(int duration) {
    for (byte i = 0; i < _matrix_length; i++) {
        _matrix[i]->set_time(duration);
    }
}

void WavetableMatrixLFO::loop(unsigned long ms) {
    for (byte i = 0; i < _matrix_length; i++) {
        _matrix[i]->loop(ms);
    }
    if (_index + 1 >= _matrix_length) {
        value = _matrix[_index]->value;
        return;
    }

    value = (1.0 - _ratio) * _matrix[_index]->value +
            _ratio * _matrix[_index + 1]->value;
}

void WavetableMatrixLFO::reset() {
    for (byte i = 0; i < _matrix_length; i++) {
        _matrix[i]->reset();
    }
}

int TBL_SQUARE[TBL_SQUARE_LEN] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};

int TBL_RAMP[TBL_RAMP_LEN] = {0, 51, 102, 153, 204, 255};

int TBL_WOBBLE[TBL_WOBBLE_LEN] = {
    0,   5,   10,  35,  75,  130, 185, 225, 240, 250, 255, 255, 200, 160, 130,
    105, 85,  70,  60,  50,  40,  32,  31,  30,  31,  32,  37,  50,  80,  110,
    130, 140, 145, 145, 100, 70,  45,  32,  25,  21,  19,  17,  16,  15,  14,
    13,  13,  12,  11,  10,  25,  50,  37,  16,  8,   4,   3,   2,   1,   0};

int TBL_TRI[TBL_TRI_LEN] = {0,   51,  102, 153, 204, 255,
                                  204, 153, 102, 51,  0};

int TBL_REV_WOBBLE[TBL_REV_WOBBLE_LEN] = {
    0,   1,   2,   3,   4,   8,   16,  37,  50,  25,  10,  11,  12,  13,  13,
    14,  15,  16,  17,  19,  21,  25,  32,  45,  70,  100, 145, 145, 140, 130,
    110, 80,  50,  37,  32,  31,  30,  31,  32,  40,  50,  60,  70,  85,  105,
    130, 160, 200, 255, 255, 250, 240, 225, 185, 130, 75,  35,  10,  5,   0};

int TBL_SAW[TBL_SAW_LEN] = {255, 204, 153, 102, 51, 0};
