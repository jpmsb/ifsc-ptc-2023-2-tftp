#ifndef ACK_H
#define ACK_H

#include <cstdint>
#include <arpa/inet.h>

class ACK {
    public:
        ACK();

        uint16_t get_block();
        void increment();
        void set();        

    private:
        uint16_t opcode;
        uint16_t block;
};

#endif
