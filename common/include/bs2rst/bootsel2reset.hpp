#include <stdint.h>

namespace bs2rst {

static constexpr uint16_t CLICK_TIME_MS = 50;
static constexpr uint16_t HOLD_TIME_SHORT_MS = 500;
static constexpr uint16_t HOLD_TIME_LONG_MS = 4000;
static constexpr uint16_t RESET_HOLD_TIME_MS = 100;

void init();
void service();
void bootsel_change();
void timer_tick();

void cpu_sleep(bool deep);
void timer_start();
void timer_stop();
bool timesel_read();
bool bootsel_read();
void reset_write(bool enable);

}  // namespace bs2rst
