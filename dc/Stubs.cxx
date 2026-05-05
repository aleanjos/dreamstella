#include <string>

class SerialPortUNIX {
public:
    SerialPortUNIX();
    virtual ~SerialPortUNIX();
    bool open(const std::string& port);
    void close();
};

SerialPortUNIX::SerialPortUNIX() { }
SerialPortUNIX::~SerialPortUNIX() { }

bool SerialPortUNIX::open(const std::string& port) { 
    return false; 
}

void SerialPortUNIX::close() { }

extern "C" int access(const char *pathname, int mode) {
    struct stat st;
    if (stat(pathname, &st) == 0) {
        return 0;
    }
    return -1;
}
