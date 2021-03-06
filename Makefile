CC = gcc -Wall
OPT = -O0
LDFLAGS = -ggdb -lpthread
S_COMMON = server/main_routine.c libs/logger.c libs/protocol.c libs/list.c server/thread_routine.c
C_COMMON = libs/protocol.c
CMN = libs/common.h

all: clean cartella server_linux linux_client

clean:
	rm -rf build
cartella:
	mkdir -p build
server_linux:
	$(CC) $(OPT) $(S_COMMON) server/server_linux.c -o build/server_linux $(LDFLAGS)
linux_client:
	$(CC) $(OPT) $(C_COMMON) client/linux_client.c -o build/linux_client $(LDFLAGS)
test_linux_client:
	$(CC) $(OPT) $(S_COMMON) client/test_linux_client.c -o build/test_linux_client $(LDFLAGS)
test_field:
	$(CC) $(OPT) $(C_COMMON) client/test_field.c -o build/test_field $(LDFLAGS)
