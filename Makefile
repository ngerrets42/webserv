CXX := clang++

# -------------------      TARGET       -------------------

TARGET ?= webserv

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src
INC_DIRS ?= ./include ./lib/njson/include
CPPFLAGS ?= -Wall -Wextra -std=c++11
LDFLAGS ?= 

# -------------------        LIB        -------------------

NJSON_DIR := ./lib/njson
LDFLAGS += -L"./lib/njson" -lnjson

# --------------------------- END -------------------------

SRCS := $(shell find $(SRC_DIRS) -name *.cpp)
HDRS := $(shell find $(INC_DIRS) -name *.h)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

INC_FLAGS := $(addprefix -I,$(INC_DIRS))
CPPFLAGS += $(INC_FLAGS) -MMD -MP

.PHONY: all lib
all: lib $(TARGET)

lib:
	$(MAKE) -C $(NJSON_DIR)

# linking
$(TARGET): $(OBJS)
	@echo "Linking..."
	@$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Done!"

# compiling
$(BUILD_DIR)/%.cpp.o: %.cpp $(HDRS)
	@$(MKDIR_P) $(dir $@)
	@echo "Compiling: " $<
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean fclean re
clean:
	$(RM) -r $(BUILD_DIR)

fclean: clean
	$(RM) $(TARGET)

re: fclean all

MKDIR_P ?= mkdir -p
