CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS =

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
BUILD_DIR = build

# Source files
FRAMING_SRC = $(SRC_DIR)/framing/framing.c
RTP_SRC = $(SRC_DIR)/rtp/rtp.c
NETWORK_SRC = $(SRC_DIR)/udp_ip/network.c
TEST_SRC = $(TEST_DIR)/test_packetization.c
MP3_TEST_SRC = $(TEST_DIR)/test_mp3_streaming.c

# Object files
FRAMING_OBJ = $(BUILD_DIR)/framing.o
RTP_OBJ = $(BUILD_DIR)/rtp.o
NETWORK_OBJ = $(BUILD_DIR)/network.o
TEST_OBJ = $(BUILD_DIR)/test_packetization.o
MP3_TEST_OBJ = $(BUILD_DIR)/test_mp3_streaming.o

# Target executables
TARGET = $(BUILD_DIR)/test_packetization
MP3_TARGET = $(BUILD_DIR)/test_mp3_streaming

.PHONY: all clean test run mp3 test-mp3

all: $(TARGET) $(MP3_TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(FRAMING_OBJ): $(FRAMING_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(RTP_OBJ): $(RTP_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(NETWORK_OBJ): $(NETWORK_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_OBJ): $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(FRAMING_OBJ) $(RTP_OBJ) $(NETWORK_OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(MP3_TEST_OBJ): $(MP3_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(MP3_TARGET): $(FRAMING_OBJ) $(RTP_OBJ) $(NETWORK_OBJ) $(MP3_TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TARGET)
	@echo "Running basic test..."
	@./$(TARGET)

test-mp3: $(MP3_TARGET)
	@echo "Running MP3 streaming test..."
	@./$(MP3_TARGET)

mp3: test-mp3

run: test

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Available targets:"
	@echo "  all      - Build all test programs"
	@echo "  test     - Build and run basic test"
	@echo "  test-mp3 - Build and run MP3 streaming test"
	@echo "  mp3      - Same as test-mp3"
	@echo "  run      - Same as test"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help message"
