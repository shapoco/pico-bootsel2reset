#include "bs2rst/bootsel2reset.hpp"

namespace bs2rst {

static uint16_t push_time_ms = 0;

void init() {}

void bootsel_rose() {
  if (push_time_ms < SHORT_PUSH_TIME_MS) {
    push_time_ms = 0;
  }
}

void tick_ms() {
  bool pressed = bootsel_read();
  bool reset_enable = false;

  if (push_time_ms < SHORT_PUSH_TIME_MS) {
    if (pressed) {
      push_time_ms++;
    } else {
      push_time_ms = 0;
    }
  } else if (push_time_ms < LONG_PUSH_TIME_MS) {
    if (pressed) {
      push_time_ms++;
    } else {
      push_time_ms = LONG_PUSH_TIME_MS;
    }
  } else if (push_time_ms < LONG_PUSH_TIME_MS + RESET_HOLD_TIME_MS) {
    reset_enable = true;
    push_time_ms++;
  } else {
    if (!pressed) {
      push_time_ms = 0;
    }
  }

  reset_write(reset_enable);
}

}  // namespace bs2rst
