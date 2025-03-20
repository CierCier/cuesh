BUILD_DIR = build
bin = build/cuesh

HEADERS = $(shell find -name '*.hpp')

SOURCES = $(shell find  -name '*.cpp')
OBJECTS = $(patsubst %.cpp, %.o, $(SOURCES))

INCLUDES = -I.
CFLAGS =  -g $(INCLUDES)
CXXFLAGS = $(CFLAGS) -std=c++23 -stdlib=libc++

CXX = clang++
CC = clang

.PHONY: all clean setup run

all: setup $(bin)

$(bin): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	@echo "Linking..."
	@$(CXX) $(CXXFLAGS)  -o $(bin) $(OBJECTS) -lreadline


src/%.o: src/%.cpp
	@echo "Compiling $<..."
	@$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(OBJECTS)


run: $(bin)
	@echo "Running..."
	@$(bin)


setup:
	@echo "Setting up..."
	@mkdir -p $(BUILD_DIR)


compile_commands.json: $(HEADERS) $(SOURCES_C) $(SOURCES_CPP)
	@echo "Generating compile_commands.json..."
	@bear -- $(MAKE) all

