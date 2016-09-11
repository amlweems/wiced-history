#
# Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

#
# Tester for WicedFS
#

TESTER_NAME := wiced_filesystem_test


# Determine what host platform is being used
UNAME = $(shell uname)
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
HOST:=WIN32
CFLAGS += -I/include
EXECUTABLE_EXTENSION:=.exe
else # UNAME
ifeq ($(findstring MINGW,$(UNAME)),MINGW)
CFLAGS += -I/mingw/include
HOST:=WIN32
EXECUTABLE_EXTENSION:=.exe
else # UNAME
HOST:=LINUX
EXECUTABLE_EXTENSION:=
endif
endif


CC:=gcc

# Add some includes and define
CFLAGS += -I/include/
CFLAGS += -D$(HOST)
CFLAGS += -DFX_SINGLE_THREAD
CFLAGS += -DFX_NO_TIMER
CFLAGS += -DUSING_WICEDFS
CFLAGS += -DUSING_FATFS
CFLAGS += -DUSING_FILEX
CFLAGS += -I.
CFLAGS += -I../../../include
CFLAGS += -I../wicedfs/src/mk_wicedfs
CFLAGS += -I../../../WICED/platform/include
CFLAGS += -I../../../WICED
CFLAGS += -I../../../WICED/WWD/include
CFLAGS += -I../../../WICED/security/BESL/include

# Enable Debug, disable optimisation
CFLAGS += -g -O0

# Flags to enable fseeko64/ftello64 on Linux
CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D__USE_LARGEFILE64

# Add pedantic warning flags
#CFLAGS += -Wall -Werror -W -Wstrict-prototypes -Wshadow  -Wwrite-strings -std=gnu99 -pedantic -Wno-format -Wconversion -Wextra -Wdeclaration-after-statement -Wconversion -Waddress -Wlogical-op -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wmissing-field-initializers -Wdouble-promotion -Wswitch-enum -Wswitch-default -Wuninitialized -Wunknown-pragmas -Wfloat-equal  -Wundef

# Flags for 32/64 bit support builds - 32 bit is default, hence no defines
CFLAGS_32 +=
CFLAGS_64 += -DWICEDFS_64BIT_LENGTHS


# Macro to convert a C filename into an Object filename
# $(1) is the C source filename
GET_OBJ_FN = $(patsubst %.c,obj/%.o,$(notdir $(1)))


# Macro to create target to compile C source files
# $(1) is the C source filename
define CREATE_C_TARGET
$(call GET_OBJ_FN,$(1)): $(1)
	mkdir -pv $$(@D)
	$(CC) -c $(CFLAGS) $(CFLAGS_$(2)) -o $$@ $$^

$(eval OBJS += $(call GET_OBJ_FN,$(1)))

endef


# List of source files outside the modules
SOURCES         :=  tester.c \
                    filex_compat.c \
                    filesystem_write.c \
                    strlcpy.c \
                    tester_block_device.c \
                    ../wicedfs/src/mk_wicedfs/wicedfs_create.c \
                    ../../../WICED/internal/wiced_filesystem.c

# Macro to read makfiles of each filesystem implementation
# $(1) is the module makefile
define PROCESS_MODULE
NAME :=
GLOBAL_INCLUDES :=
GLOBAL_DEFINES :=
$(eval CURDIR := $(dir $(1)))
$(eval $(info file $(1) CURDIR $(CURDIR)))
$(eval include $(1))
$(eval SOURCES += $(addprefix $(CURDIR),$($(NAME)_SOURCES)))
$(eval CFLAGS  += $(addprefix -D, $(GLOBAL_DEFINES)  $($(NAME)_DEFINES)))
$(eval CFLAGS  += $(addprefix -I$(CURDIR), $(GLOBAL_INCLUDES) $($(NAME)_INCLUDES)))
$(eval CFLAGS  += $($(NAME)_CFLAGS))
endef


MODULES := ../FATFS/FATFS.mk \
           ../FileX/FileX.mk \
           ../FileX/WICED/WICED.mk \
           ../wicedfs/wicedfs.mk

$(foreach MODULE, $(MODULES), $(eval $(call PROCESS_MODULE, $(MODULE))))


# Create Executable names
EXE_NAME          := $(TESTER_NAME)$(EXECUTABLE_EXTENSION)


# Main target - clean first, then build 32 and 64 bit versions
all: clean $(EXE_NAME)

# Create the targets for compiling C files
$(foreach SRC, $(SOURCES), $(eval $(call CREATE_C_TARGET,$(SRC))))


# Final link targets
$(EXE_NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^


.PHONY: all clean

clean:
	rm -rfv $(EXE_NAME) $(OBJS) obj image.bin

