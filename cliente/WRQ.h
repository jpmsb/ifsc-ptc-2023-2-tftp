#ifndef WRQ_H
#define WRQ_H

#include <cstdint>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <vector>

using namespace std;

class WRQ {
    public:
        WRQ(string & _filename, string _mode = "octet");
        int size();
        char* data();

    private:
        uint16_t opcode;
        vector<char> filename;
        vector<char> mode;
        int opcodeSize = 2;
        int filenameSize;
        int modeSize;
};

#endif
