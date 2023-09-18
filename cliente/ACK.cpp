#include "ACK.h"

ACK::ACK(){
    ACK::opcode = htons(4);
    ACK::block = 0;
}

void ACK::increment(){
    block = htons(ntohs(block)+1); 
}

uint16_t ACK::get_block(){
   return 0;
}

void ACK::set(){
}
