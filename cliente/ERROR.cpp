#include "ERROR.h"

ERROR::ERROR(){
    ERROR::opcode = htons(4);
}

ERROR::ERROR(char bytes[], size_t size){
   setBytes(bytes, size);
}

uint16_t ERROR::getOpcode(){
   return ntohs(opcode);
}

uint16_t ERROR::getErrorCode(){
   return ntohs(errorCode);
}

string ERROR::getErrorMessage(){
   return errorMessage;
}

void ERROR::setBytes(char bytes[], size_t size){
   uint8_t uint16Size = sizeof(uint16_t);

   memcpy(&opcode, bytes, uint16Size);
   memcpy(&errorCode, bytes + uint16Size, uint16Size);
   
   char * restOfBytes = bytes+4;
   errorMessage = restOfBytes;
}
