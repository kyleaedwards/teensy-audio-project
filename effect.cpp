#include "effect.h"

#include <Arduino.h>

void GrainScrubEffect::begin(int16_t *sample_bank_def, int16_t max_len_def) {
    max_sample_len = max_len_def;
    length_ms = ((float)max_sample_len / AUDIO_SAMPLE_RATE_EXACT) * 1000;
    read_head = 0;
    write_head = 0;
    prev_input = 0;
    playback_rate = 65536;
    next_playback_rate = 65536;
    accumulator = 0;
    reversed = false;
    next_reversed = false;
    sample_loaded = false;
    sample_bank = sample_bank_def;
}

void GrainScrubEffect::start() {
    if (running) {
      return;
    }
    __disable_irq();
    sample_loaded = false;
    write_enabled = false;
    zero_found = false;
    running = true;
    offset = next_offset;
    length = next_length;
    playback_rate = next_playback_rate;
    reversed = next_reversed;
    __enable_irq();
}

void GrainScrubEffect::stop() {
    __disable_irq();
    running = false;
    zero_found = false;
    sample_loaded = false;
    write_enabled = false;
    __enable_irq();
}

void GrainScrubEffect::update(void) {
    audio_block_t *block;

    if (sample_bank == NULL) {
        block = receiveReadOnly(0);
        if (block) {
            release(block);
        }
        return;
    }

    block = receiveWritable(0);
    if (!block) {
        return;
    }

    if (!running) {
        prev_input = block->data[AUDIO_BLOCK_SAMPLES - 1];
    } else {
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            // Step 1: Find zero-crossing
            if (!zero_found) {
                int16_t current_input = block->data[i];
                if ((current_input < 0 && prev_input >= 0) ||
                    (current_input >= 0 && prev_input < 0)) {
                    write_enabled = true;
                    write_head = 0;
                    read_head = 0;
                    zero_found = true;
                } else {
                    prev_input = current_input;
                }
            }

            // Step 2: Write samples to buffer
            if (write_enabled) {
                sample_bank[write_head++] = block->data[i];
                if (write_head >= offset + length) {
                    sample_loaded = true;
                }
                if (write_head >= max_sample_len) {
                    write_enabled = false;
                }
            }

            // Step 3: Figure playback
            if (sample_loaded) {
                if (!reversed) {
                    accumulator += playback_rate;
                    read_head = accumulator >> 16;
                    if (read_head >= length || read_head < 0) {
                        accumulator = 0;
                        read_head = 0;
                        // Only change the offset/length after a full repeat
                        offset = next_offset;
                        length = next_length;
                        playback_rate = next_playback_rate;
                        reversed = next_reversed;
                    }
                    // NOTE: Keep an ear out for pops if the length/start
                    // changes during a fade out.
                    if (length - read_head < 20) {
                        float fade_out = sample_bank[offset + read_head] *
                                         ((float)(length - read_head) / 20.0);
                        block->data[i] = (int16_t)fade_out;
                    } else if (read_head < 20) {
                        float fade_out = sample_bank[offset + read_head] *
                                         ((float)read_head / 20.0);
                        block->data[i] = (int16_t)fade_out;
                    } else {
                        block->data[i] = sample_bank[offset + read_head];
                    }
                } else {  // Reverse grains
                    accumulator += playback_rate;
                    read_head = length - (accumulator >> 16) - 1;
                    if (read_head < 0 || read_head >= length) {
                        accumulator = 0;
                        read_head = length - 1;
                        // Only change the offset/length after a full repeat
                        offset = next_offset;
                        length = next_length;
                        playback_rate = next_playback_rate;
                        reversed = next_reversed;
                    }
                    // NOTE: Keep an ear out for pops if the length/start
                    // changes during a fade out.
                    if (length - read_head < 20) {
                        float fade_out = sample_bank[offset + read_head] *
                                         ((float)read_head / 20.0);
                        block->data[i] = (int16_t)fade_out;
                    } else if (read_head < 20) {
                        float fade_out = sample_bank[offset + read_head] *
                                         ((float)read_head / 20.0);
                        block->data[i] = (int16_t)fade_out;
                    } else {
                        block->data[i] = sample_bank[offset + read_head];
                    }
                }
            }
        }
    }

    transmit(block);
    release(block);
}
