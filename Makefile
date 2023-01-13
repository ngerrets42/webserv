NAME = webserv
BUILD_DIR = build
VPATH := $(subst $(" "),:,$(shell find srcs -type d))
SRCS += $(wildcard srcs/*.cpp srcs/*/*.cpp) 
OBJ_FILES = $(addprefix $(BUILD_DIR)/, $(notdir $(patsubst %.cpp, %.o, $(SRCS))))
HEADER_FILES := $(addprefix -I,$(shell find includes -type d -print))
CPPFLAGS = -Wall -Werror -Wextra -std=c++11
CC = clang++

all: build_dir $(NAME)

$(NAME): $(OBJ_FILES)
	@echo Assembling $(NAME)
	@$(CC) -o $@ $^ $(CPPFLAGS) 
	@echo $(NAME) has been made!

$(BUILD_DIR)/%.o: %.cpp
	@$(CC) $(HEADER_FILES) -c $(CPPFLAGS) -o $@ $<


build_dir:
	clear
	@if [ -d "./$(BUILD_DIR)" ]; then \
	echo "Build Directory Already Exists"; \
	else \
	mkdir -p $(BUILD_DIR); \
	fi

clean:
	clear
	@echo Cleaning all object files 
	@ rm -f $(BUILD_DIR)/*.o
	@echo Removing Build Directory
	@if [ -d "./$(BUILD_DIR)" ]; then \
	rmdir $(BUILD_DIR); fi

fclean: clean
	@echo Removing $(NAME)
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re build_dir