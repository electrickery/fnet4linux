CC      = gcc
CFLAGS  = -Wall -Wextra -Werror -Wpedantic -g
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
	@echo " #### Test1 ####"
	./$(TARGET) -h -V
	@echo " #### Test2 ####"
	./$(TARGET) -t -d /dev/ttyUSB0 -s 9600 fnlinux.dsk
	@echo " #### Test3 ####"
	./$(TARGET) -t -d /dev/ttyUSB0 -s 9600 -0 ./fnlinux.dsk
	@echo " #### Test4 ####"
	./$(TARGET) -t -d /dev/ttyUSB0 -s 9600 \
	    -0 ./fnlinux.dsk \
	    -1 ./fnlinux2.dsk \
	    -2 ./fnlinux2.dsk
	@echo "Tests passed"
