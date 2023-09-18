#include "DATA.h"

DATA::DATA(){
    DATA::opcode = htons(3);
    DATA::block = 0;
}

void DATA::increment(){
    block = htons(ntohs(block)+1); 
}

uint16_t DATA::get_block(){
   return 0;
}

void DATA::set(){
}
