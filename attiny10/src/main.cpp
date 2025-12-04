#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include <stdint.h>

#include "bs2rst/bootsel2reset.hpp"

// clang-format off
// PB0 1 -|￣￣￣￣|- 6 PB3
// GND 2 -|　　　　|- 5 VCC
// PB1 3 -|＿＿＿＿|- 4 PB2
//
// 1 PB0 (PCINT0/TPIDATA/OC0A/ADC0/AIN0)
// 2 GND
// 3 PB1 (PCINT1/TPICLK/CLKI/ICP0/OC0B/ADC1/AIN1)
// 4 PB2 (T0/CLKO/PCINT2/INT0/ADC2)
// 5 VCC
// 6 PB3 (RESET/PCINT3/ADC3)
// clang-format on

static constexpr uint8_t PICO_BOOTSEL_PIN = PB2;
static constexpr uint8_t PICO_RUN_PIN = PB1;
static constexpr uint8_t TIMESEL_PIN = PB0;

static void set_int0_mode(bool edge) {
  if (edge) {
    // edge-sensitive
    EICRA &= (1 << ISC01);
    EICRA |= (1 << ISC00);
  } else {
    // level-sensitive
    EICRA &= ~((1 << ISC01) | (1 << ISC00));
  }
}

int main() {
  CLKPSR = 3;  // Set clock prescaler to divide by 8 (1.5MHz)

  // Set pin directions
  DDRB &= ~((1 << PICO_BOOTSEL_PIN) | (1 << PICO_RUN_PIN) | (1 << TIMESEL_PIN));
  PUEB |= (1 << PICO_BOOTSEL_PIN) | (1 << TIMESEL_PIN);  // Enable pull-ups

  // Setup pin change interrupt for PICO_BOOTSEL_PIN
  set_int0_mode(true);
  EIMSK |= (1 << INT0);

  // Initialize state machine
  bs2rst::init();

  sei();

  while (true) {
    bs2rst::service();
  }

  return 0;
}

ISR(INT0_vect) { bs2rst::bootsel_change(); }

ISR(TIM0_COMPA_vect) { bs2rst::timer_tick(); }

void bs2rst::timer_start() {
  TCCR0B = (1 << WGM02) | (1 << CS01);  // CTC mode, Prescaler: clk/8
  OCR0A = F_CPU / 8 / 1000 - 1;
  TCNT0 = 0;
  TIMSK0 |= (1 << OCIE0A);
}

void bs2rst::timer_stop() {
  TIMSK0 &= ~(1 << OCIE0A);
  TCCR0B = 0;
}

bool bs2rst::timesel_read() { return !(PINB & (1 << TIMESEL_PIN)); }

bool bs2rst::bootsel_read() { return !(PINB & (1 << PICO_BOOTSEL_PIN)); }

void bs2rst::reset_write(bool enable) {
  if (enable) {
    PORTB &= ~(1 << PICO_RUN_PIN);
    DDRB |= (1 << PICO_RUN_PIN);
  } else {
    DDRB &= ~(1 << PICO_RUN_PIN);
  }
}

void bs2rst::cpu_sleep(bool deep) {
  // Recovery from power down state is only possible in level sensitivity
  set_int0_mode(!deep);
  set_sleep_mode(deep ? SLEEP_MODE_PWR_DOWN : SLEEP_MODE_IDLE);
  sleep_enable();
  sleep_cpu();
  sleep_disable();
  set_int0_mode(true);
}
