#ifndef DATA_H
#define DATA_H

#include <cstdint>
#include <arpa/inet.h>
#include <algorithm>
#include <fstream>
#include <string>

class DATA {
    public:
        DATA(std::string & filename);

        uint16_t get_block();
        void increment();
        void setData(char & data); 

    private:
        uint16_t opcode;
        uint16_t block;
        char data[512];
        int count = 0;
	std::ifstream file;
};

#endif
