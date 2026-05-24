#include "wokwi-api.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define SAMPLE_RATE 16000.0
#define TONE_HZ 440.0
#define DATA_BITS 16

typedef struct {
  pin_t bck;
  pin_t ws;
  pin_t data;
  uint32_t gain_attr;
  uint8_t bit_index;
  uint8_t primed;
  int16_t sample;
  double phase;
} chip_state_t;

static int16_t next_sample(chip_state_t *chip) {
  const uint32_t gain = attr_read(chip->gain_attr);
  const double amplitude = ((double)gain / 100.0) * 12000.0;
  chip->phase += (2.0 * 3.141592653589793 * TONE_HZ) / SAMPLE_RATE;
  if (chip->phase > 2.0 * 3.141592653589793) {
    chip->phase -= 2.0 * 3.141592653589793;
  }
  return (int16_t)(sin(chip->phase) * amplitude);
}

static void begin_left_frame(chip_state_t *chip) {
  chip->bit_index = 0;
  chip->primed = 0;
  chip->sample = next_sample(chip);
}

static void on_ws(void *user_data, pin_t pin, uint32_t value) {
  chip_state_t *chip = (chip_state_t *)user_data;
  if (value == LOW) {
    begin_left_frame(chip);
  }
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

  if (!chip->primed) {
    chip->primed = 1;
    pin_write(chip->data, LOW);
    return;
  }

  const uint32_t shift = 15 - chip->bit_index;
  const uint32_t bit = ((uint16_t)chip->sample >> shift) & 1u;
  pin_write(chip->data, bit);

  chip->bit_index++;
  if (chip->bit_index >= DATA_BITS) {
    pin_write(chip->data, LOW);
  }
}

void chip_init(void) {
  chip_state_t *chip = (chip_state_t *)calloc(1, sizeof(chip_state_t));
  chip->gain_attr = attr_init("gain", 100);
  begin_left_frame(chip);

  pin_init("VCC", INPUT);
  pin_init("GND", INPUT);
  pin_init("L/R", INPUT);
  chip->bck = pin_init("BCK", INPUT);
  chip->ws = pin_init("WS", INPUT);
  chip->data = pin_init("DATA", OUTPUT);

  const pin_watch_config_t bck_watch = {
    .edge = BOTH,
    .pin_change = on_bck,
    .user_data = chip,
  };
  const pin_watch_config_t ws_watch = {
    .edge = BOTH,
    .pin_change = on_ws,
    .user_data = chip,
  };

  pin_watch(chip->bck, &bck_watch);
  pin_watch(chip->ws, &ws_watch);
}
