CC = gcc
CFLAGS = -Wall -Wextra -g -I./include -pthread
TARGET = nvram_db

# Source files
SRC = src/db_main.c src/free_space.c src/ram_bptree.c src/wal.c src/lock_manager.c

# Object files
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET) 

# Link object files to create executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TARGET) 

# Run the database
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean 