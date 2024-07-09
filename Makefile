SRC_DIR := src
BIN_DIR := bin

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

CC := gcc
CFLAGS := -Wall -std=c99

$(BIN_DIR)/quasar: $(SRC_FILES)
	mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY := clean

clean:
	rm -rf $(BIN_DIR)