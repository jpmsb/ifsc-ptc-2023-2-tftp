#ifndef Request_H
#define Request_H

#include <cstdint>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <vector>

class Request {
    public:
        Request(char bytes[], size_t size);

        uint16_t getOpcode();
        std::string getFilename();
        std::string getMode();
        int size();
        char* data();

    private:
        uint16_t opcode;
        std::string filename;
        std::string mode;
        uint16_t networkOrderOpcode;
        int opcodeSize;
        int filenameSize;
        int modeSize;
};

#endif
