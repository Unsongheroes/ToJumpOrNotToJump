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

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

int checksum(uint8_t* buffer, size_t len)
{
      size_t i;
      int checksum = 0;
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

bool errorOrNot() {
  int r = rand() % 10;
  if (r < 5) {
    return true;
  } else {
    return false;
  }
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
  uint8_t acknowledge = 0;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  linkaddr_t tmp = *src;
  NETSTACK_NETWORK.output(&tmp);
  LOG_INFO("Not acknowledge sent!\n");
}

void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  JumpPackage payload;
  memcpy(&payload, data, sizeof(payload));
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  
  if (payload.length > 0 ) { // payload received
    if(errorOrNot()){
      sendNack(src);
    }
    printSender(payload);
    printReceiver(payload);
    printPayload(payload);
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
  nullnet_set_input_callback(input_callback);

  SENSORS_ACTIVATE(button_sensor);

  while(1){
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
