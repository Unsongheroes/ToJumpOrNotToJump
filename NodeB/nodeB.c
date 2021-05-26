/// Node B receiver node
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
#define TIMEOUTLIMIT 60
//static linkaddr_t addr_nodeC =     {{0x77, 0xb7, 0x7b, 0x11, 0x0, 0x74, 0x12, 0x00 }};
static linkaddr_t addr_nodeC = {{0x03, 0x03, 0x03, 0x00, 0x03, 0x74, 0x12, 0x00}}; // cooja
static linkaddr_t addr_Sender;
static bool isPinging = false; // variable indicating if node B is pinged
static bool messageRelayed = false; // variable indicating if a message have been relayed
static bool isRelaying = false; // variable indicating if node B is relaying
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
    int timeoutCycles;
    //linkaddr_t receiver;
};
state_fn init,pinging, relaying; //the different states for the mote
/* -----------------------------         ----------------------------------- */



PROCESS(nodeB, "Node B - Sender");
AUTOSTART_PROCESSES(&nodeB);
/* ----------------------------- Helper ----------------------------------- */
static unsigned long to_milliseconds(uint64_t time) // helper function to calcule energest values to milliseconds
{
  return (unsigned long)(time / 62.5 );
}
bool errorOrNot() { // introduction of a error probability of 50 %
  int r = rand() % 10;
  if (r < 5) {
    return true;
  } else {
    return false;
  }
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

bool checkChecksum(JumpPackage payload){ // function to determine if the received checksum match the calculated checksum
  //LOG_INFO("Checking checksum: %i\n",payload.checksum );
    uint8_t pchecksum = checksum(payload.payload, payload.length);
    if(pchecksum == payload.checksum){
      //printf("Checksum correct\n");
      return true;
    }
    else{
      //printf("Checksum did not match payload\n");
      return false;
    }
}

void printSender(JumpPackage payload ) { // helper function to print the sender from the package
  LOG_INFO("Sender: " );
  linkaddr_t* sender = &payload.sender;
  LOG_INFO_LLADDR(sender);
  LOG_INFO("\n " );
  
}

void printReceiver(JumpPackage payload ) { // helper function to print the destination from the package
  LOG_INFO("Destination: " );
  linkaddr_t* destination = &payload.destination;
  LOG_INFO_LLADDR(destination);
  LOG_INFO("\n " );
  
}

void printPayload(JumpPackage payload) { // helper function to print the payload from the package
  size_t i;
  for ( i = 0; i < 64; i++)
  {
    if (payload.payload[i] ==0) {
      break;
    }
      LOG_INFO("Received message %u \n", payload.payload[i] );
  }
}



/* ----------------------------- Helper ----------------------------------- */
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest); // declaration of callback function implementation is below.
void sendAck(const linkaddr_t *src) { // function to send an acknowledge
  uint8_t acknowledge = 1;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  linkaddr_t tmp = *src;
  NETSTACK_NETWORK.output(&tmp);
  //LOG_INFO("Acknowledge sent!\n");
  //LOG_INFO_LLADDR(src);
  //LOG_INFO("\n " );
}

void sendNack(const linkaddr_t *src) { // function to send a negative acknowledge
  uint8_t acknowledge = 255;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  linkaddr_t tmp = *src;
  NETSTACK_NETWORK.output(&tmp);
  //LOG_INFO("Not acknowledge sent!\n");
  //LOG_INFO_LLADDR(src);
  //LOG_INFO("\n " );
}



