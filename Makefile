# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Directories
BUILD_DIR = build

# Source files
SRC = $(wildcard *.c)

# Object files (in build directory)
OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRC))

# Executable name (in build directory)
EXEC = $(BUILD_DIR)/main

# Default target to build the executable
all: $(EXEC)

# Rule to link object files into the executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) -lncurses

# Rule to compile .c files into .o files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run the executable
run:
	./$(EXEC)

# Clean up compiled files
clean:
	rm -rf $(BUILD_DIR)

# Phony targets (not files)
.PHONY: all clean run
