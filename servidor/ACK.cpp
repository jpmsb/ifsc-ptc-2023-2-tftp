#include "ACK.h"

ACK::ACK(){
    ACK::opcode = htons(4);
    ACK::block = 0;
}

void ACK::increment(){
    block = htons(ntohs(block)+1); 
}

uint16_t ACK::getBlock(){
   return ntohs(block);
}

uint16_t ACK::getOpcode(){
   return ntohs(opcode);
}

void ACK::setBytes(char bytes[]){
   uint8_t uint16Size = sizeof(uint16_t);

   memcpy(&opcode, bytes, uint16Size);
   memcpy(&block, bytes + uint16Size, uint16Size);
}
