global-incdirs-y += include
global-incdirs-y += ../lua
global-incdirs-y += ../lua/extensions
srcs-y += $(wildcard ./*.c)
srcs-y += $(wildcard ../lua/*.c)
srcs-y += $(wildcard ../lua/extensions/*.c)

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
