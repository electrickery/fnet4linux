CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -g
TARGET  = flexnet
LIB_NAME = 
SRC_DIR = .
OBJ_DIR = .

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

.PHONY: all clean

all: test $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^  -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

test: $(TARGET)
	./$(TARGET) -h -V
#	./$(TARGET) -d /dev/ttyUSB0 -s 9600 fnlinux.dsk
	@echo "Tests passed"
