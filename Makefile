# Base file courtesy of https://spin.atomicobject.com/2016/08/26/makefile-c-projects/

# Build and source directories, target executable name
TARGET_PYBIND ?= flinng.so
BUILD_DIR ?= ./build
SRC_DIR ?= src

# Find cpp source files
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Define flags
CPP_DEBUG_FLAGS := -g -fno-omit-frame-pointer 
CPP_OPT_FLAGS := -O3 -ffast-math
CPP_WARN_FLAGS := -Wall -Werror
INC_FLAGS := $(shell python3 -m pybind11 --includes)
CPPFLAGS ?= -std=c++11 $(INC_FLAGS) $(CPP_WARN_FLAGS) $(CPP_OPT_FLAGS) $(CPP_DEBUG_FLAGS) -MMD -MP -fopenmp

all: $(BUILD_DIR)/$(TARGET_PYBIND)

# Make target pybind
$(BUILD_DIR)/$(TARGET_PYBIND): ./pybind/pybind.cpp $(OBJS)
	$(MKDIR_P) $(dir $@)
	g++ -shared -o $(BUILD_DIR)/$(TARGET_PYBIND) $(CPPFLAGS) -undefined dynamic_lookup -fPIC ./pybind/pybind.cpp $(SRCS)

# Make c++ source into object files
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	g++ $(CPPFLAGS) -c $< -o $@

# Clean command
.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)
-include $(TESTDEPS)

MKDIR_P ?= mkdir -p
