#ifndef DATA_H
#define DATA_H

#include <cstdint>
#include <arpa/inet.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <cstring>
#include "tftp2.pb.h"

class DATA {
    public:
        DATA();
        DATA(const std::string & filename);
        DATA(char bytes[], size_t size);
        DATA(tftp2::Mensagem & message);

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
        bool isFile;
        std::string serializedMessage;
};

#endif
