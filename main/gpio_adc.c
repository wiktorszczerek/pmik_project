#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/i2c.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

#include "wifi_handler.c"
#include "gpio_adc.h"
#include <bmp280.h> //as a part of esp-idf-lib


static const char *gpTAG = "GPIO_ADC_C";

/*
				I2C
*/

void i2c_init_master()
{
	i2c_config_t i2c_config = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = I2C_SDA_PIN,
		.scl_io_num = I2C_SCL_PIN,
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = I2C_CLOCK_FREQ
	};
	i2c_param_config(I2C_NUM_0,&i2c_config);
	i2c_driver_install(I2C_NUM_0,I2C_MODE_MASTER,0,0,0);
}


/*
				GPIO Part
*/


int button_state = 0;


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

void bmp280_listener(void* pv)
{
    bmp280_params_t params;
    bmp280_init_default_params(&params); //we don't want anything extra
    bmp280_t bmp280_type;
    memset(&bmp280_type,0,sizeof(bmp280_t)); //zero the structure!
    
    ESP_ERROR_CHECK(bmp280_init_desc(&bmp280_type,BMP280_I2C_ADDRESS_0,0,I2C_SDA_PIN,I2C_SCL_PIN));
    //ESP_ERROR_CHECK(bmp280_init(&bmp280_type,&params)); sth is wrong with the library, ignoring for now
    bmp280_init(&bmp280_type,&params);
    ESP_LOGI(gpTAG,"Found BMx280 device: %s",(bmp280_type.id == BME280_CHIP_ID)?"BME280":"BMP280"); //in BME there is humidity sensor instead of pressure
   
    float pressure, temperature, dummy;
    float pressure_holder;
    while(1)
    {
        if (bmp280_read_float(&bmp280_type, &temperature, &pressure,&dummy) != ESP_OK)
        {
            printf("Temperature/pressure reading failed\n");
            continue;
        }
        else 
        {
            pressure_holder=pressure/100;
            ESP_LOGE("Read values","temp: %.2f C // pres: %.2f hPa",temperature,pressure_holder);
        }
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}



