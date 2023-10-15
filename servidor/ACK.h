#ifndef ACK_H
#define ACK_H

#include <cstdint>
#include <arpa/inet.h>
#include <cstring>

class ACK {
    public:
        ACK();
        ACK(char bytes[]);

        uint16_t getBlock();
        uint16_t getOpcode();
        void increment();
        void setBytes(char bytes[]);        

    private:
        uint16_t opcode;
        uint16_t block;
};

#endif
