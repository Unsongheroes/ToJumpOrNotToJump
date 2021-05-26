#ifndef JumpHeader_H 
#define JumpHeader_H

#include "contiki.h"
#include "net/nullnet/nullnet.h"

typedef struct JumpPackage {
   linkaddr_t sender;
   linkaddr_t destination;
   uint8_t payload[64]; 
   uint8_t sequenceNumber; 
   uint8_t checksum; 
   uint8_t length;
} JumpPackage;

#endif 