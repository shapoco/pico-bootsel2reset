#include "bs2rst/bootsel2reset.hpp"

namespace bs2rst {

static constexpr uint16_t STATE_START = 0;
static constexpr uint16_t STATE_STOP = 0xFFFF;

static bool release_detected = false;
static bool pressed = false;
static bool use_long_hold_time = false;
static int8_t debounce_counter = 0;
static uint16_t state_counter = STATE_STOP;
static bool timer_running = false;

static void activate();
static void deactivate(bool force = false);

void init() {
  if (bootsel_read()) {
    release_detected = false;
    pressed = true;
    debounce_counter = 10;
    state_counter = HOLD_TIME_LONG_MS + RESET_HOLD_TIME_MS;
    timer_start();
  } else {
    deactivate(true);
  }
}

void service() { cpu_sleep(!timer_running); }

void bootsel_change() {
  release_detected |= !bootsel_read();
  activate();
}

void timer_tick() {
  // read button state
  bool pressed_raw = bootsel_read() && !release_detected;
  release_detected = false;

  // debounce
  bool pressed_prev = pressed;
  constexpr int8_t DEBOUNCE_MAX = 10;
  if (pressed_raw) {
    if (debounce_counter <= 0) {
      debounce_counter = 1;
    } else if (debounce_counter < DEBOUNCE_MAX) {
      debounce_counter++;
    } else {
      pressed = true;
    }
  } else {
    if (debounce_counter >= 0) {
      debounce_counter = -1;
    } else if (debounce_counter > -DEBOUNCE_MAX) {
      debounce_counter--;
    } else {
      pressed = false;
    }
  }

  // read hold time selection
  if (pressed && !pressed_prev) {
    use_long_hold_time = timesel_read();
  }

  // state machine
  bool reset_enable = false;
  if (state_counter == STATE_STOP) {
    if (pressed) {
      state_counter = STATE_START;
    }
  } else if (state_counter < CLICK_TIME_MS) {
    if (pressed) {
      state_counter++;
    } else {
      deactivate();
    }
  } else if (state_counter < HOLD_TIME_LONG_MS) {
    if (pressed) {
      if (use_long_hold_time && state_counter >= HOLD_TIME_SHORT_MS) {
        state_counter = HOLD_TIME_LONG_MS;
      } else {
        state_counter++;
      }
    } else {
      state_counter = HOLD_TIME_LONG_MS;
    }
  } else if (state_counter < HOLD_TIME_LONG_MS + RESET_HOLD_TIME_MS) {
    reset_enable = true;
    state_counter++;
  } else {
    if (!pressed) {
      deactivate();
    }
  }

  reset_write(reset_enable);
}

static void activate() {
  if (!timer_running) {
    timer_start();
    timer_running = true;
  }
}

static void deactivate(bool force) {
  if (timer_running || force) {
    timer_stop();
    timer_running = false;
  }
  release_detected = false;
  pressed = false;
  debounce_counter = 0;
  state_counter = STATE_STOP;
}

}  // namespace bs2rst
