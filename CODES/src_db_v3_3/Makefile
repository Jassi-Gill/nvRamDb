# ============================================================
# PostgreSQL Extension Build (Dedicated PGXS Makefile)
# ============================================================

EXTENSION = mytam
MODULE_big = mytam
DATA = mytam--1.0.sql

# Object files to build into the shared library
OBJS = src/tam.o src/free_space.o src/ram_bptree.o src/wal.o src/lock_manager.o

# Path to pg_config
PG_CONFIG = /usr/lib/postgresql/14/bin/pg_config

# Add project-specific include directories
PG_CPPFLAGS += -I./include -I./src

# Link against PostgreSQL server build system (PGXS)
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

# Optional: custom clean target to also remove build intermediates
clean:
	rm -f $(OBJS) mytam.so
	rm -f sql/*~ *.o
	rm -rf results



# # --- Part 1: Configuration for Standalone Client/Server ---
# CC = gcc
# CFLAGS = -Wall -Wextra -Wno-missing-prototypes -Wno-declaration-after-statement -g -I./include -pthread -march=native -fPIC


# SERVER_TARGET = nvram_db
# CLIENT_TARGET = nvram_client

# BACKEND_SRC = src/free_space.c src/ram_bptree.c src/wal.c src/lock_manager.c
# SERVER_SRC = src/db_main.c $(BACKEND_SRC)
# CLIENT_SRC = src/client.c

# SERVER_OBJ = $(SERVER_SRC:.c=.o)
# CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

# # --- Part 2: PostgreSQL TAM Extension ---
# EXTENSION = mytam
# DATA = mytam--1.0.sql
# MODULE_big = mytam
# OBJS = src/tam.o src/free_space.o src/ram_bptree.o src/wal.o src/lock_manager.o

# PG_CONFIG = /usr/lib/postgresql/14/bin/pg_config

# # add your own include directories here
# PG_CPPFLAGS += -I./include -I./src

# PGXS := $(shell $(PG_CONFIG) --pgxs)
# include $(PGXS)


# # --- Part 3: Build Rules ---

# # Default target builds everything
# all: $(SERVER_TARGET) $(CLIENT_TARGET) mytam.so

# # Only your own binaries use your CFLAGS rule
# $(SERVER_TARGET): $(SERVER_OBJ)
# 	$(CC) $(CFLAGS) -o $@ $^

# $(CLIENT_TARGET): $(CLIENT_OBJ)
# 	$(CC) $(CFLAGS) -o $@ $^

# # Compile standalone .c files (NOT used by PGXS)
# src/db_main.o src/client.o: %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# # --- Part 4: Utility Rules ---
# clean:
# 	rm -f $(SERVER_OBJ) $(CLIENT_OBJ)
# 	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)
# 	rm -f *.so sql/*~ *.o
# 	rm -rf results

# server: $(SERVER_TARGET)
# 	sudo ./$(SERVER_TARGET)

# client: $(CLIENT_TARGET)
# 	./$(CLIENT_TARGET)

# .PHONY: all clean server client
