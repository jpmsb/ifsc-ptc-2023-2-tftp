#ifndef ERROR_H
#define ERROR_H

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <string>

using namespace std;

class ERROR {
    public:
        ERROR();
        ERROR(char bytes[], size_t size);

        uint16_t getOpcode();
        uint16_t getErrorCode();
        std::string getErrorMessage();
        void setBytes(char bytes[], size_t size); 

    private:
        uint16_t opcode;
        uint16_t errorCode;
        std::string errorMessage;
};

#endif
