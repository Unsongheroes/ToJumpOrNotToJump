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
#define SEND_INTERVAL (10 * CLOCK_SECOND)
static linkaddr_t addr_nodeB =     {{0xe3, 0xfd, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t addr_nodeA =     {{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};

static bool Acknowledged = 0;
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

void Radio_signal_strength(JumpPackage payload)
{
  static signed char rss;
  static signed char rss_val;
  static signed char rss_offset;
  printf("Got message from %d\n",payload.sender);
  rss_val = cc2420_last_rssi;
  rss_offset=-45;
  rss=rss_val + rss_offset;
  printf("RSSI of Last Packet Received is %d\n",rss);
}

int findBestChannel() {
    
    int localBestMean = 0; // variable to hold the best value for each channel
    int newMeasure = 0; // variable to hold the current value for each channel
    int globalBest = 0; // variable to hold the best value for all channels
    int bestChannel = 0; // variable to hold the best channel number

    int i,j;

    for (i = 0; i < 16; i++) // for each channel
    {
      cc2420_set_channel(i+11); // set channel
      int localSum = 0;
      for (j = 0; j < 5; j++) // take 5 measurement for each channel
      {
          newMeasure = cc2420_rssi(); // get new RSSI value, by using this function we already have it in dBm since it adds the offset internally. 
          localSum += newMeasure; // take a new measurement
          //printf("Channel: %i - new measure: %i\n", i+11,newMeasure);
      }
      localBestMean = localSum / 5;
      printf("Channel: %i - mean: %i\n", i+11,localBestMean);
        if (i == 0) {
          globalBest = localBestMean; // save the best measure for the first channel
          bestChannel = i+11; // save the best channel number
        } else {
          if(localBestMean > globalBest){
            globalBest = localBestMean;  // if better then set the best value for the current iteration
            bestChannel = i+11; // save the best channel number
          }
        }
    }
    printf("Best: %i dBm on channel: %i\n", globalBest, bestChannel);
    return bestChannel;
}

void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{
  uint8_t ack;

  memcpy(&ack, data, sizeof(ack));

  if (ack == 1) {
    Acknowledged = 1;
    LOG_INFO("Acknowledged received from: ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
  }
}

PROCESS_THREAD(nodeA, ev, data)
{
  static struct etimer periodic_timer;

  uint8_t payloadData[64] = {1,2,3,4,5,6,7};
  JumpPackage payload = {{{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}},{{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}},{1,2,3,4,5,6,7},checksum(payloadData,7),7};
  nullnet_buf = (uint8_t *)&payload;

  nullnet_len = sizeof(payload);
  nullnet_set_input_callback(input_callback);
    
  PROCESS_BEGIN();
    printf("STARTING NODE A,,, \n");  
    etimer_set(&periodic_timer, SEND_INTERVAL);

  SENSORS_ACTIVATE(button_sensor);
    while (1)
    {
//        nullnet_set_input_callback(input_callback);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 	// Wait until time is expired
        etimer_reset(&periodic_timer);
        if(!Acknowledged ) {
          

          NETSTACK_NETWORK.output(&addr_nodeB);
          size_t i;
          for ( i = 0; i < 64; i++)
          {
            if(payload.payload[i]==0) {
            break;
            }
            LOG_INFO("Sent message %u \n ", payload.payload[i] );
          }  
        } else {
          LOG_INFO("Message delivered!\n");
          PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
          Acknowledged = 0;
        }
    }
 
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */
