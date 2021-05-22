#include "JumpHeader.h"
#include "cc2420.h"

#include "dev/button-sensor.h"

#include "net/netstack.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int ACK_n = 0;

static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t cooja_nodeC = {{0x02, 0x02, 0x02, 0x00, 0x02, 0x74, 0x12, 0x00}};

void sendPayload(){
    uint8_t payloadData[64] = {1,2,3,4,5,6,7};
    JumpPackage payload = {{{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}},{{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}},payloadData,1,checksum(payloadData,7),7};
    nullnet_buf = (uint8_t *)&payload;

    nullnet_len = sizeof(payload);
  

    NETSTACK_NETWORK.output(&addr_nodeC);
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

        int i;

        for (i = 0; i < 100; i++)
        {
            sendPayload();
        }

        printf("Number of ACKs received: %i", ACK_n);
        ACK_n = 0;

        PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    }
    PROCESS_END();
}