RM = rm -rf
OPTIMIZATION = -O3
CROSS_COMPILE ?= arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc

BINDIR = ../bin
INCDIR = ../include
SRCDIR = ../src

BIN = tof-imager
OUTPUT = $(BINDIR)/$(BIN)
INSTALLPATH = /home/tof-imager
ip ?= 192.168.7.2# DME660 IP over USB; May be set in build command.
PUTTY_PATH ?= C:\putty# Path to pscp and plink; May be set in build command. 

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

ifeq ($(OS),Windows_NT) # TODO: Find out why ifeq cannot be placed in deploy part.
	DEPLOY1 = $(PUTTY_PATH)\\pscp $(BINDIR)/$(BIN) root@$(ip):
	DEPLOY2 = $(PUTTY_PATH)\\plink root@$(ip) "chmod +x ~/tof-imager"
	DEPLOY3 = $(PUTTY_PATH)\\plink root@$(ip) "mv ~/tof-imager /home/tof-imager/tof-imager"
else
	DEPLOY1 = rsync $(BINDIR)/* root@$(ip):$(INSTALLPATH)/
endif

$(info INFO: Compiling...)
all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

cdepend: .cdepend
.cdepend: $(CSOURCES)
	$(RM) ./.cdepend
	$(foreach SRC, $(CSOURCES), $(CC) $(CFLAGS) -MM -MT $(SRC:.c=.o) $(SRC) >> ./.cdepend;) 

install: all
	install -d $(INSTALLPATH)
	install -m 754 $(BINDIR)/* $(INSTALLPATH)

deploy: all
	$(info INFO: Deploying...)
	$(DEPLOY1)
	$(DEPLOY2)
	$(DEPLOY3)

clean:
	$(RM) $(OBJECTS) $(OUTPUT) ./.cdepend

-include .cdepend
