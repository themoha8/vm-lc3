PROGRAM_NAME = lc3
OBJ_PATH = ./obj/
SRCMODULES = lc3.c
OBJMODULES = $(addprefix $(OBJ_PATH), $(SRCMODULES:.c=.o))

ifeq ($(RELEASE), 1)
	CFLAGS = -Wall -Werror -Wextra -ansi -pedantic -O3
else
	CFLAGS = -Wall -Werror -Wextra -ansi -pedantic -g -O0
endif

CC = gcc 

$(PROGRAM_NAME): main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_PATH)%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_PATH)deps.mk: $(SRCMODULES)
	$(CC) -MM $^ > $@

.PHONY: clean
clean:
	rm -f $(OBJ_PATH)*.o $(PROGRAM_NAME) $(OBJ_PATH)deps.mk

ifneq (clean, $(MAKECMDGOALS))
-include $(OBJ_PATH)deps.mk
endif

