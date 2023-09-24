#ifndef DATA_H
#define DATA_H

#include <cstdint>
#include <arpa/inet.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <cstring>

class DATA {
    public:
        DATA(std::string & filename);
        DATA(char & buffer);

        uint16_t getBlock();
        uint16_t getOpcode();
        const char* getData();
        void increment();
        void setData(char _data[], size_t size);
        int size();
        int dataSize();
        void setBytes(char bytes[], size_t size);

    private:
        uint16_t opcode;
        uint16_t block;
        char data[512];
        int count;
	std::ifstream file;
        std::streamsize bytesAmount;
};

#endif
