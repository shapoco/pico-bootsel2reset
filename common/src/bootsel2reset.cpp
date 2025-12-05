#include "bs2rst/bootsel2reset.hpp"

namespace bs2rst {

static constexpr uint16_t RESET_HOLD_TIME_MS = 100;
static constexpr uint16_t SHORT_CLICK_TIME_MS = 100;
static constexpr uint16_t LONG_CLICK_TIME_MS = 1000;
static constexpr uint16_t SHORT_HOLD_TIME_MS = 500 - RESET_HOLD_TIME_MS;
static constexpr uint16_t LONG_HOLD_TIME_MS = 5000 - RESET_HOLD_TIME_MS;

static constexpr uint16_t ST_START = 0;
static constexpr uint16_t ST_RESET_START = LONG_HOLD_TIME_MS;
static constexpr uint16_t ST_RESET_END = ST_RESET_START + RESET_HOLD_TIME_MS;
static constexpr uint16_t ST_STOP = 0xFFFF;

static bool release_detected = false;
static bool pressed = false;
static bool short_mode = false;
static bool no_click = false;
static int8_t debounce_counter = 0;
static uint16_t state_counter = ST_STOP;
static bool timer_running = false;

static void activate();
static void deactivate(bool force = false);

void init() {
  if (bootsel_read()) {
    release_detected = false;
    pressed = true;
    debounce_counter = 10;
    state_counter = ST_RESET_END;
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
    short_mode = timesel_read();
    no_click = noclick_read();
  }
  uint16_t st_hold_start =
      short_mode ? SHORT_CLICK_TIME_MS : LONG_CLICK_TIME_MS;
  uint16_t st_hold_end = short_mode ? SHORT_HOLD_TIME_MS : LONG_HOLD_TIME_MS;

  // state machine
  bool reset_enable = false;
  if (state_counter == ST_STOP) {
    if (pressed) {
      state_counter = ST_START;
    }
  } else if (state_counter < st_hold_start) {
    if (pressed) {
      state_counter++;
    } else {
      deactivate();
    }
  } else if (state_counter < st_hold_end) {
    if (pressed) {
      if (state_counter + 1 < st_hold_end) {
        state_counter++;
      } else {
        state_counter = ST_RESET_START;
      }
    } else {
      if (no_click) {
        deactivate();
      } else {
        state_counter = ST_RESET_START;
      }
    }
  } else if (state_counter < ST_RESET_START) {
    state_counter = ST_RESET_START;
  } else if (state_counter < ST_RESET_END) {
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
  state_counter = ST_STOP;
}

}  // namespace bs2rst
