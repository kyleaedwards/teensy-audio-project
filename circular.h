// circular.h

#include <Audio.h>

#pragma once

/**
 * An adaptation of John-Mike Reed's granular effect in the Teensy Audio
 * Library.
 *
 * TODO: Currently, this effect begins writing to the buffer only when
 * triggered, which means there's a slight delay before it begins repeating,
 * which may not be very musical. By recording to the buffer all the time using
 * a circular buffer, you can begin playback instantly, however it introduces a
 * potential issue of hold length. If you were to hold a freeze for longer than
 * the maximum buffer length, then you would begin to overwrite it with unwanted
 * results.
 *
 * To make it work correctly, you'll need two buffers that are used alternately.
 * The downside of this is that the effect would take up twice the amount of
 * memory as the current granular effect.
 *
 * See https://github.com/PaulStoffregen/Audio/blob/master/effect_granular.h
 */
class GrainScrubEffectCircular : public AudioStream {
   public:
    GrainScrubEffectCircular(void) : AudioStream(1, inputQueueArray) {}

    /**
     * Similar to the granular effect, initialize with an int16_t audio buffer
     * and the maximum length of the buffer.
     *
     * @param sample_bank_def Audio buffer array of int16_t
     * @param max_len_def Length of sample_bank_def
     */
    void begin(int16_t *sample_bank_def, int16_t max_len_def);

    /**
     * Calculates a integer playback rate from a float value.
     *
     * @param ratio Speed of playback where 1.0 is the standard sample rate
     */
    void setSpeed(float ratio) {
        if (ratio < -4.0) {
            ratio = -4.0;
        } else if (ratio < 0 && ratio >= -0.125) {
            ratio = -0.125;
        } else if (ratio < 0.125)
            ratio = 0.125;
        else if (ratio > 4.0)
            ratio = 4.0;
        next_playback_rate = ratio * 65536.0 + 0.499;
    }

    /**
     * Reverses the current playback speed.
     */
    void reverse(void) { next_reversed = true; }
    void forward(void) { next_reversed = false; }

    /**
     * Sets the start position based on a millisecond value. Useful when
     * the position needs to be quantized to a beat.
     *
     * @param ms Milliseconds from the start of the delay sample
     */
    void setStartMs(float ms) {
        if (ms < 0.0) {
            ms = 0.0;
        } else if (ms > length_ms) {
            ms = length_ms - 1.0;
        }
        int16_t new_offset = ms * AUDIO_SAMPLE_RATE_EXACT * 0.001;
        next_offset = new_offset;
        if (ideal_length + next_offset > max_sample_len) {
            next_length = max_sample_len - next_offset;
        } else {
            next_length = ideal_length;
        }
    }

    /**
     * Sets the start position based on a fractional value based
     * on the full delay buffer length.
     *
     * @param pos Fraction of the max delay time
     */
    void setStartPos(float pos) {
        if (pos < 0.0)
            pos = 0.0;
        else if (pos > 0.99)
            pos = 0.99;
        int16_t new_offset = pos * max_sample_len;
        next_offset = new_offset;
        if (ideal_length + next_offset > max_sample_len) {
            next_length = max_sample_len - next_offset - 1;
        } else {
            next_length = ideal_length;
        }
    }

    /**
     * Sets the length based on a millisecond value. Useful when
     * the length needs to be quantized to a beat.
     *
     * @param ms Millisecond length of the sample playback
     */
    void setLengthMs(float ms) {
        if (ms < 1.0) {
            ms = 1.0;
        } else if (ms > length_ms) {
            ms = length_ms;
        }
        int16_t new_length = (ms * AUDIO_SAMPLE_RATE_EXACT * 0.001) - offset;
        if (new_length < 50) {
            new_length = max_sample_len - offset - 1;
        }
        next_length = new_length;

        // Ideal length keeps the true length, even if the start time
        ideal_length = next_length;
    }

    /**
     * Sets the start position based on a fractional value based
     * on the full delay buffer length.
     *
     * @param pos Fraction of the max delay time
     */
    void setLengthPos(float pos) {
        if (pos < 0.01)
            pos = 0.01;
        else if (pos > 1.0)
            pos = 1.0;
        int16_t new_length = (max_sample_len * pos) - offset;
        if (new_length < 50) {
            new_length = max_sample_len - offset - 1;
        }
        next_length = new_length;
        ideal_length = next_length;
    }

    void debug(void) {
        Serial.print("Max Sample Length: ");
        Serial.println(max_sample_len);
        Serial.print("Accumulator: ");
        Serial.println(accumulator >> 16);
        Serial.print("Write Head: ");
        Serial.println(write_head);
        Serial.print("Read Head: ");
        Serial.println(read_head);
        Serial.print("Offset: ");
        Serial.print(next_offset);
        Serial.print(" -> ");
        Serial.println(offset);
        Serial.print("Length: ");
        Serial.print(next_length);
        Serial.print(" -> ");
        Serial.println(length);
        Serial.print("Playback Rate: ");
        Serial.print(next_playback_rate);
        Serial.print(" -> ");
        Serial.println(playback_rate);
        Serial.print("Reversed: ");
        Serial.print(next_reversed);
        Serial.print(" -> ");
        Serial.println(reversed);
    }

    void start(void);
    void stop(void);
    virtual void update(void);

   private:
    audio_block_t *inputQueueArray[1];
    int16_t *sample_bank;
    int32_t playback_rate;
    int32_t next_playback_rate;
    uint32_t accumulator;
    int16_t max_sample_len;
    int16_t write_head;
    int16_t read_head;
    int16_t read_head_offset;
    int16_t active_buffer;
    int16_t offset;
    int16_t length;
    int16_t ideal_length;
    int16_t next_length;
    int16_t next_offset;
    float length_ms;
    bool running;
    bool reversed;
    bool next_reversed;
};
