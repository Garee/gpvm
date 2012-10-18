SHELL = /bin/sh

.SUFFIXES:
.SUFFIXES: .cpp .o
.PHONY: all distclean clean

SRCDIR = src
OBJDIR = obj
INCDIR = include

SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(addprefix $(OBJDIR)/, $(notdir $(SRC:.cpp=.o)))
LIB = -lOpenCL
BIN = vm

CC = g++
CFLAGS = -Wall -Wextra -pedantic -Werror -g -I $(INCDIR)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN) $(LIB)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

distclean: clean
	rm -f $(BIN)

clean:
	rm -f $(OBJ)
	if test -d $(OBJDIR); then rmdir $(OBJDIR); fi


