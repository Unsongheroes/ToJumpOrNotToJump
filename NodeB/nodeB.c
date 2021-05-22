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
#define TIMEOUTLIMIT 160
//static linkaddr_t addr_nodeB =     {{0x77, 0xb7, 0x7b, 0x11, 0x0, 0x74, 0x12, 0x00 }};
static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
//static linkaddr_t addr_nodeA =     {{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};
static linkaddr_t addr_Sender;
//static clock_time_t timelimit = 0;
static bool checksum = false;
static bool isPinging = false;
// static bool isRelaying = false;
static JumpPackage payload;
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
bool errorOrNot() {
  int r = rand() % 10;
  if (r > 10) {
    return true;
  } else {
    return false;
  }
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

bool checkChecksum(JumpPackage payload){
  LOG_INFO("Checking checksum: %i\n",payload.checksum );
    int pchecksum = checksum(payload.payload, payload.length);
    if(pchecksum == payload.checksum){
      printf("Checksum correct\n");
      return true;
    }
    else{
      printf("Checksum did not match payload\n");
      return false;
    }
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



/* ----------------------------- Helper ----------------------------------- */
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest);
void sendAck(const linkaddr_t *src) {
  uint8_t acknowledge = 1;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  NETSTACK_NETWORK.output(src);
  LOG_INFO("Acknowledge sent!\n");
  nullnet_set_input_callback(input_callback);
}

void sendNack(const linkaddr_t *src) {
  uint8_t acknowledge = -1;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  NETSTACK_NETWORK.output(src);
  LOG_INFO("Not acknowledge sent!\n");
  nullnet_set_input_callback(input_callback);
}

/* ----------------------------- Callbacks ----------------------------------- */
void ack_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) {
  printf("ack_callback called\n");
  if (memcmp(src, &addr_Sender,8) == 0) {
    return;
  }
  uint8_t ack;
  memcpy(&ack, data, sizeof(ack));
  
  if (ack == 1) {
    
    LOG_INFO("Acknowledged received from: ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
    
    sendAck(&addr_Sender);
  } else if(ack == -1) {
    LOG_INFO("Not acknowledged received from: ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
    
    sendNack(&addr_Sender);
  }
}

void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{
  memcpy(&payload, data, sizeof(payload));

  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload);
  addr_Sender = *src;
  if(payload.payload == 0) {
    isPinging = true;
  } else if(payload.length > 0) {
    checksum = checkChecksum(payload);
  }

/*   if(acknowlegde) {
    printSender(payload);
    printReceiver(payload);
    printPayload(payload);
    
  }
  */

/*   if (payload.length > 0) { //received payload
      // if(errorOrNot()) {
      //  sendNack(&addr_Sender);
      // }
      printSender(payload);
      printReceiver(payload);
      printPayload(payload);
    if(!checkChecksum(payload)){
      sendNack(&addr_Sender);
    } else {
      NETSTACK_NETWORK.output(&addr_nodeC);

      nullnet_set_input_callback(ack_callback);
    }
  } else { // received ping
    sendAck(&addr_Sender);
  } */
}


void pinging(struct state * state) {
  printf("%s \n", __func__);
  nullnet_set_input_callback(input_callback);
  if(isPinging) {
    isPinging = false;
    sendAck(&addr_Sender);
  } 
  state->next = relaying;
  
}


void relaying(struct state * state) {
  printf("%s \n", __func__);
  // nullnet_set_input_callback(ack_callback);
  
  if(!checksum) {
    sendNack(&addr_Sender);
  } else {
    
    NETSTACK_NETWORK.output(&addr_nodeC);

    nullnet_set_input_callback(ack_callback);
    etimer_set(&periodic_timer, state->timeoutCycles);
    state->next = pinging;
  }
 /*  if(isPinging) {
    state->next = pinging;
    // sendAck(&addr_Sender);
  } */
}

void init(struct state * state)
{
    printf("%s \n", __func__);
    JumpPackage payload;
    nullnet_buf = (uint8_t *)&payload;
    nullnet_len = sizeof(payload);

    state->timeoutCycles = 20; 
    state->timeoutCounter = 0;
    state->nackCounter = 0;
    isPinging = false;
    etimer_set(&periodic_timer, state->timeoutCycles);
    
    state->next = pinging;
}

static struct state state = { init, 0, 0, 5 };
PROCESS_THREAD(nodeB, ev, data)
{
  

 /*  JumpPackage payload;
  nullnet_buf = (uint8_t *)&payload;
  nullnet_len = sizeof(payload); 
  nullnet_set_input_callback(input_callback); */
  
  PROCESS_BEGIN();
    printf("STARTING NODE B,,, \n");
    // etimer_set(&periodic_timer, TIMEOUTLIMIT);

  SENSORS_ACTIVATE(button_sensor);
    while (1)
    {
        //nullnet_set_input_callback(input_callback);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 	// Wait until time is expired
        etimer_reset(&periodic_timer);
        printf("Timeout reached\n");
        nullnet_set_input_callback(input_callback);

    }
 
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */

