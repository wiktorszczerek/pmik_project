#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

#include "wifi_handler.c"
#include "gpio_adc.h"


/*
				GPIO Part
*/


int button_state = 0;

float temperature = 0;
float humidity = 0;


void gpio_setup()
{
	gpio_pullup_en(BUTTON_PIN);
	gpio_set_direction(BUTTON_PIN,GPIO_MODE_INPUT);
}


void button_listener(void * ignore)
{
	int val=0;
	int old_val=0;
	while(1)
	{
		int tests = 0;
		for(int i=0;i<BUTTON_TESTS;i++)
		{
			old_val = val;
			val=gpio_get_level(BUTTON_PIN);
			if(val == old_val) 
			{
				tests++;
			}
			vTaskDelay(10/portTICK_PERIOD_MS);
		}
		if(tests == BUTTON_TESTS)
		{	
			button_state=val;
			ESP_LOGD(TAG,"you pressed me: %d",val);
		}
	}
	vTaskDelete(NULL);	
}

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
	uint64_t tick = 0;
	uint64_t difference = 0;
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

/*
				ADC Part
*/


#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;

void adc_setup()
{
    if (unit == ADC_UNIT_1) {
        adc1_config_width(ADC_WIDTH_BIT_12);
        adc1_config_channel_atten(channel, atten);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }
    
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
}

void adc_listener(void * ignore)
{
	while (1) 
	{
	
		char* sendBuffer = (char*)malloc( 13 );
     uint32_t adc_reading = 0;
     //Multisampling
     for (int i = 0; i < NO_OF_SAMPLES; i++) {
         if (unit == ADC_UNIT_1) {
             adc_reading += adc1_get_raw((adc1_channel_t)channel);
         } else {
             int raw;
             adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
             adc_reading += raw;
         }
     }
     adc_reading /= NO_OF_SAMPLES;
     
     //Convert adc_reading to voltage in mV (for 12b is 4095 - 3.3V i guess)
     uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
     
     ESP_LOGD("Read voltage","%d",voltage);
     
     sprintf(sendBuffer,"#AV!%08X&",voltage);
     send_data(sendBuffer);
     sprintf(sendBuffer,"#PI&");
     send_data(sendBuffer);
     
     vTaskDelay(pdMS_TO_TICKS(1000));
     free(sendBuffer);
 }
}




