# Dopus5 commands makefile

###########################################################
# Enter debug=no on "Make" command line to disable debug output.
export debug
###########################################################
# Export arch to select the AROS architecture
export arch
# Export cpu to select OS3 CPU variant (currently only cpu=68000 is honoured;
# the default OS3 build targets 68020-60 with 68030 tuning).
export cpu
###########################################################
# Example Commands:

# make os4
# Compiles debug version of dopus5 components for OS4.

# make MOS debug=no
# Compiles normal (non-debug) version of dopus5 components for MorphOS.

# make OS3 release
# Compiles debug version of dopus5 components for OS3 & creates
# release archive.

# make os3-68000 debug=no
# Compiles normal version of dopus5 commands for plain 68000 OS3 systems.

# make x86_64-aros release debug=no
# Compiles normal version of dopus5 components for x86_64-AROS & creates
# release archive.
###########################################################
# Read platform arguments from the "Make" command line:
# Acceptable platform arguments are os3, os3-68000, os4, mos,
# i386-aros, x86_64-aros or aarch64-aros.
#
# Order matters: check os3-68000 BEFORE os3 because `os3` is a
# substring of `os3-68000` and findstring would otherwise match both.
PLATFORM:= none
NAME:=
ifeq ($(findstring os3-68000, $(MAKECMDGOALS)),os3-68000)
	PLATFORM:= os3
	cpu:= 68000
	NAME:= os3-68000 $(NAME)
else ifeq ($(findstring os3, $(MAKECMDGOALS)),os3)
	PLATFORM:= os3
	NAME:= $(PLATFORM) $(NAME)
endif
ifeq ($(findstring os4, $(MAKECMDGOALS)),os4)
	PLATFORM:= os4
	NAME:= $(PLATFORM) $(NAME)
endif
ifeq ($(findstring mos, $(MAKECMDGOALS)),mos)
	PLATFORM:= mos
	NAME:= $(PLATFORM) $(NAME)
endif
ifeq ($(findstring i386-aros, $(MAKECMDGOALS)),i386-aros)
	PLATFORM:= aros
	arch:= i386
	NAME:= $(arch)-$(PLATFORM) $(NAME)
endif
ifeq ($(findstring x86_64-aros, $(MAKECMDGOALS)),x86_64-aros)
	PLATFORM:= aros
	arch:= x86_64
	NAME:= $(arch)-$(PLATFORM) $(NAME)
endif
ifeq ($(findstring aarch64-aros, $(MAKECMDGOALS)),aarch64-aros)
	PLATFORM:= aros
	arch:= aarch64
	NAME:= $(arch)-$(PLATFORM) $(NAME)
endif
NAME := $(strip $(NAME))

# Generate error if no platform entered on "Make" command line.
ifeq ($(PLATFORM), none)
$(error Error: Enter a platform (os3, os3-68000, os4, mos, i386-aros, x86_64-aros or aarch64-aros).)
endif

#Generate error if multiple platforms entered on "Make" command line.
ifneq ($(words $(NAME)), 1)
$(error Error: Enter only 1 platform (os3, os3-68000, os4, mos, i386-aros, x86_64-aros or aarch64-aros).)
endif

# Compile all if only a platform is entered on "Make" command line.
ifeq ($(strip $(filter-out $(NAME), $(MAKECMDGOALS))),)
$(NAME) : all
endif
###########################################################


.PHONY : all
all: dopusrt loaddb viewfont
	@echo " "

.PHONY : dopusrt
dopusrt:
	@echo " "
	@echo "================== dopusrt ================="
	@$(MAKE) --no-print-directory -C dopusrt -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : loaddb
loaddb:
	@echo " "
	@echo "================== loaddb =================="
	@$(MAKE) --no-print-directory -C loadwb -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : viewfont
viewfont:
	@echo " "
	@echo "================= viewfont ================="
	@$(MAKE) --no-print-directory -C ViewFont -f makefile.$(PLATFORM) arch=$(arch)


###########################################################

.PHONY : clean
clean: clean-dopusrt clean-loaddb clean-viewfont
	@echo " "

.PHONY : clean-dopusrt
clean-dopusrt:
	@echo " "
	@echo "--------------- clean dopusrt --------------"
	@$(MAKE) --no-print-directory -C dopusrt -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-loaddb
clean-loaddb:
	@echo " "
	@echo "--------------- clean loaddb ---------------"
	@$(MAKE) --no-print-directory -C loadwb -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-viewfont
clean-viewfont:
	@echo " "
	@echo "-------------- clean viewfont --------------"
	@$(MAKE) --no-print-directory -C ViewFont -f makefile.$(PLATFORM) arch=$(arch) clean


###########################################################

# Dummy targets to quietly dispose of platform
# commands when entered as a goal for "make"
# on the command line (like: make os3).

.PHONY : os3 os3-68000 os4 mos aros i386-aros x86_64-aros aarch64-aros
i386-aros: mos
x86_64-aros: mos
aarch64-aros: mos
os3: mos
os3-68000: mos
os4: mos
aros: mos
mos:
	-@sh -c "echo -n"

