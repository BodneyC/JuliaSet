CC=gcc
MPICC=mpicc
LIBS=-lm
IDIR=include
SDIR=src
ODIR=obj
BDIR=bin
CFLAGS=include

LIB_SRC=$(wildcard $(IDIR)/*.c)
LIB_OBJ=$(patsubst $(IDIR)/%.c, $(ODIR)/%.o, $(LIB_SRC))
SRC=$(wildcard $(SDIR)/*.c)
OBJ=$(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(SRC))
BIN=$(patsubst $(SDIR)/%.c, $(BDIR)/%, $(SRC))
DEPS=$(wildcard $(ODIR)/c*.o)

all: $(LIB_OBJ) $(OBJ) $(BIN)

$(BIN): $(BDIR)/%: $(ODIR)/%.o
	$(MPICC) -o $@ -I $(CFLAGS) $(LIBS) $(DEPS) $<

$(OBJ): $(ODIR)/%.o: $(SDIR)/%.c
	$(MPICC) -o $@ -I $(CFLAGS) $(LIBS) -c $<

$(LIB_OBJ): $(ODIR)/%.o: $(IDIR)/%.c
	$(CC) -o $@ $(LIBS) -c $<

DEBUG: LIBS+=-DDEBUG
DEBUG: all

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o

cleaner:
	rm -f $(ODIR)/*.o $(BDIR)/* im*
