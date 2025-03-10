BUILD_DIR = build
bin = build/cuesh

HEADERS = $(shell find include -name '*.h') $(shell find include -name '*.hpp')

SOURCES_C = $(shell find src -name '*.c')
OBJECTS_C = $(patsubst src/%.c, src/%.o, $(SOURCES_C))

SOURCES_CPP = $(shell find src -name '*.cpp')
OBJECTS_CPP = $(patsubst src/%.cpp, src/%.o, $(SOURCES_CPP))

INCLUDES = -Iinclude
CFLAGS = -Wall -Wextra -Werror -g $(INCLUDES)
CXXFLAGS = $(CFLAGS) -std=c++17



OBJECTS = $(OBJECTS_C) $(OBJECTS_CPP)

.PHONY: all clean setup run

all: setup $(bin)

$(bin): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	@echo "Linking..."
	@g++ $(CXXFLAGS)  -o $(bin) $(OBJECTS) -lreadline

src/%.o: src/%.c $(HEADERS)
	@echo "Compiling $<..."
	@gcc $(CFLAGS) -c $< -o $@

src/%.o: src/%.cpp $(HEADERS)
	@echo "Compiling $<..."
	@g++ $(CXXFLAGS) -c $< -o $@

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
