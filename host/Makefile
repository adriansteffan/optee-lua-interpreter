CC      ?= $(CROSS_COMPILE)gcc
LD      ?= $(CROSS_COMPILE)ld
AR      ?= $(CROSS_COMPILE)ar
NM      ?= $(CROSS_COMPILE)nm
OBJCOPY ?= $(CROSS_COMPILE)objcopy
OBJDUMP ?= $(CROSS_COMPILE)objdump
READELF ?= $(CROSS_COMPILE)readelf

SRCS = $(wildcard ../lua/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

SRCS += main.c

CFLAGS += -Wall -I../ta/include -I$(TEEC_EXPORT)/include -I./include -I../lua
#Add/link other required libraries here
LDADD += -lteec -L$(TEEC_EXPORT)/lib

BINARY = invoke_lua_interpreter

.PHONY: all
all: $(BINARY)

 
$(BINARY): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDADD)

.PHONY: clean
clean:
	rm -f $(OBJS) $(BINARY)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@