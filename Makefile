TARGET=dit
LDFLAGS=-lm -lreadline

CC=cc
OPT=-O0
DEPFLAGS=-MP -MD
WFLAGS=-Wall -Wextra
INCFLAGS=-I $(INCDIR)
CFLAGS=$(WFLAGS) $(INCFLAGS) $(OPT) $(DEPFLAGS)

INCDIR=./include
SRCDIR=./src
BLDDIR=./build

INCFILES=$(wildcard $(INCDIR)/*.h)
SRCFILES=$(wildcard $(SRCDIR)/*.c)
OBJFILES=$(patsubst $(SRCDIR)/%.c,$(BLDDIR)/%.o,$(SRCFILES))
DEPFILES=$(patsubst $(SRCDIR)/%.c,$(BLDDIR)/%.d,$(SRCFILES))

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $(TARGET)

$(BLDDIR)/%.o: $(SRCDIR)/%.c $(INCFILES) Makefile
	$(CC) -g $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(BLDDIR)/*

test: $(TARGET)
	./$(TARGET)

.PHONY:
	all clean test
