#-----------------------------------------------------------------------------
#  Makefile v4
#
#   make                    // makes Debug version (default)
#   make VER=RELEASE        // makes Release version
#   make clean              // removes object files
#
#   VER = {RELEASE, DEBUG}
#-----------------------------------------------------------------------------


#-- MUST Edit:
APPNAME  = piClock
SRCDIR   = ./*.c 
INCLUDES = -I./include
REVDIR   = .


#USBFLAGS=   `libusb-config --cflags`
#USBLIBS=    `libusb-config --libs`

#-- default settings:
CFLAGS   = -Wall -Wextra
#CFLAGS   = -Wall -Wextra -Werror -DPI_HW $(USBFLAGS)
LDFLAGS  = -lallegro -lallegro_main -lallegro_font -lallegro_primitives -lallegro_color
LDFLAGS += -lallegro_ttf -lallegro_memfile -lallegro_image -lallegro_ttf -lpthread -lm
#LDFLAGS  = -lpthread -lwiringPi -lwiringPiDev $(USBLIBS)
DBGFLAGS = -g -O0
RELFLAGS = -O2

CC       = gcc
BINDIR   = .
OBJDIR   = obj


#============= DO NOT EDIT BELOW THIS LINE ===================================
CFLAGS += $(INCLUDES) -c 

VER = DEBUG
ifeq "$(VER)" "RELEASE"
CFLAGS += $(RELFLAGS)
RELCLN = clean 
else
ifeq "$(VER)" "DEBUG"
CFLAGS += $(DBGFLAGS)
RELCLN =
endif
endif

SRCS    := $(shell find $(SRCDIR) -name '*.c')
SRCDIRS := $(shell find $(SRCDIR) -name '*.c' -exec dirname {} \; | uniq)
OBJS    := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean cleanall

all: $(RELCLN) $(BINDIR)/$(APPNAME) 

$(BINDIR)/$(APPNAME): build_objdirs $(OBJS)
	@mkdir -p `dirname $@`
	@echo "  Linking $@..."
	@$(CC) $(OBJS) $(LDFLAGS) -o $@
ifeq "$(VER)" "RELEASE"
	@strip -s $(APPNAME)
endif
	@echo ""
	@echo "==== Done building" $(APPNAME) "====="
	@echo ""

$(OBJDIR)/%.o: %.c
	@$(call make-depend,$<,$@,$(subst .o,.d,$@))
	@echo "  Compiling $<..."
	@$(CC) $(CFLAGS) $< -o $@

build_objdirs:
ifeq "$(VER)" "RELEASE"
	@echo "  << Building RELEASE Version >>"
else
	@echo " "
endif
#	@echo "  Updating LMI_revision.h"
#	@./extract_svn_rev
#	@mv LMI_revision.h $(REVDIR)
	@$(call make-objdirs)

clean:
	@echo ""
	@echo "  Cleaning..."
	@$(RM) -r $(OBJDIR)
	@$(RM) $(APPNAME)
ifneq "$(VER)" "RELEASE"
	@echo "==== Done Cleaning" $(APPNAME) "====="
	@echo ""
endif

#--- create \obj dirs 
# usage: $(call make-objdirs)
define make-objdirs
   for dir in $(SRCDIRS); \
   do \
	mkdir -p $(OBJDIR)/$$dir; \
   done
endef

#--- make dependencies
# usage: $(call make-depend,source-file,object-file,depend-file)
define make-depend
  $(CC) -MM       \
        -MF $3    \
        -MP       \
        -MT $2    \
        $(CFLAGS) \
        $1
endef


-include $(OBJS:.o=.d)

