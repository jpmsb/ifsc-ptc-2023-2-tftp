# Makefile para compilar o servidor TFTP

CC = g++
CFLAGS = -Wl,--copy-dt-needed-entries -lprotobuf 

# Lista de arquivos fonte
SRCS = BaseSocket.cpp UDPSocket.cpp Callback.cpp poller.cpp ACK.cpp DATA.cpp RRQ.cpp WRQ.cpp ERROR.cpp TFTP.cpp tftp2.pb.cc cliente.cpp

# Nome do executável
TARGET = cliente

all: $(TARGET)

$(TARGET): $(SRCS)
	rm -f $(TARGET)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

