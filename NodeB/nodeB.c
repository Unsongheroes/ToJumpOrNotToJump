/// Node B receiver node
#include "JumpHeader.h"
#include "cc2420.h"

#include "dev/button-sensor.h"

#include "net/netstack.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
/* ----------------------------- Defines ----------------------------------- */
#define SEND_INTERVAL (8 * CLOCK_SECOND)
//static linkaddr_t addr_nodeB =     {{0x77, 0xb7, 0x7b, 0x11, 0x0, 0x74, 0x12, 0x00 }};
static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t addr_nodeA =     {{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};
static linkaddr_t addr_Sender;
/* -----------------------------         ----------------------------------- */
PROCESS(nodeB, "Node B - Sender");
AUTOSTART_PROCESSES(&nodeB);
/* ----------------------------- Helper ----------------------------------- */
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
  NETSTACK_NETWORK.output(src);
  LOG_INFO("Acknowledge sent!\n");
}

/* ----------------------------- Helper ----------------------------------- */
/* ----------------------------- Callbacks ----------------------------------- */
void ack_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) {
  uint8_t ack;

  memcpy(&ack, data, sizeof(ack));

  if (ack == 1) {
    
    LOG_INFO("Acknowledged received from: ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
    
    sendAck(&addr_Sender);
  }
}

void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{

  JumpPackage payload;
  memcpy(&payload, data, sizeof(payload));
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  
  printSender(payload);
  printReceiver(payload);
  printPayload(payload);

  /*
    LOG_INFO(" from address ");
    LOG_INFO_LLADDR(src);
    LOG_INFO(" sent to ");
    LOG_INFO_LLADDR(dest);
    LOG_INFO("\n");
  */
  addr_Sender = *src;
  NETSTACK_NETWORK.output(&addr_nodeC);
  
  nullnet_set_input_callback(ack_callback);
}

/* ----------------------------- Callbacks ----------------------------------- */


PROCESS_THREAD(nodeB, ev, data)
{
  static struct etimer periodic_timer;

  JumpPackage payload;
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  nullnet_set_input_callback(input_callback);
    
  PROCESS_BEGIN();
    printf("STARTING NODE B,,, \n");
    etimer_set(&periodic_timer, SEND_INTERVAL);

  SENSORS_ACTIVATE(button_sensor);
    while (1)
    {
        //nullnet_set_input_callback(input_callback);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 	// Wait until time is expired
        etimer_reset(&periodic_timer);

    }
 
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */

