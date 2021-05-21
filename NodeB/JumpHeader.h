#ifndef JumpHeader_H 
#define JumpHeader_H

#include "contiki.h"
#include "net/nullnet/nullnet.h"

typedef struct JumpPackage {
   linkaddr_t sender;
   linkaddr_t destination;
   uint8_t payload[64]; 
   uint8_t sequenceNumber;
   int checksum; 
   int length;
} JumpPackage;

#endif 