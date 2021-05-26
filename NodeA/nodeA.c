/// Node A - Sender node
#include "JumpHeader.h"
#include "cc2420.h"

#include "dev/button-sensor.h"
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

#define RSSI_THRESHOLD -120 
#define NACK_COUNTER_LIMIT 3
#define TIMEOUT_COUNTER_LIMIT 3
#define INITIAL_TIMEOUT 30 
//static linkaddr_t addr_nodeB =     {{0xe3, 0xfd, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}}; //physical domain

//static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}}; //physical domain

static linkaddr_t addr_nodeB = {{0x02, 0x02, 0x02, 0x00, 0x02, 0x74, 0x12, 0x00}}; // cooja
static linkaddr_t addr_nodeC = {{0x03, 0x03, 0x03, 0x00, 0x03, 0x74, 0x12, 0x00}}; // cooja

static bool Acknowledged = false; // variable determining whether an ACK was received
static bool Notacknowledged = false; // variable determining whether a NACK was received
static int counter = 0; // Counter to determine number of packages that should be transmitted
static int ackCounter = 0;
static int nackCounter = 0;
static int Pinging = 0; // Pinging variable can have 3 states: 0 = no response received, 1 = RSSI value is above threshold, 2 = RSSI value below threshold.
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
    bool relaying; // bool indicating 
    int timeoutCycles; // integer indicating how many clock cycles before next state transition
    bool transmitting; // bool indicating if transmission or pinging is in progress
    //linkaddr_t receiver;
};
state_fn init, pinging, transmitting; //the different states for the mote
/*-----------------------------------------------------------*/

PROCESS(nodeA, "Node A - Sender");
AUTOSTART_PROCESSES(&nodeA);
/* ----------------------------- Helper ----------------------------------- */
static unsigned long to_10milseconds(uint64_t time)
{
  return (unsigned long)(time / 62.5 );
}


uint8_t checksum(uint8_t* buffer, uint8_t len) // function to calculate a naive checksum
{
      size_t i;
      uint8_t checksum = 0;
      for (i = 0; i < len; ++i) {
        checksum += buffer[i];
      }
            
      return checksum;
}

bool Radio_signal_strength( const linkaddr_t *src) // function to measure the RSSI value and determine whether or not it is above or below the threshold.
{
  static signed char rss;
  static signed char rss_val;
  static signed char rss_offset;
  //printf("Got message from: "); 
  //LOG_INFO_LLADDR(src);
  //printf("\n");
  rss_val = cc2420_last_rssi;
  rss_offset=-45;
  rss=rss_val + rss_offset;
  //printf("RSSI of Last Packet Received is %d\n",rss);
  if(rss > RSSI_THRESHOLD && rss < 0) {
    return true; // RSSI is below threshold
  } else {
    return false; // RSSI is above threshold
  }
  
}

void sendPayload(linkaddr_t dest, int sequenceNumber) { // function to send and construct a package.
  uint8_t payloadData[64] = {1*sequenceNumber,2*sequenceNumber,3*sequenceNumber,4*sequenceNumber,5*sequenceNumber,6*sequenceNumber,7*sequenceNumber};
  JumpPackage payload;
  payload.sender = (linkaddr_t){{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}}; //source adddress
  payload.destination = (linkaddr_t){{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}}; // sink addrses
  memcpy(payload.payload,payloadData,64);
  payload.sequenceNumber = sequenceNumber;
  payload.checksum = checksum(payloadData,7);
  payload.length = 7;
  // assign to nullnet buffer
  nullnet_buf = (uint8_t *)&payload; 
  nullnet_len = sizeof(payload);
  NETSTACK_NETWORK.output(&dest); //transmit the package
}


