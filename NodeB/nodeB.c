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
#define TIMEOUTLIMIT 60
//static linkaddr_t addr_nodeB =     {{0x77, 0xb7, 0x7b, 0x11, 0x0, 0x74, 0x12, 0x00 }};
//static linkaddr_t addr_nodeC =     {{0x43, 0xf5, 0x6e, 0x14, 0x00, 0x74, 0x12, 0x00}};
static linkaddr_t addr_nodeC = {{0x03, 0x03, 0x03, 0x00, 0x03, 0x74, 0x12, 0x00}}; // cooja
//static linkaddr_t addr_nodeA =     {{0x77, 0xb7, 0x7b, 0x11, 0x00, 0x74, 0x12, 0x00}};
static linkaddr_t addr_Sender;
//static clock_time_t timelimit = 0;
static bool isPinging = false;
static bool messageRelayed = false;
static bool isRelaying = false;
// static bool isRelaying = false; (kristoffers)
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
  linkaddr_t tmp = *src;
  NETSTACK_NETWORK.output(&tmp);
  LOG_INFO("Acknowledge sent!\n");
  LOG_INFO_LLADDR(src);
  LOG_INFO("\n " );
}

void sendNack(const linkaddr_t *src) {
  uint8_t acknowledge = 255;
  nullnet_buf = (uint8_t *)&acknowledge;
  nullnet_len = sizeof(acknowledge);
  linkaddr_t tmp = *src;
  NETSTACK_NETWORK.output(&tmp);
  LOG_INFO("Not acknowledge sent!\n");
  LOG_INFO_LLADDR(src);
  LOG_INFO("\n " );
}

/* ----------------------------- Callbacks ----------------------------------- */
/* void ack_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) {
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
  } else if(ack == 255) {
    LOG_INFO("Not acknowledged received from: ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
    
    sendNack(&addr_Sender);
  }
} */

void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{

  uint8_t received = 99;
    
  memcpy(&received, data, sizeof(received));
  printf("Received: %d\n",received); 
  if (received == 0) {
    isPinging = true;
    isRelaying = false;
    addr_Sender = *src;
    sendAck(&addr_Sender);
     

    printf("Received ping from Node A\n");
  } else if( received == 255) {
      LOG_INFO("Not acknowledged received from: ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
    isPinging = false;
    isRelaying = false;
    sendNack(&addr_Sender);
  } else if (received == 1 ) {
    LOG_INFO("Acknowledged received from: ");
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
    isPinging = false;
    isRelaying = false;
    sendAck(&addr_Sender);
  } else {
     addr_Sender = *src;

    JumpPackage payloadTemp;
    memcpy(&payloadTemp, data, sizeof(payloadTemp));
    printf("payload length: %u\n", payloadTemp.length);
    printSender(payloadTemp);
    printReceiver(payloadTemp);
    printPayload(payloadTemp);
    nullnet_buf = (uint8_t *)&payloadTemp;
    nullnet_len = sizeof(payloadTemp);
    
    if (checkChecksum(payloadTemp)) {
      NETSTACK_NETWORK.output(&addr_nodeC);
    } else {
      sendNack(&addr_Sender);
    }
    LOG_INFO("Message relayed\n");
    messageRelayed = true;
    isRelaying = true;
    isPinging = false;
    printf("Received payload from Node A\n");
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
  //nullnet_set_input_callback(input_callback);
  if(isRelaying ) {
    state->timeoutCycles = 20;
    state->next = relaying;
  } else if(isPinging) {
    
    state->next = relaying;
    state->timeoutCycles = 20;
  } 
  
  
  etimer_set(&periodic_timer, state->timeoutCycles);
}


void relaying(struct state * state) {
  printf("%s \n", __func__);
  printf("messagesrelayed: %i, ispinging: %i \n", messageRelayed, isPinging);
  // nullnet_set_input_callback(ack_callback);
  //nullnet_set_input_callback(input_callback);
  /* if (isPinging) {
    isPinging = false;
    state->next = relaying;
    state->timeoutCycles = 20;
    etimer_set(&periodic_timer, state->timeoutCycles);
  } else */ 
  if (isRelaying) {
    if(!messageRelayed) {
      printf("not pinging and implies not messagesrelayed \n");
      sendNack(&addr_Sender);
      isRelaying = false;
      state->timeoutCycles = 20;
      etimer_set(&periodic_timer, state->timeoutCycles);
      state->next = pinging;
    } else {
      state->timeoutCycles = TIMEOUTLIMIT;
      messageRelayed = false;
    
      etimer_set(&periodic_timer, state->timeoutCycles);
      state->next = relaying;
    }
    
  } else {
      state->timeoutCycles = 20;
      etimer_set(&periodic_timer, state->timeoutCycles);
      state->next = pinging;
  }

 /*  if(isPinging) {
    state->next = pinging;
    // sendAck(&addr_Sender);
  } */
  etimer_set(&periodic_timer, state->timeoutCycles);
}

void init(struct state * state)
{
    printf("%s \n", __func__);

    nullnet_set_input_callback(input_callback);
    
    state->timeoutCycles = 20; 
    state->timeoutCounter = 0;
    state->nackCounter = 0;
    isPinging = false;
    messageRelayed = false;
    isRelaying = false;
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
  state.next(&state);
  SENSORS_ACTIVATE(button_sensor);
    while (1)
    {
        //nullnet_set_input_callback(input_callback);

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 	// Wait until time is expired
        if (state.next != NULL) // handle next state if not null
            state.next(&state);
    }
 
  PROCESS_END();
}

/* -----------------------------         ----------------------------------- */

