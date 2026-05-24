#include "wokwi-api.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define SAMPLE_RATE 16000.0
#define TONE_HZ 440.0
#define BITS_PER_SAMPLE 32

typedef struct {
  pin_t bck;
  pin_t ws;
  pin_t data;
  uint32_t gain_attr;
  uint32_t bit_index;
  int32_t sample;
  double phase;
} chip_state_t;

static int32_t next_sample(chip_state_t *chip) {
  const uint32_t gain = attr_read(chip->gain_attr);
  const double amplitude = (double)gain / 100.0 * 400000.0;
  chip->phase += (2.0 * 3.141592653589793 * TONE_HZ) / SAMPLE_RATE;
  if (chip->phase > 2.0 * 3.141592653589793) {
    chip->phase -= 2.0 * 3.141592653589793;
  }
  return (int32_t)(sin(chip->phase) * amplitude);
}

static void on_bck(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (value == LOW) {
    return;
  }

  if (pin_read(chip->ws) != LOW) {
    pin_write(chip->data, LOW);
    return;
  }

  const uint32_t bit =
    ((uint32_t)chip->sample >> (31 - chip->bit_index)) & 1u;
  pin_write(chip->data, bit);

  chip->bit_index++;
  if (chip->bit_index >= BITS_PER_SAMPLE) {
    chip->bit_index = 0;
    chip->sample = next_sample(chip);
  }
}

void chip_init(void) {
  chip_state_t *chip = (chip_state_t *)calloc(1, sizeof(chip_state_t));
  chip->gain_attr = attr_init("gain", 70);
  chip->sample = next_sample(chip);

  pin_init("VCC", INPUT);
  pin_init("GND", INPUT);
  pin_init("L/R", INPUT);
  chip->bck = pin_init("BCK", INPUT);
  chip->ws = pin_init("WS", INPUT);
  chip->data = pin_init("DATA", OUTPUT);

  const pin_watch_config_t config = {
    .edge = BOTH,
    .pin_change = on_bck,
    .user_data = chip,
  };
  pin_watch(chip->bck, &config);
}
