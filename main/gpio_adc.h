#ifndef __GPIO_ADC_H
#define __GPIO_ADC_H

#define BUTTON_PIN GPIO_NUM_18
#define BUTTON_TESTS 10

#define DHT11_PIN GPIO_NUM_17
#define DHT11_INTERVAL 1200 //1.2 sec interval, everything should be okay after this time
#define DHT11_RESPONSE_TIME_MAX 5200 //5.2ms, or 5200us
#define US_SAFE_VALUE 6
enum dht11_states{DHT11_OK,DHT11_TIME_EXCEEDED,DHT11_CHECKSUM_ERROR};

void gpio_setup();
void adc_setup();

void button_listener(void * ignore);
void adc_listener(void * ignore);
void dht11_listener(void * ignore);

#endif
