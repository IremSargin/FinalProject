CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS =

# Directories
SRC_DIR = src
INC_DIR = include
TEST_DIR = tests
APPS_DIR = apps
BUILD_DIR = build

# Common source files
FRAMING_SRC = $(SRC_DIR)/framing/framing.c
RTP_SRC = $(SRC_DIR)/rtp/rtp.c
NETWORK_SRC = $(SRC_DIR)/udp_ip/network.c

# Sender/Receiver source files
SENDER_MODULE_SRC = $(SRC_DIR)/sender/sender.c
RECEIVER_MODULE_SRC = $(SRC_DIR)/receiver/receiver.c

# Application source files
SENDER_APP_SRC = $(APPS_DIR)/sender.c
RECEIVER_APP_SRC = $(APPS_DIR)/receiver.c

# Test source files
TEST_SRC = $(TEST_DIR)/test_packetization.c
MP3_TEST_SRC = $(TEST_DIR)/test_mp3_streaming.c

# Common object files
FRAMING_OBJ = $(BUILD_DIR)/framing.o
RTP_OBJ = $(BUILD_DIR)/rtp.o
NETWORK_OBJ = $(BUILD_DIR)/network.o

# Sender/Receiver object files
SENDER_MODULE_OBJ = $(BUILD_DIR)/sender.o
RECEIVER_MODULE_OBJ = $(BUILD_DIR)/receiver.o

# Application object files
SENDER_APP_OBJ = $(BUILD_DIR)/sender_app.o
RECEIVER_APP_OBJ = $(BUILD_DIR)/receiver_app.o

# Test object files
TEST_OBJ = $(BUILD_DIR)/test_packetization.o
MP3_TEST_OBJ = $(BUILD_DIR)/test_mp3_streaming.o

# Executables
SENDER_BIN = $(BUILD_DIR)/sender
RECEIVER_BIN = $(BUILD_DIR)/receiver
TEST_BIN = $(BUILD_DIR)/test_packetization
MP3_TEST_BIN = $(BUILD_DIR)/test_mp3_streaming

.PHONY: all clean test test-mp3 sender receiver help stream

# Build everything
all: $(TEST_BIN) $(MP3_TEST_BIN) $(SENDER_BIN) $(RECEIVER_BIN)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Common modules
$(FRAMING_OBJ): $(FRAMING_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(RTP_OBJ): $(RTP_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(NETWORK_OBJ): $(NETWORK_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Sender module
$(SENDER_MODULE_OBJ): $(SENDER_MODULE_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Receiver module
$(RECEIVER_MODULE_OBJ): $(RECEIVER_MODULE_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Test programs
$(TEST_OBJ): $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(FRAMING_OBJ) $(RTP_OBJ) $(NETWORK_OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(MP3_TEST_OBJ): $(MP3_TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(MP3_TEST_BIN): $(FRAMING_OBJ) $(RTP_OBJ) $(NETWORK_OBJ) $(MP3_TEST_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Sender application
$(SENDER_APP_OBJ): $(SENDER_APP_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SENDER_BIN): $(FRAMING_OBJ) $(RTP_OBJ) $(NETWORK_OBJ) $(SENDER_MODULE_OBJ) $(SENDER_APP_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Receiver application
$(RECEIVER_APP_OBJ): $(RECEIVER_APP_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(RECEIVER_BIN): $(FRAMING_OBJ) $(RTP_OBJ) $(NETWORK_OBJ) $(RECEIVER_MODULE_OBJ) $(RECEIVER_APP_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Run tests
test: $(TEST_BIN)
	@echo "Running basic test..."
	@./$(TEST_BIN)

test-mp3: $(MP3_TEST_BIN)
	@echo "Running MP3 streaming test..."
	@./$(MP3_TEST_BIN)

# Build sender
sender: $(SENDER_BIN)
	@echo "Sender built successfully: $(SENDER_BIN)"

# Build receiver
receiver: $(RECEIVER_BIN)
	@echo "Receiver built successfully: $(RECEIVER_BIN)"

# Run streaming demo (automated loopback test)
stream: $(SENDER_BIN) $(RECEIVER_BIN)
	@echo "=========================================="
	@echo "  RTP Streaming Loopback Test"
	@echo "=========================================="
	@echo ""
	@echo "Starting receiver in background..."
	@./$(RECEIVER_BIN) 5005 received.mp3 15 &
	@sleep 2
	@echo ""
	@echo "Starting sender..."
	@./$(SENDER_BIN) ses.mp3 127.0.0.1 5005
	@sleep 1
	@echo ""
	@echo "Comparing files..."
	@if cmp -s ses.mp3 received.mp3; then \
		echo "✓ SUCCESS: Files are identical!"; \
	else \
		echo "✗ WARNING: Files differ!"; \
		ls -lh ses.mp3 received.mp3; \
	fi
	@echo ""

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) received.mp3

# Help
help:
	@echo "Available targets:"
	@echo "  all        - Build all programs (tests + sender + receiver)"
	@echo "  test       - Build and run basic test"
	@echo "  test-mp3   - Build and run MP3 streaming test"
	@echo "  sender     - Build sender application"
	@echo "  receiver   - Build receiver application"
	@echo "  stream     - Run full loopback test (sender -> receiver)"
	@echo "  clean      - Remove build artifacts"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Manual usage:"
	@echo "  Terminal 1: ./build/receiver [port] [output_file] [timeout]"
	@echo "  Terminal 2: ./build/sender [input_file] [dest_ip] [dest_port]"
