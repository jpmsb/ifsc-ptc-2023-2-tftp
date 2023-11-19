#include "DATA.h"

DATA::DATA()
  : opcode(htons(3)), block(0), count(0), bytesAmount(0)
{
    
}

DATA::DATA(tftp2::Mensagem & pbMessage)
  : opcode(htons(3)), block(htons(1)), count(1), bytesAmount(0), isFile(false) {

   serializedMessage = pbMessage.SerializeAsString();
   std::string dataString = serializedMessage.substr(0, 512);
   memcpy(data, dataString.c_str(), 512);
   bytesAmount = dataString.size();
}

DATA::DATA(const std::string & filename)
  : opcode(htons(3)), block(htons(1)), count(1), bytesAmount(0), isFile(true), file(filename, std::ios::binary)
{
    file.read((char *)data, 512);
    bytesAmount = file.gcount();
}

DATA::DATA(char bytes[], size_t size){
    uint8_t uint16Size = sizeof(uint16_t);

    memcpy(&opcode, bytes, uint16Size);
    memcpy(&block, bytes + uint16Size, uint16Size);
    memcpy(data, bytes+4, size-4);
    bytesAmount = size-4;
    count = ntohs(block);
}

void DATA::increment(){
    count = ntohs(block)+1;
    block = htons(count);
    memset(data, 0, 512);

    if(isFile){
	file.read((char *)data, 512);
	bytesAmount = file.gcount();
    } else {
        std::string dataString = serializedMessage.substr(512*(count-1), 512);
        memcpy(data, dataString.c_str(), 512);
        bytesAmount = dataString.size();
    }
}

uint16_t DATA::getBlock(){
    return count;
}

uint16_t DATA::getOpcode(){
    return ntohs(opcode);
}

const char* DATA::getData(){
    return data;
}

int DATA::size(){
    return 4+bytesAmount; 
}

int DATA::dataSize(){
    return bytesAmount;
}

void DATA::setData(char _data[], size_t size){
    memcpy(data, _data, size);
    bytesAmount = size;
}

void DATA::setBytes(char bytes[], size_t size){
    uint8_t uint16Size = sizeof(uint16_t);

    memcpy(&opcode, bytes, uint16Size);
    memcpy(&block, bytes + uint16Size, uint16Size);
    memcpy(data, bytes+4, size-4);
    bytesAmount = size-4;
    count = ntohs(block);
}
