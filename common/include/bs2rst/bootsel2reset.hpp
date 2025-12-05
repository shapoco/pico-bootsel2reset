#include <stdint.h>

namespace bs2rst {

void init();
void service();
void bootsel_change();
void timer_tick();

void cpu_sleep(bool deep);
void timer_start();
void timer_stop();
bool timesel_read();
bool noclick_read();
bool bootsel_read();
void reset_write(bool enable);

}  // namespace bs2rst
