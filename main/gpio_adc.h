#ifndef __GPIO_ADC_H
#define __GPIO_ADC_H

#define I2C_SCL_PIN GPIO_NUM_22
#define I2C_SDA_PIN GPIO_NUM_21
#define I2C_CLOCK_FREQ 1000000 //max is 1MHz for ESP32 soooo let's just stick with it

#define BUTTON_PIN GPIO_NUM_18
#define BUTTON_TESTS 10


void gpio_setup();
void adc_setup();

void button_listener(void * ignore);
void adc_listener(void * ignore);
void bmp280_listener(void * pv);

void i2c_init_master();

#endif
