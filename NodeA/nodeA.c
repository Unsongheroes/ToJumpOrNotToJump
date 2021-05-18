/// Node A - Sender node
#include "JumpHeader.h"
#include "cc2420.h"

#include "dev/button-sensor.h"
/* #include "lib/random.h"
#include "core/net/rime.h"
#include "contiki/core/net/rime/collect.h" */
#include "net/netstack.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
/* ----------------------------- Defines ----------------------------------- */
#define SEND_INTERVAL (4 * CLOCK_SECOND)
#define RSSI_THRESHOLD -10

 static linkaddr_t addr_nodeB =     {{0xe3, 0xfd, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};

// static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t addr_broadCast = {{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}};

//static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};

//static linkaddr_t addr_nodeA =     {{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t cooja_nodeA = {{0x01, 0x01, 0x01, 0x00, 0x01, 0x74, 0x12, 0x00}};
// static linkaddr_t cooja_nodeC = {{0x02, 0x02, 0x02, 0x00, 0x02, 0x74, 0x12, 0x00}};

static bool Acknowledged = 0;
static bool Pinging = true;
//static clock_time_t PingStartBtime = 0;
static clock_time_t PayloadStartCtime = 0;
//static clock_time_t PingEndBtime = 0;
static clock_time_t PayloadEndCtime = 0;
/* static clock_time_t PayloadStartBtime = 0;
static clock_time_t PayloadStartCtime = 0;
static clock_time_t PayloadEndBtime = 0;
static clock_time_t PayloadEndCtime = 0;
 */
/* -----------------------------         ----------------------------------- */
PROCESS(nodeA, "Node A - Sender");
AUTOSTART_PROCESSES(&nodeA);
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

bool Radio_signal_strength( const linkaddr_t *src)
{
  static signed char rss;
  static signed char rss_val;
  static signed char rss_offset;
  printf("Got message from: ");
  LOG_INFO_LLADDR(src);
  printf("\n");
  rss_val = cc2420_last_rssi;
  rss_offset=-45;
  rss=rss_val + rss_offset;
  printf("RSSI of Last Packet Received is %d\n",rss);
  if(rss > RSSI_THRESHOLD && rss < 0) {
    return true;
  } else {
    return false;
  }
  
}

void sendPayload(linkaddr_t dest) {
  uint8_t payloadData[64] = {1,2,3,4,5,6,7};
  JumpPackage payload = {{{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}},{{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}},{1,2,3,4,5,6,7},checksum(payloadData,7),7};
  nullnet_buf = (uint8_t *)&payload;

  nullnet_len = sizeof(payload);
  
  PayloadStartCtime = clock_time();
  printf("ping! %lu \n", PayloadStartCtime);
  
  NETSTACK_NETWORK.output(&dest);
  size_t i;
  for ( i = 0; i < 64; i++)
  {
    if(payload.payload[i]==0) {
    break;
    }
    LOG_INFO("Sent message %u \n ", payload.payload[i] );
  }
}

void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{
  PayloadEndCtime = clock_time();
  printf("payload end: %lu \n", PayloadEndCtime);
  LOG_INFO("payload time: %lu \n", PayloadEndCtime - PayloadStartCtime);
 //  uint8_t ack;
 /*  memcpy(&ack, data, sizeof(ack));
  if (Pinging) {
    if (Radio_signal_strength(src)) {
      Pinging = false;
      sendPayload(addr_nodeC);
    } else {
      Pinging = false;
      sendPayload(addr_nodeB);
    }
  } else {
    
    if (ack == 1) {
      Acknowledged = 1;
      LOG_INFO("Acknowledged received from: ");
      LOG_INFO_LLADDR(src);
      LOG_INFO_("\n");
    } else if (ack == 0) {
      LOG_INFO("Not acknowledged received from: ");
      LOG_INFO_LLADDR(src);
      LOG_INFO_("\n");
    }
  } */
  
}


PROCESS_THREAD(nodeA, ev, data)
{
  static struct etimer periodic_timer;

  
    
  PROCESS_BEGIN();
    clock_init();
    printf("STARTING NODE A,,, \n");  
    etimer_set(&periodic_timer, SEND_INTERVAL);
    nullnet_set_input_callback(input_callback);
    SENSORS_ACTIVATE(button_sensor);
    while (1)
    {
//        nullnet_set_input_callback(input_callback);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 	// Wait until time is expired
        etimer_reset(&periodic_timer);
        if (Pinging) {
          
          //uint8_t payloadData = 1;
          //nullnet_buf = (uint8_t *)&payloadData;

          //nullnet_len = sizeof(payloadData);
          //nullnet_set_input_callback(input_callback);

          sendPayload(addr_nodeB);
          //NETSTACK_NETWORK.output(&addr_nodeC);

        } else if(!Acknowledged ) {
            //sendPayload(addr_nodeC);
        }  else {

          LOG_INFO("Message delivered!\n");
          // PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
          Acknowledged = 0;
        }
    }
 
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */
