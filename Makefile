# Compiler and flags
CXX := g++

# Versioning: Major.Minor.Patches
VERSION    ?= 1.0.0
DATE       := $(shell date +%Y-%m-%d)
DATETIME   := $(shell date +%Y-%m-%d_%H:%M:%S)

GDAL_INC := /usr/include/gdal
GDAL_LIB := /usr/lib64

# Directories
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
INCLUDE_DIR := include

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# Include and link flags
INCLUDES := -I$(GDAL_INC) -I$(INCLUDE_DIR)
LDFLAGS := -L$(GDAL_LIB) -ljpeg -lz  # For Ubuntu with alternative libs

GDAL_CFLAGS = $(shell gdal-config --cflags)
GDAL_LIBS = $(shell gdal-config --libs)

# Build types
DEBUG_FLAGS := -g -O0 -Wall -Wextra -std=c++11
RELEASE_FLAGS := -O3 -Wall -Wextra -std=c++11
VERSION_FLAGS := \
    -DAPP_VERSION=\"$(VERSION)\" \
    -DAPP_DATE=\"$(DATE)\" \
    -DAPP_DATETIME=\"$(DATETIME)\"

# Default target
all: release

# Release build
release: CXXFLAGS := $(RELEASE_FLAGS) $(INCLUDES) $(VERSION_FLAGS)
release: $(BIN_DIR)/clip

# Debug build
debug: CXXFLAGS := $(DEBUG_FLAGS) $(INCLUDES) $(VERSION_FLAGS)
debug: $(BIN_DIR)/clip

# Link target
$(BIN_DIR)/clip: $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(GDAL_CFLAGS) -o $@ $^ $(LDFLAGS) $(GDAL_LIBS)

# Compile each .cpp to .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure bin/ and obj/ dirs exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean build files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean debug release

# make          # builds in release mode
# make release  # explicitly build release
# make debug    # builds with debug symbols (-g)
# make clean    # removes all build artifacts
