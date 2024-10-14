# Compiler and flags
CC = g++
CFLAGS = -std=c++17 -Wall

# Target executable
TARGET = myfind

# Source files
SRC = myfind.cpp

# Build the executable
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean up the build artifacts
clean:
	rm -f $(TARGET)