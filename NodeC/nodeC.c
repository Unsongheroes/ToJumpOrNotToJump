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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define SEND_INTERVAL (3 * CLOCK_SECOND)

void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  unsigned count;
  memcpy(&count, data, sizeof(count));
  LOG_INFO("Received message %u from address ", count );
  LOG_INFO_LLADDR(src);
  LOG_INFO_(" sent to ");
  LOG_INFO_LLADDR(dest);
  LOG_INFO_("\n");

}

/*---------------------------------------------------------------------------*/
PROCESS(main_process, "Main proces");
AUTOSTART_PROCESSES(&main_process);
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(main_process, ev, data)
{

  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);

  while(1){

    nullnet_set_input_callback(input_callback);

    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    printf("hey");
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
