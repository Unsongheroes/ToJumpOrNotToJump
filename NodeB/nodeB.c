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
#define SEND_INTERVAL (3 * CLOCK_SECOND)
//static linkaddr_t addr_nodeB =     {{0x77, 0xb7, 0x7b, 0x11, 0x0, 0x74, 0x12, 0x00 }};
static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t addr_nodeA =     {{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};
/* -----------------------------         ----------------------------------- */
PROCESS(nodeB, "Node B - Sender");
AUTOSTART_PROCESSES(&nodeB);
/* ----------------------------- Helper ----------------------------------- */

void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{

  JumpPackage payload;
  memcpy(&payload.payload, data, sizeof(payload.payload));
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  size_t i;
  for ( i = 0; i < 64; i++)
  {
      LOG_INFO("Received message %u \n", payload.payload[i] );
  }
  LOG_INFO(" from address ");
  LOG_INFO_LLADDR(src);
  LOG_INFO(" sent to ");
  LOG_INFO_LLADDR(dest);
  LOG_INFO("\n");

  NETSTACK_NETWORK.output(&addr_nodeC);
  for ( i = 0; i < 64; i++)
  {
      LOG_INFO("Received message %u to node C \n", payload.payload[i] );
  }
}

PROCESS_THREAD(nodeB, ev, data)
{
  static struct etimer periodic_timer;

  JumpPackage payload;
  nullnet_buf = (JumpPackage *)&payload;
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

 
        printf("pulsing - \n");

    }
 
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */

