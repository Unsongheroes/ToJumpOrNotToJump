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
#include "sys/energest.h"


/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* ----------------------------- Defines ----------------------------------- */
#define SEND_INTERVAL (4 * CLOCK_SECOND)
#define RSSI_THRESHOLD -110
#define NACK_COUNTER_LIMIT 3
#define TIMEOUT_COUNTER_LIMIT 3
#define INITIAL_TIMEOUT 30
//static linkaddr_t addr_nodeB =     {{0xe3, 0xfd, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};

//static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};

// static linkaddr_t addr_wrong =     {{0x44, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t addr_broadCast = {{0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}};

//static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};

//static linkaddr_t addr_nodeA =     {{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};
static linkaddr_t addr_nodeB = {{0x02, 0x02, 0x02, 0x00, 0x02, 0x74, 0x12, 0x00}}; // cooja
static linkaddr_t addr_nodeC = {{0x03, 0x03, 0x03, 0x00, 0x03, 0x74, 0x12, 0x00}}; // cooja
//static int timeoutCycles = 10;  // 20 clockcycles = 20/125 =0.16
//static int nackCounter = 0;
static bool Acknowledged = false;
static bool Notacknowledged = false;
static int Pinging = 0;
static struct etimer periodic_timer;
/* -----------------------------         ----------------------------------- */
// State machine setup
struct state;
typedef void state_fn(struct state *);
struct state
{
    state_fn * next; // next pointer to the next state
    int timeoutCounter;
    int nackCounter;
    int sequenceNumber;
    bool relaying;
    int timeoutCycles;
    bool transmitting;
    //linkaddr_t receiver;
};
state_fn init, pinging, transmitting; //the different states for the mote
/*-----------------------------------------------------------*/



PROCESS(nodeA, "Node A - Sender");
AUTOSTART_PROCESSES(&nodeA);
/* ----------------------------- Helper ----------------------------------- */
static unsigned long to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}


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

void sendPayload(linkaddr_t dest, int sequenceNumber) {
  uint8_t payloadData[64] = {1*sequenceNumber,2*sequenceNumber,3*sequenceNumber,4*sequenceNumber,5*sequenceNumber,6*sequenceNumber,7*sequenceNumber};
  JumpPackage payload;
  payload.sender = (linkaddr_t){{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};
  payload.destination = (linkaddr_t){{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
  memcpy(payload.payload,payloadData,64);
  payload.sequenceNumber = sequenceNumber;
  payload.checksum = checksum(payloadData,7);
  payload.length = 7;
  //{{{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}},{{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}},payloadData,sequenceNumber,checksum(payloadData,7),7}
  nullnet_buf = (uint8_t *)&payload;

  nullnet_len = sizeof(payload);
  

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


void transmitting_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) {
  uint8_t ack;
  memcpy(&ack, data, sizeof(ack));
  if (ack == 1) {
    
      Acknowledged = true;
      LOG_INFO("Acknowledged received from: ");
      LOG_INFO_LLADDR(src);
      LOG_INFO_("\n");
    } else if (ack == 255) {
      Notacknowledged = true;
      LOG_INFO("Not acknowledged received from: ");
      LOG_INFO_LLADDR(src);
      LOG_INFO_("\n");
    }
}
void pinging_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{
  uint8_t ack;
  memcpy(&ack, data, sizeof(ack));

  if (Radio_signal_strength(src)) {
    Pinging = 2;
  } else {
    Pinging = 1;
  }
}

void transmitting(struct state * state) {
  printf("%s \n", __func__);
  nullnet_set_input_callback(transmitting_callback);
  if(Acknowledged) {
    if(state->sequenceNumber == 3) {
      state->timeoutCycles = 1250;
      state->next = init;
    } else {
      Acknowledged = false;
      state->sequenceNumber++;
      if (state->relaying) 
        sendPayload(addr_nodeB,state->sequenceNumber);
      else 
        sendPayload(addr_nodeC,state->sequenceNumber);
      etimer_set(&periodic_timer, state->timeoutCycles);
      state->next = transmitting;
    }
    etimer_set(&periodic_timer, state->timeoutCycles);
  } else if (state->sequenceNumber == 1 && !state->transmitting) {
      state->transmitting = true;
      if (state->relaying) 
        sendPayload(addr_nodeB,state->sequenceNumber);
      else 
        sendPayload(addr_nodeC,state->sequenceNumber);
      etimer_set(&periodic_timer, state->timeoutCycles);
      state->next = transmitting;
  }  else if (state->timeoutCounter < TIMEOUT_COUNTER_LIMIT && !Notacknowledged) {
      state->timeoutCycles = state->timeoutCycles * 2;
      state->timeoutCounter++;
      if (state->relaying) 
        sendPayload(addr_nodeB,state->sequenceNumber);
      else 
        sendPayload(addr_nodeC,state->sequenceNumber);
      etimer_set(&periodic_timer, state->timeoutCycles);
      state->next = transmitting;
      LOG_INFO("Timeout cycles: %i timeout counter: %i clock_time: %lu \n",state->timeoutCycles,state->timeoutCounter,clock_time());
  } else if (state->nackCounter < NACK_COUNTER_LIMIT && Notacknowledged) {
      state->nackCounter++;
      LOG_INFO("nack counter: %i clock_time: %lu \n",state->nackCounter,clock_time());
      
      Notacknowledged = false;
      if (state->relaying) 
        sendPayload(addr_nodeB,state->sequenceNumber);
      else 
        sendPayload(addr_nodeC,state->sequenceNumber);

      state->next = transmitting;
      etimer_set(&periodic_timer, state->timeoutCycles);
      
  } else {
    LOG_INFO("Timeout or nack limit reached go ping. Relaying: %d\n",state->relaying);
    Acknowledged = false;
    Notacknowledged = false;
    state->relaying = !state->relaying;
    state->timeoutCounter = 0;
    state->nackCounter = 0;
    state->transmitting = false;
    state->timeoutCycles = INITIAL_TIMEOUT;
    state->next = pinging;
    Pinging = 0;
    etimer_set(&periodic_timer, state->timeoutCycles);
  }
}

void pinging(struct state * state) {
 printf("%s \n", __func__);

 if (Pinging == 2) {
    printf("Setting next state to transmtting \n");
    state->next = transmitting;
    state->transmitting = false;
    state->timeoutCounter = 0;
    state->timeoutCycles = INITIAL_TIMEOUT;
    etimer_set(&periodic_timer, state->timeoutCycles);
 } else if (!state->transmitting && Pinging == 0) {
    state->transmitting = true;
    uint8_t payloadData = 0; 
    nullnet_buf = (uint8_t *)&payloadData;

    nullnet_len = sizeof(payloadData);
    nullnet_set_input_callback(pinging_callback);
    if (state->relaying) {
      printf("data: %u \n", *nullnet_buf);
      NETSTACK_NETWORK.output(&addr_nodeB);
    }
    else 
      NETSTACK_NETWORK.output(&addr_nodeC);
    etimer_set(&periodic_timer, state->timeoutCycles);
 } else if (state->timeoutCounter < TIMEOUT_COUNTER_LIMIT && Pinging == 0) {
    state->timeoutCycles = state->timeoutCycles * 2;
    state->timeoutCounter++;
    uint8_t payloadData = 0; 
    nullnet_buf = (uint8_t *)&payloadData;

    nullnet_len = sizeof(payloadData);
    nullnet_set_input_callback(pinging_callback);
    if (state->relaying) {
      printf("data: %u \n", *nullnet_buf);
      NETSTACK_NETWORK.output(&addr_nodeB);
    }
    else 
      NETSTACK_NETWORK.output(&addr_nodeC);
    
    etimer_set(&periodic_timer, state->timeoutCycles);
    state->next = pinging;
    LOG_INFO("Timeout cycles: %i timeout counter: %i clock_time: %lu relaying: %d \n",state->timeoutCycles,state->timeoutCounter,clock_time(),state->relaying);
  } else {
    state->timeoutCounter = 0;
    if(state->relaying == true) {
      state->timeoutCycles = 1250;
      state->next = init;
    } else {
      state->timeoutCycles = INITIAL_TIMEOUT;
      state->next = pinging;
      Pinging = 0;
      state->relaying = true;
    }
    
    etimer_set(&periodic_timer, state->timeoutCycles);
    LOG_INFO("Timeout counter limit reached...\n");
  }
}
void init(struct state * state)
{
    printf("%s \n", __func__);
    
    state->timeoutCycles = INITIAL_TIMEOUT; 
    state->timeoutCounter = 0;
    state->nackCounter = 0;
    state->sequenceNumber = 1;
    state->relaying = false;
    state->transmitting = false;
    Pinging = 0;
    Acknowledged = false;
    Notacknowledged = false;
    etimer_set(&periodic_timer, state->timeoutCycles);
    state->next = pinging;
}

static struct state state = { init, 0, 0, false, INITIAL_TIMEOUT, false };
PROCESS_THREAD(nodeA, ev, data)
{
    
  PROCESS_BEGIN();
    printf("STARTING NODE A,,, \n"); 
    state.next(&state);
    SENSORS_ACTIVATE(button_sensor);
    while (1)
    {
//        nullnet_set_input_callback(input_callback);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 	// Wait until time is expired
        if (state.next != NULL) // handle next state if not null
            state.next(&state);

         /* Update all energest times. */
        energest_flush();

        printf("\nEnergest:\n");
        printf(" CPU          %4lus LPM      %4lus DEEP LPM %4lus  Total time %lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
        printf(" Radio LISTEN %4lus TRANSMIT %4lus OFF      %4lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)));
        //etimer_reset(&periodic_timer);
       /*  if (timeoutCounter < TIMEOUT_COUNTER_LIMIT) {
          timeoutCycles = timeoutCycles * 2;
          timeoutCounter++;
          etimer_set(&periodic_timer, timeoutCycles);
          LOG_INFO("Timeout cycles: %i timeout counter: %i clock_time: %lu \n",timeoutCycles,timeoutCounter,clock_time());
        } else {
          LOG_INFO("Timeout counter limit reached...\n");
          PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
        }
        


        if (Pinging) {
          uint8_t payloadData = 1;
          nullnet_buf = (uint8_t *)&payloadData;

          nullnet_len = sizeof(payloadData);
          nullnet_set_input_callback(input_callback);


          NETSTACK_NETWORK.output(&addr_nodeC);

        } else if(!Acknowledged ) {
            sendPayload(addr_nodeC);
        }  else {

          LOG_INFO("Message delivered!\n");
          PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
          Acknowledged = 0;
        } */
    }
 
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */
