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
        ERROR(uint8_t errorCode);
        ERROR(char bytes[], size_t size);

        uint16_t getOpcode();
        uint16_t getErrorCode();
        std::string getErrorMessage();
        char* data();
        int size();
        void setBytes(char bytes[], size_t size); 

    private:
        string tftpErrorMessages[9] = { 
            "Not defined, see error message (if any).",
            "File not found.",
            "Access violation.",
            "Disk full or allocation exceeded.",
            "Illegal TFTP operation.",
            "Unknown transfer ID.",
            "File already exists.",
            "No such user.",
            "File or parent directory does not exists."
        };
        uint16_t opcode;
        uint16_t errorCode;
        std::string errorMessage;
};

#endif
