/// Node B receiver node
#include "contiki.h"
#include "cc2420.h"

#include "dev/button-sensor.h"

#include "net/nullnet/nullnet.h"
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

/* -----------------------------         ----------------------------------- */
PROCESS(nodeB, "Node B - Sender");
AUTOSTART_PROCESSES(&nodeB);
/* ----------------------------- Helper ----------------------------------- */

void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{

  unsigned count;
  memcpy(&count, data, sizeof(count));
  nullnet_buf = (uint8_t *)&count;
  nullnet_len = sizeof(count);
  LOG_INFO("Received message %u from address ", count );
  LOG_INFO_LLADDR(src);
  LOG_INFO(" sent to ");
  LOG_INFO_LLADDR(dest);
  LOG_INFO("\n");

  NETSTACK_NETWORK.output(&addr_nodeC);

  LOG_INFO("sending message %u to Node C \n", count );
  
}

PROCESS_THREAD(nodeB, ev, data)
{
  static struct etimer periodic_timer;

  static unsigned count = 0;

  nullnet_buf = (uint8_t *)&count;
  nullnet_len = sizeof(count);
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

