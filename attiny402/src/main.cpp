#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include <stdint.h>

#include "bs2rst/bootsel2reset.hpp"

// clang-format off
//                ______   
//       VDD 1 --| o    |-- 8 GND
//       PA6 2 --|      |-- 7 PA3
//       PA7 3 --|      |-- 6 PA0 (UPDI)
// (SDA) PA1 4 --|______|-- 5 PA2 (SCL)
// 
// # Name  Other/Special ADC0  AC0   YSART0  SPI0    TWI0  TCA0    TCB0  CCL
// 6 PA0   RESET/UDPI    AIN0  XDIR  SS_N                                LUT0-IN0
// 4 PA1                 AIN1        TxD(3)  MOSI    SDA   WO1           LUT0-IN1
// 5 PA2   EVOUT0        AIN2        RxD(3)  MISO    SCL   WO2           LUT0-IN2
// 7 PA3   EXTCLK        AIN3  OUT   XCK     SCK           WO0/WO3
// 8 GND
// 1 VDD
// 2 PA6                 AIN6  AINN0 TxD     MOSI(3)               WO0   LUT0-OUT
// 3 PA7                 AIN7  AINP0 RxD     MISO(3)       WO0(3)        LUT1-OUT
// clang-format on

static constexpr uint8_t PICO_BOOTSEL_PIN = 1;
static constexpr uint8_t PICO_RUN_PIN = 2;
static constexpr uint8_t TIMESEL_PIN = 3;

static void set_int0_mode(bool edge) {
  if (edge) {
    // edge-sensitive
    (&PORTA.PIN0CTRL)[PICO_BOOTSEL_PIN] &= ~PORT_ISC_gm;
    (&PORTA.PIN0CTRL)[PICO_BOOTSEL_PIN] |= PORT_ISC_BOTHEDGES_gc;
  } else {
    // level-sensitive
    (&PORTA.PIN0CTRL)[PICO_BOOTSEL_PIN] &= ~PORT_ISC_gm;
    (&PORTA.PIN0CTRL)[PICO_BOOTSEL_PIN] |= PORT_ISC_LEVEL_gc;
  }
}

int main() {
  // 20MHz / 16 = 1.25MHz
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, CLKCTRL_PEN_bm | CLKCTRL_PDIV_16X_gc);

  // Set pin directions
  PORTA.DIRCLR =
      (1 << PICO_BOOTSEL_PIN) | (1 << PICO_RUN_PIN) | (1 << TIMESEL_PIN);
  (&PORTA.PIN0CTRL)[PICO_BOOTSEL_PIN] |= PORT_PULLUPEN_bm;
  (&PORTA.PIN0CTRL)[TIMESEL_PIN] |= PORT_PULLUPEN_bm;

  // Setup pin change interrupt for PICO_BOOTSEL_PIN
  // set_int0_mode(true);
  (&PORTA.PIN0CTRL)[PICO_BOOTSEL_PIN] &= ~PORT_ISC_gm;
  (&PORTA.PIN0CTRL)[PICO_BOOTSEL_PIN] |= PORT_ISC_BOTHEDGES_gc;
  sei();

  bs2rst::init();

  // Sleep
  while (true) {
    bs2rst::service();
  }

  return 0;
}

ISR(PORTA_PORT_vect) {
  PORTA.INTFLAGS = PORT_INT0_bm << PICO_BOOTSEL_PIN;  // Clear interrupt flag
  bs2rst::bootsel_change();
}

ISR(TCA0_CMP0_vect) {
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;  // Clear interrupt flag
  bs2rst::timer_tick();
}

void bs2rst::timer_start() {
  // 1ms interval timer
  TCA0.SINGLE.PER = (F_CPU / 1000) - 1;  // 1.25MHz / 1000
  TCA0.SINGLE.CMP0 = (F_CPU / 1000) - 1;
  TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm | TCA_SINGLE_CLKSEL_DIV1_gc;
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP0_bm;
}

void bs2rst::timer_stop() { TCA0.SINGLE.CTRLA = 0; }

bool bs2rst::timesel_read() { return !(PORTA.IN & (1 << TIMESEL_PIN)); }

bool bs2rst::bootsel_read() { return !(PORTA.IN & (1 << PICO_BOOTSEL_PIN)); }

void bs2rst::reset_write(bool enable) {
  if (enable) {
    PORTA.OUTCLR = (1 << PICO_RUN_PIN);
    PORTA.DIRSET = (1 << PICO_RUN_PIN);
  } else {
    PORTA.DIRCLR = (1 << PICO_RUN_PIN);
  }
}

void bs2rst::cpu_sleep(bool deep) {
  // Recovery from power down state is only possible in level sensitivity
  // set_int0_mode(!deep);
  set_sleep_mode(deep ? SLEEP_MODE_PWR_DOWN : SLEEP_MODE_IDLE);
  sleep_enable();
  sleep_cpu();
  sleep_disable();
  // set_int0_mode(true);
}
