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

using namespace bs2rst;

static constexpr uint8_t PICO_BOOTSEL_PIN = PB2;
static constexpr uint8_t PICO_RESET_PIN = PB4;

static uint16_t push_time_ms = 0;

int main() {
  CLKPR = (1 << CLKPCE);  // Enable change of the clock prescaler
  CLKPR = 3;              // Set clock prescaler to divide by 8 (1MHz)

  // Set pin directions
  DDRB &= ~((1 << PICO_BOOTSEL_PIN) | (1 << PICO_RESET_PIN));
  PORTB |= (1 << PICO_BOOTSEL_PIN);

  // Setup pin change interrupt for PICO_BOOTSEL_PIN
  MCUCR |= (1 << ISC01) | (1 << ISC00);
  GIMSK |= (1 << INT0);
  sei();

  // Start Timer0 for 1ms tick
  TCCR0A = (1 << WGM01);         // CTC mode
  TCCR0B = (1 << CS01);          // Prescaler: clk/64
  OCR0A = F_CPU / 8 / 1000 - 1;  // Compare value for 1ms
  TCNT0 = 0;
  TIMSK |= (1 << OCIE0A);  // Enable timer compare interrupt

  // Sleep
  while (true) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
  }

  return 0;
}

ISR(INT0_vect) {
  if (push_time_ms < SHORT_PUSH_TIME_MS) {
    push_time_ms = 0;
  }
}

ISR(TIMER0_COMPA_vect) {
  bool pressed = !(PINB & (1 << PICO_BOOTSEL_PIN));
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

  if (reset_enable) {
    PORTB &= ~(1 << PICO_RESET_PIN);
    DDRB |= (1 << PICO_RESET_PIN);
  } else {
    DDRB &= ~(1 << PICO_RESET_PIN);
  }
}
