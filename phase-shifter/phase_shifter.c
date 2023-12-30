#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

#define FORTYKHZ_WRAP 3125
#define MICROSECONDS_PER_WAVELENGTH 25

void reset_counters() {
    pwm_set_mask_enabled(0x00000000);
    for (uint16_t index = 0; index < 30; index++)
    {
        uint slice = pwm_gpio_to_slice_num(index);
        pwm_set_counter(slice, 0); // empty PWM counter
    }
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void reduce_ringing(uint slice_num_zero, uint slice_num_two) {
	// Reduce ringing by one cycle of 180 degress out of phase
	uint slice_zero_counter = pwm_get_counter(slice_num_zero);
	uint slice_two_counter = pwm_get_counter(slice_num_two);
	uint value_to_set_to_zero  = (slice_zero_counter + 1562) % 3125;
	pwm_set_counter(slice_num_zero, value_to_set_to_zero);
	pwm_set_counter(slice_num_two, (slice_two_counter + 1562 % 3125));
	printf("counter: %d \t setting: %d\n", slice_zero_counter, value_to_set_to_zero);
	pwm_set_mask_enabled(0x000000FF);
	sleep_us(MICROSECONDS_PER_WAVELENGTH * .5);
    pwm_set_mask_enabled(0x00000000);
}

void pulse() {
	// Initial 8 pulses
	pwm_set_mask_enabled(0x000000FF);
	sleep_us(MICROSECONDS_PER_WAVELENGTH * 8);
    pwm_set_mask_enabled(0x00000000);
}

int main() {
    stdio_init_all();
    gpio_set_function(0, GPIO_FUNC_PWM);
    gpio_set_function(2, GPIO_FUNC_PWM);
    gpio_set_function(4, GPIO_FUNC_PWM);
    gpio_set_function(6, GPIO_FUNC_PWM);

    uint slice_num_zero = pwm_gpio_to_slice_num(0);
    uint slice_num_two = pwm_gpio_to_slice_num(2);
    uint slice_num_four = pwm_gpio_to_slice_num(4);
    uint slice_num_six = pwm_gpio_to_slice_num(6);

    pwm_set_wrap(slice_num_zero, FORTYKHZ_WRAP);
    pwm_set_wrap(slice_num_two, FORTYKHZ_WRAP);
    pwm_set_wrap(slice_num_four, FORTYKHZ_WRAP);
    pwm_set_wrap(slice_num_six, FORTYKHZ_WRAP);
    pwm_set_chan_level(slice_num_zero, PWM_CHAN_A, 1562);
    pwm_set_chan_level(slice_num_two, PWM_CHAN_A, 1562);
    pwm_set_chan_level(slice_num_four, PWM_CHAN_A, 1562);
    pwm_set_chan_level(slice_num_six, PWM_CHAN_A, 1562);
    reset_counters();
    pwm_set_mask_enabled(0x000000FF);
    adc_init();
    adc_gpio_init(28);
    adc_select_input(2);
    
    uint16_t current_value = 0;

    while(1)
    {
       uint16_t result = adc_read();
       long pwm_counter = map(result, 0, 4095, 0, 3125);
       printf("Raw: %d \t counter: %d\n", result, pwm_counter);
       if (result != current_value) {
          reset_counters();
          pwm_set_counter(slice_num_zero, pwm_counter);
           current_value = pwm_counter;
       }
		pulse();
//       reduce_ringing(slice_num_zero, slice_num_two);
        sleep_ms(20);
    }
}