void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) // callback function implementation
{

  uint8_t received = 99; //initialize a random value before assigning the received message to it
    
  memcpy(&received, data, sizeof(received));
  //printf("Received: %d\n",received); 
  if (received == 0) { // if ping received
    isPinging = true;
    isRelaying = false;
    addr_Sender = *src;
    sendAck(&addr_Sender);
     

    //printf("Received ping from Node A\n");
  } else if( received == 255) { // if negative acknowledge received
    //  LOG_INFO("Not acknowledged received from: ");
    //LOG_INFO_LLADDR(src);
    //LOG_INFO_("\n");
    isPinging = false;
    isRelaying = false;
    sendNack(&addr_Sender);
  } else if (received == 1 ) { // if acknowledge received
    //LOG_INFO("Acknowledged received from: ");
    //LOG_INFO_LLADDR(src);
    //LOG_INFO_("\n");
    isPinging = false;
    isRelaying = false;
    sendAck(&addr_Sender);
  } else { // if package received
     addr_Sender = *src;

    JumpPackage payloadTemp;
    memcpy(&payloadTemp, data, sizeof(payloadTemp));
    //printf("payload length: %u\n", payloadTemp.length);
    //printSender(payloadTemp);
    //printReceiver(payloadTemp);
    //printPayload(payloadTemp);
    nullnet_buf = (uint8_t *)&payloadTemp;
    nullnet_len = sizeof(payloadTemp);
    
    if (checkChecksum(payloadTemp)) { //if checksum matches
      NETSTACK_NETWORK.output(&addr_nodeC); //relay message
    } else { // if checksum not matches send nack
      sendNack(&addr_Sender);
    }
    //LOG_INFO("Message relayed\n");
    messageRelayed = true;
    isRelaying = true;
    isPinging = false;
    //printf("Received payload from Node A\n");
  }

}


void pinging(struct state * state) { // Pinging state
  //printf("%s \n", __func__);
  if(isRelaying) { // if relaying transition to relaying state
    state->timeoutCycles = 125;
    state->next = relaying;
  } else if(isPinging) { // if pinging transition to relaying state
    state->next = relaying;
    state->timeoutCycles = 125;
  } /* else {
    state->next = pinging;
    state->timeoutCycles = 125;
  } */

  etimer_set(&periodic_timer, state->timeoutCycles);
}


void relaying(struct state * state) { // relaying state

  if (isRelaying) { // if relaying
    if(!messageRelayed) { // if a message has not been relayed
      //printf("not pinging implies not messagesrelayed \n");
      sendNack(&addr_Sender);
      isRelaying = false;
      state->timeoutCycles = 125;
      etimer_set(&periodic_timer, state->timeoutCycles);
      state->next = pinging;
    } else { // if a message has been relayed set longer timeout and wait for ack or nack
      state->timeoutCycles = TIMEOUTLIMIT;
      messageRelayed = false;
    
      etimer_set(&periodic_timer, state->timeoutCycles);
      state->next = relaying;
    }
    
  } else {
    //measure and print energest times
    energest_flush();

    printf("\nEnergest:\n");

    printf("10ms *: CPU          %4lums LPM      %4lus DEEP LPM %4lums  Total time %lums\n",
        to_milliseconds(energest_type_time(ENERGEST_TYPE_CPU)),
        to_milliseconds(energest_type_time(ENERGEST_TYPE_LPM)),
        to_milliseconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
        to_milliseconds(ENERGEST_GET_TOTAL_TIME()));
    printf("10ms *: Radio LISTEN %4lums TRANSMIT %4lums OFF      %4lums\n",
        to_milliseconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
        to_milliseconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
        to_milliseconds(ENERGEST_GET_TOTAL_TIME()
                  - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                  - energest_type_time(ENERGEST_TYPE_LISTEN)));
      state->timeoutCycles = 125;
      etimer_set(&periodic_timer, state->timeoutCycles);
      state->next = pinging;
  }

 /*  if(isPinging) {
    state->next = pinging;
    // sendAck(&addr_Sender);
  } */
  etimer_set(&periodic_timer, state->timeoutCycles);
}

void init(struct state * state) // init state intialize variables and callback functions
{
    //printf("%s \n", __func__);

    nullnet_set_input_callback(input_callback);
    
    state->timeoutCycles = 125; 
    state->timeoutCounter = 0;
    state->nackCounter = 0;
    isPinging = false;
    messageRelayed = false;
    isRelaying = false;
    etimer_set(&periodic_timer, state->timeoutCycles);
    
    state->next = pinging;
}

static struct state state = { init, 0, 0, 5 }; // set initial state 
PROCESS_THREAD(nodeB, ev, data)
{
  

 /*  JumpPackage payload;
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload); 
  nullnet_set_input_callback(input_callback); */
  
  PROCESS_BEGIN();
    printf("STARTING NODE B,,, \n");
    // etimer_set(&periodic_timer, TIMEOUTLIMIT);
  state.next(&state);
  SENSORS_ACTIVATE(button_sensor);
    while (1)
    {
        //nullnet_set_input_callback(input_callback);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 	// Wait until time is expired
        if (state.next != NULL) // handle next state if not null 
            state.next(&state);
        
        etimer_reset(&periodic_timer); // reset timer
    }
 
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */

