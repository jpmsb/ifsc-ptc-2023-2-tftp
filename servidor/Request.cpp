#include "Request.h"

using namespace std;

Request::Request(char bytes[], size_t size) 
    : opcodeSize(2) {
    uint8_t uint16Size = sizeof(uint16_t);

    memcpy(&networkOrderOpcode, bytes, uint16Size);
    opcode = ntohs(networkOrderOpcode);

    filenameSize = strnlen(bytes + uint16Size, size);
    filename.assign(bytes + uint16Size, filenameSize);
    
    modeSize = strnlen(bytes + uint16Size + filenameSize + 1, size);
    mode.assign(bytes + uint16Size + filenameSize + 1, modeSize);
}

uint16_t Request::getOpcode(){
    return opcode;
}

string Request::getFilename(){
    return filename;
}

string Request::getMode(){
    return mode;
}

int Request::size(){
    return opcodeSize + filenameSize + 1 + modeSize + 1;
}

char* Request::data(){
     char* data = new char[size()];

     memcpy(data, &networkOrderOpcode, sizeof(networkOrderOpcode));
     memcpy(data + opcodeSize, filename.data(), filenameSize);
     memcpy(data + opcodeSize + filenameSize + 1, mode.data(), modeSize + 1);

     return data;
}