void transmitting_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) { // callback function for the transmitting state
  uint8_t ack;
  memcpy(&ack, data, sizeof(ack)); // copy data
  if (ack == 1) { //acknowledged received
    
      Acknowledged = true;
      //LOG_INFO("Acknowledged received from: ");
      ackCounter++;
      counter++;
      //LOG_INFO_LLADDR(src);
      //LOG_INFO_("\n");
    } else if (ack == 255) { // negative acknowledge received
      Notacknowledged = true;
      nackCounter++;
      counter++;
      //LOG_INFO("Not acknowledged received from: ");
      //LOG_INFO_LLADDR(src);
      //LOG_INFO_("\n");
    }
}
void pinging_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) // callback function for the pinging state
{
  uint8_t ack;
  memcpy(&ack, data, sizeof(ack));

  if (Radio_signal_strength(src)) {
    Pinging = 2;
  } else {
    Pinging = 1;
  }
}

void transmitting(struct state * state) { // Transmitting state
  //printf("%s \n", __func__);
  nullnet_set_input_callback(transmitting_callback); // set callback function
  if(Acknowledged) { // if an acknowledge have been received
    //printf("Received acknowledge\n");
    if(state->sequenceNumber == 3) { // if all packages have been sent
      //printf("Sequence ended wait 10 sec.'\n");
      state->timeoutCycles = 250; // set timer to two seconds
      state->next = init; // set next state to init
    } else { // if not all packages have been sent
      Acknowledged = false; 
      state->sequenceNumber++; //increment sequence number
      if (state->relaying) // if relaying transmit to node B
        sendPayload(addr_nodeB,state->sequenceNumber);
      else // if not relaying transmit to node C
        sendPayload(addr_nodeC,state->sequenceNumber);
      etimer_set(&periodic_timer, state->timeoutCycles); // set timer
      state->next = transmitting; // set next state to transmitting
    }
    //printf("Starting timer\n");
    etimer_set(&periodic_timer, state->timeoutCycles);
  } else if (state->sequenceNumber == 1 && !state->transmitting) { // if this is the first transmission of a package
      state->transmitting = true;
      if (state->relaying) // if relaying transmit to node B
        sendPayload(addr_nodeB,state->sequenceNumber);
      else // if not relaying transmit to node C
        sendPayload(addr_nodeC,state->sequenceNumber);
      etimer_set(&periodic_timer, state->timeoutCycles); 
      state->next = transmitting; // set next state to transmitting
  }  else if (state->timeoutCounter < TIMEOUT_COUNTER_LIMIT && !Notacknowledged) { // if timeout counter is below the limit and no negative acknowledge received
      state->timeoutCycles = state->timeoutCycles * 2; // double the timeout period
      state->timeoutCounter++; 
      if (state->relaying)  // if relaying transmit to node B
        sendPayload(addr_nodeB,state->sequenceNumber);
      else // if not relaying transmit to node C
        sendPayload(addr_nodeC,state->sequenceNumber);
      etimer_set(&periodic_timer, state->timeoutCycles); 
      state->next = transmitting; // set next state to transmitting
      //LOG_INFO("Timeout cycles: %i timeout counter: %i clock_time: %lu \n",state->timeoutCycles,state->timeoutCounter,clock_time());
  } else if (state->nackCounter < NACK_COUNTER_LIMIT && Notacknowledged) {  // if negative acknowledge counter is below the limit and a negative acknowledge received
      state->nackCounter++;   
      //LOG_INFO("nack counter: %i clock_time: %lu \n",state->nackCounter,clock_time());
      
      Notacknowledged = false;
      if (state->relaying) // if relaying transmit to node B
        sendPayload(addr_nodeB,state->sequenceNumber);
      else // if not relaying transmit to node C
        sendPayload(addr_nodeC,state->sequenceNumber);

      state->next = transmitting;
      etimer_set(&periodic_timer, state->timeoutCycles);
      
  } else { // if timeout counter or nack counter has reached the limit then set the next state to pinging and reset variables
    //LOG_INFO("Timeout or nack limit reached go ping. Relaying: %d\n",state->relaying);
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

void pinging(struct state * state) { //Pinging state
 //printf("%s \n", __func__);

 if (Pinging == 2) { //if acknowledge received and RSSI below threshold then set next state to transmitting
    //printf("Setting next state to transmtting \n");
    state->next = transmitting;
    state->transmitting = false;
    state->timeoutCounter = 0;
    state->timeoutCycles = INITIAL_TIMEOUT;
    etimer_set(&periodic_timer, state->timeoutCycles);
 } else if (!state->transmitting && Pinging == 0) { // First time pinging a node
    state->transmitting = true;
    uint8_t payloadData = 0; 
    nullnet_buf = (uint8_t *)&payloadData;

    nullnet_len = sizeof(payloadData);
    nullnet_set_input_callback(pinging_callback);
    if (state->relaying) { // if relaying ping node B
      //printf("data: %u \n", *nullnet_buf);
      NETSTACK_NETWORK.output(&addr_nodeB);
    }
    else // if not relaying ping node C
      NETSTACK_NETWORK.output(&addr_nodeC);
    etimer_set(&periodic_timer, state->timeoutCycles);
 } else if (state->timeoutCounter < TIMEOUT_COUNTER_LIMIT && Pinging == 0) { // if timeout counter is below the limit and no response received
    state->timeoutCycles = state->timeoutCycles * 2;
    state->timeoutCounter++;
    uint8_t payloadData = 0; 
    nullnet_buf = (uint8_t *)&payloadData;

    nullnet_len = sizeof(payloadData);
    nullnet_set_input_callback(pinging_callback);
    if (state->relaying) { // if relaying ping node B
      //printf("data: %u \n", *nullnet_buf);
      NETSTACK_NETWORK.output(&addr_nodeB);
    }
    else // if not relaying ping node C
      NETSTACK_NETWORK.output(&addr_nodeC);
    
    etimer_set(&periodic_timer, state->timeoutCycles);
    state->next = pinging;
    //LOG_INFO("Timeout cycles: %i timeout counter: %i clock_time: %lu relaying: %d \n",state->timeoutCycles,state->timeoutCounter,clock_time(),state->relaying);
  } else { // if timeout counter has reached the limit
    state->timeoutCounter = 0;
    if(state->relaying == true) { // if pinging node B reset the state machine and transition to init state and set timer to 2 seconds
      state->timeoutCycles = 250;
      state->next = init;
      nackCounter++; //increment counter for nack since the package has been deemed lost
      counter++;
    } else { // If pinging node C set relaying to true so the node A pings node B instead
      state->timeoutCycles = INITIAL_TIMEOUT;
      state->next = pinging;
      Pinging = 0;
      state->relaying = true;
    }
    
    etimer_set(&periodic_timer, state->timeoutCycles);
    //LOG_INFO("Timeout counter limit reached...\n");
  }
}
void init(struct state * state) // Init state set initial variables to initial values
{
    //printf("%s \n", __func__);
    
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

static struct state state = { init, 0, 0, false, INITIAL_TIMEOUT, false }; //initialize state machine
PROCESS_THREAD(nodeA, ev, data)
{
    
  PROCESS_BEGIN();
    printf("STARTING NODE A,,, \n"); 
    state.next(&state);
    while (counter < 100)
    {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 	// Wait until time is expired
        if (state.next != NULL) // handle next state if not null
            state.next(&state);

    }
    //energest measurements and print statements
    energest_flush();

    printf("\nEnergest:\n");
    printf(" CPU          %4lus LPM      %4lus DEEP LPM %4lus  Total time %lus\n",
        to_10milseconds(energest_type_time(ENERGEST_TYPE_CPU)),
        to_10milseconds(energest_type_time(ENERGEST_TYPE_LPM)),
        to_10milseconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
        to_10milseconds(ENERGEST_GET_TOTAL_TIME()));
    printf(" Radio LISTEN %4lus TRANSMIT %4lus OFF      %4lus\n",
        to_10milseconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
        to_10milseconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
        to_10milseconds(ENERGEST_GET_TOTAL_TIME()
                  - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                  - energest_type_time(ENERGEST_TYPE_LISTEN)));
  LOG_INFO("Nack received: %i  Ack received: %i\n", nackCounter, ackCounter);
  LOG_INFO("Nack received: %i  Ack received: %i\n", nackCounter, ackCounter);
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */
