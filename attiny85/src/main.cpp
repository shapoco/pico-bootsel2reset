#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>

#include <stdint.h>

#include "bs2rst/bootsel2reset.hpp"

// clang-format off
//        RESETn dW PCINT5 ADC0 PB5 -|￣￣￣￣|- VCC
// XTAL1 CLKI OC1Bn PCINT3 ADC3 PB3 -|　　　　|- PB2 ADC1      PCINT2 SCK USCK SCL T0 INT0
// XTAL2 CLKO OC1B  PCINT4 ADC2 PB4 -|　　　　|- PB1      AIN1 PCINT1 MISO DO     OC0B OC1A
//                              GND -|＿＿＿＿|- PB0 AREF AIN0 PCINT0 MOSI DI SDA OC0A OC1An
// clang-format on

static constexpr uint8_t PICO_BOOTSEL_PIN = PB2;
static constexpr uint8_t PICO_RUN_PIN = PB3;
static constexpr uint8_t TIMESEL_PIN = PB4;

static void set_int0_mode(bool edge) {
  if (edge) {
    // edge-sensitive
    MCUCR &= (1 << ISC01);
    MCUCR |= (1 << ISC00);
  } else {
    // level-sensitive
    MCUCR &= ~((1 << ISC01) | (1 << ISC00));
  }
}

int main() {
  CLKPR = (1 << CLKPCE);  // Enable change of the clock prescaler
  CLKPR = 3;              // Set clock prescaler to divide by 8 (1MHz)

  // Set pin directions
  DDRB &= ~((1 << PICO_BOOTSEL_PIN) | (1 << PICO_RUN_PIN) | (1 << TIMESEL_PIN));
  PORTB |= (1 << PICO_BOOTSEL_PIN) | (1 << TIMESEL_PIN);  // Enable pull-ups

  // Setup pin change interrupt for PICO_BOOTSEL_PIN
  set_int0_mode(true);
  GIMSK |= (1 << INT0);

  // Initialize state machine
  bs2rst::init();

  sei();

  while (true) {
    bs2rst::service();
  }

  return 0;
}

ISR(INT0_vect) { bs2rst::bootsel_change(); }

ISR(TIMER0_COMPA_vect) { bs2rst::timer_tick(); }

void bs2rst::timer_start() {
  TCCR0A = (1 << WGM01);  // CTC mode
  TCCR0B = (1 << CS01);   // Prescaler: clk/8
  OCR0A = F_CPU / 8 / 1000 - 1;
  TCNT0 = 0;
  TIMSK |= (1 << OCIE0A);
}

void bs2rst::timer_stop() {
  TIMSK &= ~(1 << OCIE0A);
  TCCR0A = 0;
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
