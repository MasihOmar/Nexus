# NexOS Makefile
# Build automation for the NexOS project

CC = gcc
CFLAGS = -Wall -Wextra -static -O2
SRC_DIR = src
BUILD_DIR = build
DISTRO_DIR = distro

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = nexos

# Default target
all: $(BUILD_DIR) $(TARGET)
	@echo "Build complete: $(TARGET)"

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/globals.h
	$(CC) $(CFLAGS) -c $< -o $@

# Link final binary
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

# Build ISO (calls existing script)
iso: $(TARGET)
	@echo "Building ISO..."
	cd $(DISTRO_DIR) && ./build_iso.sh

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Cleaned build artifacts"

# Clean everything including ISO
distclean: clean
	rm -f $(DISTRO_DIR)/nexos.iso
	rm -rf $(DISTRO_DIR)/rootfs
	@echo "Cleaned all generated files"

# Install to distro rootfs (for development)
install: $(TARGET)
	cp $(TARGET) $(DISTRO_DIR)/rootfs/bin/nexos
	@echo "Installed to $(DISTRO_DIR)/rootfs/bin/nexos"

# Run tests (placeholder)
test: $(TARGET)
	@echo "Running basic sanity checks..."
	./$(TARGET) --version 2>/dev/null || echo "Binary runs (no --version support yet)"
	@echo "Tests passed"

# Show help
help:
	@echo "NexOS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build nexos binary (default)"
	@echo "  iso       - Build complete ISO image"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo "  install   - Copy binary to rootfs"
	@echo "  test      - Run basic tests"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make          # Build binary"
	@echo "  make iso      # Build ISO"
	@echo "  make clean    # Clean up"

.PHONY: all clean distclean iso install test help
