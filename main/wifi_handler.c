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

#define WIFI_SSID "UPC2479847"
#define WIFI_PASSWORD "4nchuyxrHfzU"

#define SERVER_ADDRESS "192.168.0.66" //server will have ONE AND ONLY.

#define WIFI_MAX_RETRY 30

#define RX_BUFFSIZE_BYTES 128 //we have to agree on something
#define START_PORT 10100

static const char *TAG = "WIFI_HANDLER";
TaskHandle_t socket_client_handle = NULL;

char* rx_data = NULL;

int sock =0;

static EventGroupHandle_t wifi_event_group;
const int IPV4_GOTIP_BIT = BIT0;
//static int s_retry_num = 0;

int got_ip = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, IPV4_GOTIP_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        got_ip=1;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, IPV4_GOTIP_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_sta()
{
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD
        },
    };
	 
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
	 ESP_LOGI(TAG,"SSID set is: %s",wifi_config.sta.ssid);
}

void wait_for_ip()
{
    uint32_t bits = IPV4_GOTIP_BIT;

    ESP_LOGI(TAG, "Waiting for AP connection...");
    xEventGroupWaitBits(wifi_event_group, bits, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");
}


void socket_client(void *pvParameters)
{
	int rc = 0;
	while(1)
	{
		ESP_LOGI(TAG, "Socket client started");
		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		
		if(sock < 0)
		{
			ESP_LOGE(TAG,"Unable to create socket: ERROR");
			break;
		}
		ESP_LOGD(TAG, "socket: rc: %d", sock);
		struct sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		inet_pton(AF_INET, SERVER_ADDRESS, &serverAddress.sin_addr.s_addr);
		serverAddress.sin_port = htons(START_PORT);

		rc = connect(sock, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr_in));
		if (rc != 0) {
      	ESP_LOGE(TAG, "Socket unable to connect: ERROR");
      }
		ESP_LOGD(TAG, "connect rc: %d", rc);
		

		while (1) 	
		{
			int recv_len = recv(sock, rx_data, sizeof(rx_data) - 1, 0);
			if(got_ip)
			{
				//ESP_LOGI("data", "%s", RXdata);

				//handleData(RXdata, strlen(RXdata));

				if (recv_len < 0)
				{
					ESP_LOGD("WRONG READ SIZE", "");
					break;
				}				
			}
		}
	  if (sock != -1) 
	  {
	   close(sock);
     }
	} 
	vTaskDelete(NULL);
}

int send_data(char * data)
{	
	int rc;
	rc = send(sock, data, strlen(data), 0);
	//ESP_LOGI(tag, "send: rc: %d, data: %s", rc, data);
	ESP_LOGI(TAG, "sent: %s", data);
	return rc;
}

void start_socket_client()
{
	rx_data = (char *) calloc(RX_BUFFSIZE_BYTES,sizeof(char));
	
	xTaskCreate(socket_client, "socket_task", 1024*20, NULL, tskIDLE_PRIORITY, &socket_client_handle);
	configASSERT(socket_client_handle);
	
}

int get_got_ip()
{
	return got_ip;
}
