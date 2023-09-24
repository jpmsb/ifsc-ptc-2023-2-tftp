#include "DATA.h"

using namespace std;

DATA::DATA(string & filename)
  : opcode(htons(3)), block(0), count(0), file(filename, ifstream::binary)
{
    file.read((char *)data, 512);
    
}

DATA::DATA(char & buffer) : opcode(htons(3)), block(0) {
    memcpy(data, &buffer, 512);
}

void DATA::increment(){
    block = htons(ntohs(block)+1);
    file.read((char *)data, 512*ntohs(block)); 
}

uint16_t DATA::getBlock(){
   return 0;
}

int DATA::size(){
   streamsize bytesAmount = file.gcount();
   return 4+bytesAmount; 
}

// void DATA::setData(char & data){
//     DATA::data = data;
// }
