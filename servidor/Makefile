# Makefile para compilar o servidor TFTP

CC = g++
CFLAGS = -Wl,--copy-dt-needed-entries -lprotobuf 

# Lista de arquivos fonte
SRCS = BaseSocket.cpp UDPSocket.cpp Callback.cpp poller.cpp ACK.cpp DATA.cpp ERROR.cpp TFTPServer.cpp Request.cpp tftp2.pb.cc servidor.cpp

# Nome do executável
TARGET = servidor

all: $(TARGET)

$(TARGET): $(SRCS)
	rm -f $(TARGET)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

