#include "JumpHeader.h"
#include "cc2420.h"

#include "dev/button-sensor.h"

#include "net/netstack.h"

#include "sys/energest.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

static unsigned long to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

int checksum(uint8_t* buffer, size_t len)
{
      size_t i;
      int checksum = 0;
      for (i = 0; i < len; ++i) {
        checksum += buffer[i];
      }
            
      return checksum;
}

bool checkChecksum(JumpPackage payload){
  LOG_INFO("Checking checksum: %i\n",payload.checksum );
  int pchecksum = checksum(payload.payload, payload.length);
  if(pchecksum == payload.checksum){
    printf("Checksum correct\n");
    return true;
  }
  else{
    printf("Checksum did not match payload\n");
    return false;
  }
}

void printSender(JumpPackage payload ) {
  LOG_INFO("Sender: " );
  linkaddr_t* sender = &payload.sender;
  LOG_INFO_LLADDR(sender);
  LOG_INFO("\n " );
  
}

void printReceiver(JumpPackage payload ) {
  LOG_INFO("Destination: " );
  linkaddr_t* destination = &payload.destination;
  LOG_INFO_LLADDR(destination);
  LOG_INFO("\n " );
  
}

void printPayload(JumpPackage payload) {
  size_t i;
  for ( i = 0; i < 64; i++)
  {
    if (payload.payload[i] ==0) {
      break;
    }
      LOG_INFO("Received message %u \n", payload.payload[i] );
  }
}

void sendAck(const linkaddr_t *src) {
  uint8_t acknowledge = 1;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  LOG_INFO("\n");
  LOG_INFO_LLADDR(src);
  LOG_INFO("\n");
  linkaddr_t tmp = *src;
  NETSTACK_NETWORK.output(&tmp);
  LOG_INFO("Acknowledge sent!\n");
}

void sendNack(const linkaddr_t *src) {
  uint8_t acknowledge = 255;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  linkaddr_t tmp = *src;
  NETSTACK_NETWORK.output(&tmp);
  LOG_INFO("Not acknowledge sent!\n");
}


void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest){
    JumpPackage payload;
    memcpy(&payload, data, sizeof(payload));
    nullnet_buf = (uint8_t *)&payload;
    nullnet_len = sizeof(payload);
    printSender(payload);
    printReceiver(payload);
    printPayload(payload);
    if (checkChecksum(payload)) {
      sendAck(src);
    }
    else{
      sendNack(src);
    }
}

PROCESS(nodeC, "Node C - Receiver");
AUTOSTART_PROCESSES(&nodeC);

PROCESS_THREAD(nodeC, ev, data)
{
  static struct etimer periodic_timer;
    PROCESS_BEGIN();
    
    printf("STARTING NODE C,,, \n");
    nullnet_set_input_callback(input_callback);
       

    SENSORS_ACTIVATE(button_sensor);
    etimer_set(&periodic_timer, CLOCK_SECOND * 10);
    while (1){
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
      /* Update all energest times. */
        energest_flush();

        printf("\nEnergest:\n");
        printf(" CPU          %4lus LPM      %4lus DEEP LPM %4lus  Total time %lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
        printf(" Radio LISTEN %4lus TRANSMIT %4lus OFF      %4lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)));
    }
    PROCESS_END();
}