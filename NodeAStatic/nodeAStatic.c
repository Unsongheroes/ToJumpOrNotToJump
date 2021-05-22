#include "JumpHeader.h"
#include "cc2420.h"

#include "dev/button-sensor.h"

#include "net/netstack.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

int ACK_n = 0;

//static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
static linkaddr_t cooja_nodeC = {{0x02, 0x02, 0x02, 0x00, 0x02, 0x74, 0x12, 0x00}};

int checksum(uint8_t* buffer, size_t len)
{
      size_t i;
      int checksum = 0;
      for (i = 0; i < len; ++i) {
        checksum += buffer[i];
      }
            
      return checksum;
}

void sendPayload(){
    uint8_t payloadData[64] = {1,2,3,4,5,6,7};
    JumpPackage payload;
    payload.sender = (linkaddr_t){{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};
    payload.destination = (linkaddr_t){{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
    memcpy(payload.payload,payloadData,64);
    payload.sequenceNumber = 1;
    payload.checksum = checksum(payloadData,7);
    payload.length = 7;
    nullnet_buf = (uint8_t *)&payload;

    nullnet_len = sizeof(payload);
  

    NETSTACK_NETWORK.output(&cooja_nodeC);
    size_t i;
    for ( i = 0; i < 64; i++)
    {
      if(payload.payload[i]==0) {
      break;
    }
    LOG_INFO("Sent message %u \n ", payload.payload[i] );
  }
}


void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest){
    uint8_t ack;
    memcpy(&ack, data, sizeof(ack));
    if (ack == 1) {
      LOG_INFO("Acknowledged received from: ");
      LOG_INFO_LLADDR(src);
      LOG_INFO_("\n");
      ACK_n++;
    } else if (ack == 255) {
      LOG_INFO("Not acknowledged received from: ");
      LOG_INFO_LLADDR(src);
      LOG_INFO_("\n");
    }
}

PROCESS(nodeA, "Node A - Sender");
AUTOSTART_PROCESSES(&nodeA);

PROCESS_THREAD(nodeA, ev, data)
{
    PROCESS_BEGIN();
    
    printf("STARTING NODE A,,, \n"); 
    nullnet_set_input_callback(input_callback);

    SENSORS_ACTIVATE(button_sensor);
    
    while(1){

        sendPayload();

        PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
        printf("Number of ACKs received: %i", ACK_n);
    }
    PROCESS_END();
}