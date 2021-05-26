/// Node C - intermediate node
/**
 * \file
 *         Application for deciding if a node should send directly to another or use another node to send through
 * \author
 *         Frederik Hasselholm Larsen
 */


#include "contiki.h"
#include "dev/button-sensor.h"
#include "net/nullnet/nullnet.h"
#include "net/netstack.h"
#include "JumpHeader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/energest.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
static unsigned long to_milliseconds(uint64_t time) // helper function to calcule energest values to milliseconds
{
  return (unsigned long)(time / 62.5 );
}
uint8_t checksum(uint8_t* buffer, uint8_t len) // function to calculate a naive checksum
{
      size_t i;
      uint8_t checksum = 0;
      /*
      linkaddr_t* sender = &buffer.sender;
      linkaddr_t* destination = &buffer.destination;
       for (i = 0; i < 8; i++)
      {
        checksum += sender[i];
        checksum += destination[i];
      } */
      for (i = 0; i < len; ++i) {
        checksum += buffer[i];
      }
            
      return checksum;
}

bool errorOrNot() { // introduction of a error probability of 50 %
  int r = rand() % 10;
  if (r > 10) {
    return true;
  } else {
    return false;
  }
}

bool checkChecksum(JumpPackage payload){ // function to determine if the received checksum matach the calculated checksum
  //LOG_INFO("Checking checksum: %i\n",payload.checksum );
  uint8_t pchecksum = checksum(payload.payload, payload.length);
  if(pchecksum == payload.checksum){
    //printf("Checksum correct\n");
    return true;
  }
  else{
    //printf("Checksum did not match payload\n");
    return false;
  }

}

void printSender(JumpPackage payload ) { // helper function to print the sender from the package
  LOG_INFO("Sender: " );
  linkaddr_t* sender = &payload.sender;
  LOG_INFO_LLADDR(sender);
  LOG_INFO("\n " );
  
}

void printReceiver(JumpPackage payload ) { // helper function to print the destination from the package
  LOG_INFO("Destination: " );
  linkaddr_t* destination = &payload.destination;
  LOG_INFO_LLADDR(destination);
  LOG_INFO("\n " );
  
}

void printPayload(JumpPackage payload) { // helper function to print the payload from the package
  size_t i;
  for ( i = 0; i < 64; i++)
  {
    if (payload.payload[i] ==0) {
      break;
    }
      LOG_INFO("Received message %u \n", payload.payload[i] );
  }
}

void sendAck(const linkaddr_t *src) { // function to send an acknowledge
  uint8_t acknowledge = 1;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  //LOG_INFO("\n");
  //LOG_INFO_LLADDR(src);
  //LOG_INFO("\n");
  linkaddr_t tmp = *src;
  NETSTACK_NETWORK.output(&tmp);
  //LOG_INFO("Acknowledge sent!\n");
}
 
void sendNack(const linkaddr_t *src) { // function to send a negative acknowledge
  uint8_t acknowledge = 0;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  linkaddr_t tmp = *src;
  NETSTACK_NETWORK.output(&tmp);
  //LOG_INFO("Not acknowledge sent!\n");
}

void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) // callback function implementation
{
  JumpPackage payload;
  memcpy(&payload, data, sizeof(payload));
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  
  if (payload.length > 0 ) { // payload received
    if(errorOrNot()){
      sendNack(src);
    }
    //printSender(payload);
    //printReceiver(payload);
    //printPayload(payload);
    if (checkChecksum(payload)) {
      sendAck(src);
    }
    else{
      sendNack(src);
    }
  } else { // ping received
      sendAck(src);
  }

   
}

/*---------------------------------------------------------------------------*/
PROCESS(main_process, "Main proces");
AUTOSTART_PROCESSES(&main_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(main_process, ev, data)
{
  PROCESS_BEGIN();
  printf("STARTING NODE C,,, \n");
  nullnet_set_input_callback(input_callback); // set callback function 

  SENSORS_ACTIVATE(button_sensor);

  while(1){
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    //measure and print energest times
    energest_flush();
    printf("\nEnergest:\n");
    printf("10ms *: CPU          %4lums LPM      %4lus DEEP LPM %4lums  Total time %lums\n",
        to_milliseconds(energest_type_time(ENERGEST_TYPE_CPU)),
        to_milliseconds(energest_type_time(ENERGEST_TYPE_LPM)),
        to_milliseconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
        to_milliseconds(ENERGEST_GET_TOTAL_TIME()));
    printf("10ms *: Radio LISTEN %4lums TRANSMIT %4lums OFF      %4lums\n",
        to_milliseconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
        to_milliseconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
        to_milliseconds(ENERGEST_GET_TOTAL_TIME()
                  - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                  - energest_type_time(ENERGEST_TYPE_LISTEN)));
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
