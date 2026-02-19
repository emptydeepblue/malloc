# Colors
GREEN = \033[0;32m
YELLOW = \033[0;33m
RED = \033[0;31m
RESET = \033[0m

# Set HOSTTYPE
ifeq ($(HOSTTYPE),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

# Project name
NAME = libft_malloc_$(HOSTTYPE).so
LINK = libft_malloc.so

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -fPIC -g
LDFLAGS = -shared -lpthread

# Directories
SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj
LIBFT_DIR = libft

# Source files
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Libft
LIBFT = $(LIBFT_DIR)/libft.a

# Include paths
INC = -I$(INC_DIR) -I$(LIBFT_DIR)

# Rules
all: $(LIBFT) $(NAME)
	@ln -sf $(NAME) $(LINK)
	@echo "$(GREEN)✓ $(NAME) created$(RESET)"
	@echo "$(GREEN)✓ Symbolic link $(LINK) -> $(NAME)$(RESET)"

$(NAME): $(OBJ)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBFT)
	@echo "$(GREEN)✓ Linking complete$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(INC) -c $< -o $@
	@echo "$(YELLOW)Compiling: $<$(RESET)"

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(LIBFT):
	@$(MAKE) -C $(LIBFT_DIR)
	@$(MAKE) bonus -C $(LIBFT_DIR)

clean:
	@$(MAKE) -C $(LIBFT_DIR) clean
	@rm -rf $(OBJ_DIR)
	@echo "$(RED)✗ Object files removed$(RESET)"

fclean: clean
	@$(MAKE) -C $(LIBFT_DIR) fclean
	@rm -f $(NAME) $(LINK) test
	@echo "$(RED)✗ $(NAME) and $(LINK) removed$(RESET)"

re: fclean all

test: all
	@echo "$(YELLOW)Compiling test.c...$(RESET)"
	@gcc -o test test.c -L. -lft_malloc -lpthread
	@echo "$(YELLOW)Running tests...$(RESET)"
	@export LD_LIBRARY_PATH=. && LD_PRELOAD=./libft_malloc.so ./test

vtest: all
	@echo "$(YELLOW)Compiling vtest.c...$(RESET)"
	@gcc -o test test.c -L. -lft_malloc -lpthread
	@echo "$(YELLOW)Running valgrind tests...$(RESET)"
	@export LD_LIBRARY_PATH=. && LD_PRELOAD=./libft_malloc.so valgrind --leak-check=full --track-origins=yes ./test
	@echo "$(YELLOW)NOTE: Valgrind will affect the show_alloc_mem output due to its instrumentation. Focus on memory leaks and errors reported.$(RESET)"
	@echo "$(YELLOW)NOTE2: 1 error in 1 context is expected for the double free test. Focus on other errors.$(RESET)"

htest: all
	@echo "$(YELLOW)Compiling htest.c...$(RESET)"
	@gcc -o test test.c -L. -lft_malloc -lpthread
	@echo "$(YELLOW)Running helgrind tests...$(RESET)"
	@export LD_LIBRARY_PATH=. && LD_PRELOAD=./libft_malloc.so valgrind --tool=helgrind --track-lockorders=yes ./test
	@echo "$(YELLOW)NOTE: Helgrind will affect the show_alloc_mem output due to its instrumentation. Focus on thread safety issues reported.$(RESET)" 

.PHONY: all clean fclean re test vtest htest
