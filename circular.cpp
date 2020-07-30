#include "circular.h"

#include <Arduino.h>

void GrainScrubEffectCircular::begin(int16_t *sample_bank_def, int16_t max_len_def) {
    max_sample_len = max_len_def / 2;
    length_ms = ((float)max_sample_len / AUDIO_SAMPLE_RATE_EXACT) * 1000;
    active_buffer = 0;
    read_head = 0;
    write_head = 0;
    playback_rate = 65536;
    next_playback_rate = 65536;
    accumulator = 0;
    reversed = false;
    next_reversed = false;
    sample_bank = sample_bank_def;
}

void GrainScrubEffectCircular::start() {
    if (running) {
        return;
    }
    __disable_irq();
    active_buffer = active_buffer == 0 ? 1 : 0;
    read_head = 0;
    read_head_offset = write_head;
    running = true;
    offset = next_offset;
    length = next_length;
    playback_rate = next_playback_rate;
    reversed = next_reversed;
    __enable_irq();
}

void GrainScrubEffectCircular::stop() {
    __disable_irq();
    running = false;
    __enable_irq();
}

void GrainScrubEffectCircular::update(void) {
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
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            sample_bank[write_head] = block->data[i];
            sample_bank[write_head + max_sample_len] = block->data[i];
            write_head++;
            if (write_head >= max_sample_len) {
                write_head = 0;
            }
        }
    } else {
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            // Step 1: Write samples to buffer
            if (active_buffer == 0) {
                sample_bank[write_head + max_sample_len] = block->data[i];
            } else {
                sample_bank[write_head] = block->data[i];
            }
            write_head++;
            if (write_head >= max_sample_len) {
                write_head = 0;
            }

            // Step 2: Figure playback
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
                int16_t read_index =
                    (offset + read_head + read_head_offset) %
                    max_sample_len;
                if (active_buffer == 1) {
                    read_index += max_sample_len;
                }
                if (length - read_head < 20) {
                    float fade_out = sample_bank[read_index] *
                                     ((float)(length - read_head) / 32.0);
                    block->data[i] = (int16_t)fade_out;
                } else if (read_head < 20) {
                    float fade_out =
                        sample_bank[read_index] * ((float)read_head / 32.0);
                    block->data[i] = (int16_t)fade_out;
                } else {
                    block->data[i] = sample_bank[read_index];
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
                int16_t read_index =
                    (offset + read_head + read_head_offset) %
                    max_sample_len;
                if (active_buffer == 1) {
                    read_index += max_sample_len;
                }
                if (length - read_head < 20) {
                    float fade_out =
                        sample_bank[read_index] * ((float)(length - read_head) / 32.0);
                    block->data[i] = (int16_t)fade_out;
                } else if (read_head < 20) {
                    float fade_out =
                        sample_bank[read_index] * ((float)read_head / 32.0);
                    block->data[i] = (int16_t)fade_out;
                } else {
                    block->data[i] = sample_bank[read_index];
                }
            }
        }
    }

    transmit(block);
    release(block);
}
