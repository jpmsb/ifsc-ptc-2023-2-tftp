#include "ERROR.h"

ERROR::ERROR()
    : opcode(htons(5)), errorCode(htons(0)){
}

ERROR::ERROR(uint8_t errorCode)
    : opcode(htons(5)), errorCode(htons(errorCode)) {
    errorMessage = tftpErrorMessages[errorCode];
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

int ERROR::size(){
   return 4 + errorMessage.size() + 1;
}

char* ERROR::data(){
   uint8_t uint16Size = sizeof(uint16_t);
   char * bytes = new char[size()];

   memcpy(bytes, &opcode, uint16Size);
   memcpy(bytes + uint16Size, &errorCode, uint16Size);
   memcpy(bytes + uint16Size + uint16Size, errorMessage.c_str(), errorMessage.size() + 1);

   return bytes;
}

void ERROR::setBytes(char bytes[], size_t size){
   uint8_t uint16Size = sizeof(uint16_t);

   memcpy(&opcode, bytes, uint16Size);
   memcpy(&errorCode, bytes + uint16Size, uint16Size);
   
   char * restOfBytes = bytes+4;
   errorMessage = restOfBytes;
}
