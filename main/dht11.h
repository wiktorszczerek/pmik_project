#ifndef __DHT11_H
#define __DHT11_H

#define DHT11_PIN GPIO_NUM_17
#define DHT11_INTERVAL 1200 //1.2 sec interval, everything should be okay after this time
#define DHT11_RESPONSE_TIME_MAX 5200 //5.2ms, or 5200us
#define US_SAFE_VALUE 7 //it was needed, cuz there is a problem with DHT11 setting up
enum dht11_states{DHT11_OK,DHT11_TIME_EXCEEDED,DHT11_CHECKSUM_ERROR};

void dht11_listener(void * ignore);

#endif
