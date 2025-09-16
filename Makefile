CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = -lpthread
TARGETS = server client
BUILD_DIR = build
EXECUTABLES = $(TARGETS:%=$(BUILD_DIR)/%)

SERVER_OBJS = $(BUILD_DIR)/server.o $(BUILD_DIR)/server_main.o $(BUILD_DIR)/client_list.o $(BUILD_DIR)/common.o
CLIENT_OBJS = $(BUILD_DIR)/client.o $(BUILD_DIR)/client_main.o $(BUILD_DIR)/common.o

.PHONY: all clean mkbuild

all: mkbuild $(EXECUTABLES)

mkbuild:
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/server.o: src/server/server.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/client_list.o: src/server/client_list.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/server_main.o: src/server/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/client.o: src/client/client.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/client_main.o: src/client/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/common.o: src/common/common.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/server.o: include/server.h include/common.h include/client_list.h
$(BUILD_DIR)/client_list.o: include/client_list.h
$(BUILD_DIR)/server_main.o: include/server.h include/common.h
$(BUILD_DIR)/client.o: include/client.h include/common.h
$(BUILD_DIR)/client_main.o: include/client.h include/common.h
$(BUILD_DIR)/common.o: include/common.h

clean:
	rm -rf $(BUILD_DIR)

build: all

run-server: $(BUILD_DIR)/server
	sudo $<

run-client: $(BUILD_DIR)/client
	sudo $<