/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "wifi_handler.h"
#include "gpio_adc.h"

static const char *TAG = "Main";

TaskHandle_t x_button = NULL;
TaskHandle_t x_adc = NULL;
TaskHandle_t x_dht11 = NULL;
TaskHandle_t x_temp = NULL; //for deleting previous ones


void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "WIFI STATION, nvs init went flawlessly");
    
    //wifi_init_sta();
    //wait_for_ip();
    //while(get_got_ip() == 0)
    	//vTaskDelay(100 / portTICK_PERIOD_MS);		
	
	vTaskDelay(2000 / portTICK_PERIOD_MS);
    
  //  start_socket_client();
    
    gpio_setup();
    adc_setup();
    
    ESP_LOGI(TAG,"Creating listeners");
    //xTaskCreate(button_listener,"button",5*1024,NULL,10,&x_button);
    //xTaskCreate(adc_listener,"adc",5*1024,NULL,10,&x_adc);
    xTaskCreate(dht11_listener,"dht11",5*1024,NULL,10,&x_dht11);
    
    while(1)
    {
    };

}
