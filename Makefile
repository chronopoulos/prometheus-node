RM = rm -rf
OPTIMIZATION = -O3
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc

BINDIR = bin
INCDIR = include
SRCDIR = src

BIN = promNode
OUTPUT = $(BINDIR)/$(BIN)

$(info INFO: Scanning for C sources...)
CSOURCES += $(shell find -L $(SRCDIR) -name '*.c')

$(info INFO: Scanning for C header files...)
INC = $(shell find -L $(INCDIR) -name '*.h' -exec dirname {} \; | uniq)
INC += $(INCDIR)
INCLUDES = $(INC:%=-I%)

LDLIBS = -lpthread -lrt -lm

OBJECTS += $(CSOURCES:%.c=%.o)

CFLAGS = -Wall -c -std=c99 -g -mtune=cortex-a8 -march=armv7-a $(OPTIMIZATION) $(INCLUDES) 
LDFLAGS = # -Xlinker --verbose

$(info INFO: Compiling...)
all: $(BINDIR) $(OUTPUT)

$(BINDIR):
	mkdir -p $(BINDIR)

$(OUTPUT): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

deploy: all
	$(info INFO: Deploying...)
	$(DEPLOY1)
	$(DEPLOY2)
	$(DEPLOY3)

clean:
	$(RM) $(OBJECTS) $(BINDIR)
