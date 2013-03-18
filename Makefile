SHELL = /bin/sh

.SUFFIXES:
.SUFFIXES: .cpp .o
.PHONY: all test distclean clean 

SRCDIR = src
OBJDIR = obj
INCDIR = include

SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRC:.cpp=.o)))
LIB = -lOpenCL
BIN = vm
TEST = vmtest

CC = g++
CFLAGS = -Wall -Wextra -pedantic -Werror -g -I $(INCDIR)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN) $(LIB)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

test:
	gcc -w -std=c99 -I $(INCDIR) tests/vmtest.c -o $(TEST)
	./vmtest

distclean: clean
	rm -f $(BIN) $(TEST)

clean:
	rm -f $(OBJ)
	if test -d $(OBJDIR); then rmdir $(OBJDIR); fi


