#ifndef __GPIO_ADC_H
#define __GPIO_ADC_H

#define BUTTON_PIN GPIO_NUM_18
#define BUTTON_TESTS 10


void gpio_setup();
void adc_setup();

void button_listener(void * ignore);
void adc_listener(void * ignore);

#endif
