#include "RRQ.h"

using namespace std;

RRQ::RRQ(string & _filename, string _mode)
    : opcode(htons(1)) {
    filename.insert(filename.end(), _filename.begin(), _filename.end());
    filename.push_back('\0');
    filenameSize = filename.size();
    
    mode.insert(mode.end(), _mode.begin(), _mode.end());
    mode.push_back('\0');
    modeSize = mode.size();
}

int RRQ::size(){
    return opcodeSize + filenameSize + modeSize;
}

char* RRQ::data(){
     char* data = new char[size()];
     memcpy(data, &opcode, sizeof(opcode));
     memcpy(data + opcodeSize, filename.data(), filenameSize);
     memcpy(data + opcodeSize + filenameSize, mode.data(), modeSize);

     return data;
}
