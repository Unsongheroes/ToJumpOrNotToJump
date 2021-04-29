/// Node A - Sender node
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
#define SEND_INTERVAL (3 * CLOCK_SECOND)
static linkaddr_t addr_nodeB =     {{0xe3, 0xfd, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t addr_nodeA =     {{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};

/* -----------------------------         ----------------------------------- */
PROCESS(nodeA, "Node A - Sender");
AUTOSTART_PROCESSES(&nodeA);
/* ----------------------------- Helper ----------------------------------- */

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
  JumpPackage payload;

  memcpy(&payload.payload, data, sizeof(payload));
  size_t i;
  for ( i = 0; i < 64; i++)
  {
      LOG_INFO("Received message %u \n ", payload.payload[i] );
  }
  LOG_INFO(" from address ");
  LOG_INFO_LLADDR(src);
  LOG_INFO(" sent to ");
  LOG_INFO_LLADDR(dest);
  LOG_INFO("\n");
}

PROCESS_THREAD(nodeA, ev, data)
{
  static struct etimer periodic_timer;

  JumpPackage payload[64] = {{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00},{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00},{33}};
  nullnet_buf = (JumpPackage *)&payload;
  nullnet_len = sizeof(payload);
  nullnet_set_input_callback(input_callback);
    
  PROCESS_BEGIN();
    printf("STARTING NODE A,,, \n");
    etimer_set(&periodic_timer, SEND_INTERVAL);

  SENSORS_ACTIVATE(button_sensor);
    while (1)
    {
        nullnet_set_input_callback(input_callback);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 	// Wait until time is expired
        etimer_reset(&periodic_timer);

        NETSTACK_NETWORK.output(&addr_nodeB);

        printf("sending message: %u\n", payload);
        LOG_INFO("TEST\n");
        LOG_INFO_LLADDR(&addr_nodeB);
    }
 
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */
