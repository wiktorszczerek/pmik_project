#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

#include "dht11.h"

float temperature = 0;
float humidity = 0;

int dht11_check_level_over_period(int us, int level)
{
	int diff = 0;
	uint64_t tick = (uint64_t)esp_timer_get_time();
	while(gpio_get_level(DHT11_PIN) == level)
	{
		diff = (uint64_t)esp_timer_get_time() - tick;
	}
	if(diff > (us+US_SAFE_VALUE))
	{
		ESP_LOGE("time was","%d",diff);
		return -1;
	}
	return diff;
}

int dht11_start()
{
	gpio_pullup_en(DHT11_PIN);
	vTaskDelay(DHT11_INTERVAL/portTICK_PERIOD_MS);
	gpio_set_direction(DHT11_PIN,GPIO_MODE_OUTPUT);
	gpio_set_level(DHT11_PIN,0);
	ets_delay_us(20 * 1000);
	gpio_set_level(DHT11_PIN,1);
	gpio_set_direction(DHT11_PIN,GPIO_MODE_INPUT);
	ets_delay_us(40);
	if(dht11_check_level_over_period(80,0) == -1)
		return DHT11_TIME_EXCEEDED;
	if(dht11_check_level_over_period(80,1) == -1)
		return DHT11_TIME_EXCEEDED;
	return DHT11_OK;
}

uint8_t make_byte_from_bits(uint8_t* arr)
{
	return ((arr[0]!=0)<<7 | (arr[1]!=0) << 6 | (arr[2]!=0) << 5 | (arr[3]!=0) << 4 | (arr[4]!=0) << 3 | (arr[5]!=0) << 2 | (arr[6]!=0) << 1 | (arr[7]!=0) << 0);
}


int dht11_process_data()
{
	uint8_t data[40] = {0};
	for(int i=0;i<40;i++)
	{
		if(dht11_check_level_over_period(50,0) == -1)
		{
			return DHT11_TIME_EXCEEDED;
		}
		data[i]=(dht11_check_level_over_period(70,1) > 28)?1:0;
	}
	uint8_t data_uint[5] = {0};
	for(int i=0;i<5;++i)
	{
		data_uint[i] = make_byte_from_bits(data+(i*8));
	}
	if(data_uint[0]+data_uint[1]+data_uint[2]+data_uint[3] != data_uint[4]) return DHT11_CHECKSUM_ERROR;
	
	temperature = data_uint[2];
	if(data_uint[3] & 0x80)
	{
		temperature = -1 - temperature;
	}
	temperature  += (data_uint[3] & 0x0f) * 0.1;
	
	humidity = data_uint[0]+data_uint[1]*0.1;
	return DHT11_OK;
}


void dht11_listener(void * ignore)
{
	while(dht11_start() != DHT11_OK) 
	{
		ESP_LOGE("DHT11","Unsuccessful setup! Trying to connect again...");
		vTaskDelay(DHT11_INTERVAL/portTICK_PERIOD_MS);
	}
	dht11_process_data();	
	while(1)
	{
		while(dht11_start() != DHT11_OK) 
		{
			ESP_LOGE("DHT11","Unsuccessful setup! Trying to connect again...");
			vTaskDelay(DHT11_INTERVAL/portTICK_PERIOD_MS);
		}
		dht11_process_data();
		ESP_LOGI("DHT11","temperature: %.2f || humidity: %.2f ",temperature,humidity);
	}
}

