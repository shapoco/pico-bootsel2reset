#include <stdint.h>

namespace bs2rst {

    static constexpr uint16_t SHORT_PUSH_TIME_MS = 100;
static constexpr uint16_t LONG_PUSH_TIME_MS = 1000;
static constexpr uint16_t RESET_HOLD_TIME_MS = 100;

void init();
void bootsel_rose();
void tick_ms();
bool bootsel_read();
void reset_write(bool enable);

}  // namespace bs2rst
