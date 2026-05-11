# Academia Portal - Makefile
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -pthread

SERVER_SRCS = server/server.c server/utils.c server/db.c server/menus.c
SERVER_BIN  = server/server_bin
CLIENT_BIN  = client/client_bin

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRCS) server/server.h
	$(CC) $(CFLAGS) $(SERVER_SRCS) -o $(SERVER_BIN)

$(CLIENT_BIN): client/client.c
	$(CC) $(CFLAGS) client/client.c -o $(CLIENT_BIN)

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) data/*.tmp data/notify.fifo

.PHONY: all clean