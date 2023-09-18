#ifndef DATA_H
#define DATA_H

#include <cstdint>
#include <arpa/inet.h>

class DATA {
    public:
        DATA();

        uint16_t get_block();
        void increment();
        void set();        

    private:
        uint16_t opcode;
        uint16_t block;
};

#endif
