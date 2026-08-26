# Directory Opus 5
# Copyright 1993-2012 Jonathan Potter & GP Software
# Copyright 2012-2013 DOPUS5 Open Source Team
# Copyright 2023-2026 Dimitris Panokostas

# Main DOpus5 makefile

###########################################################
# Enter debug=no on "Make" command line to disable debug output.
export debug
###########################################################
# Export arch to select the AROS architecture
export arch
# Export cpu to select OS3 CPU variant (currently only cpu=68000 is honoured;
# the default OS3 build targets 68020-60 with 68030 tuning).
export cpu
PYTHON ?= python3
###########################################################
# Example Commands:

# make os4
# Compiles debug version of dopus5 components for OS4.

# make MOS debug=no
# Compiles normal (non-debug) version of dopus5 components for MorphOS.

# make OS3 release
# Compiles debug version of dopus5 components for OS3 & creates
# release archive.

# make os3-68000 release debug=no
# Compiles normal version of dopus5 for plain 68000 OS3 systems &
# creates release archive (DOpus5_<rev>_os3-68000.lha).

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
# Sets archive type & version.
# REVISION is derived from PROG_REVISION in Include/dopus/version.h so the
# release archive filename (DOpus5_<REV>_<platform>.lha) always matches the
# binary's reported version. Bumping the version.h value is the only step.
REVISION := $(shell awk '/^\#define[ \t]+PROG_REVISION/ {print $$3; exit}' Include/dopus/version.h)
ifeq ($(debug), no)
TYPE :=
VERSION := _$(REVISION)_
else
TYPE := _debug
VERSION := _$(REVISION)_dev_
endif
###########################################################
# Sets compile system.
HOST := $(shell uname)
ifeq ($(HOST), AmigaOS)
	SYSTEM := amiga
endif
ifeq ($(HOST), MorphOS)
	SYSTEM := amiga
endif
ifeq ($(HOST), AROS)
	SYSTEM := amiga
endif
ifeq ($(HOST), Linux)
	SYSTEM := linux
endif
###########################################################
# Sets destination directory and lha command for releases
DEST := ../releases/
ifeq ($(SYSTEM), amiga)
ARCDEST := /releases/
LHA := lha a -a -r -q
else ifeq ($(SYSTEM), linux)
ARCDEST := $(DEST)
LHA := lha a -q
else
ARCDEST := $(DEST)
LHA := lha ao5q
endif
###########################################################
# Sets base archive to use for releases
BASEARC := basedata.lha
RELEASE_DIR := DOpus5.$(NAME)
RELEASE_ROOT := $(RELEASE_DIR)/DOpus5
RELEASE_ARCDEST := $(abspath $(ARCDEST))
###########################################################
# Commands
REMOVE := rm -rf
COPY := cp -p
MAKEDIR := mkdir -p
DATE := date
###########################################################
ifeq ($(PLATFORM), os4)
SHELL := sh
endif

.PHONY : all
all : program library modules commands

.PHONY : program
program:
	@echo " "
	@echo "<<<<<<<<<<<<<<<<<< program >>>>>>>>>>>>>>>>>>"
	@$(MAKE) -C Program -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : library
library:
	@echo " "
	@echo "<<<<<<<<<<<<<<<<<< library >>>>>>>>>>>>>>>>>>"
	@$(MAKE) -C Library -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : modules
modules:
	@echo " "
	@echo "<<<<<<<<<<<<<<<<<< modules >>>>>>>>>>>>>>>>>>"
	@$(MAKE) -C Modules $(NAME) arch=$(arch)

.PHONY : commands
commands:
	@echo " "
	@echo "<<<<<<<<<<<<<<<<<< commands >>>>>>>>>>>>>>>>>"
	@$(MAKE) -C Misc/C $(NAME) arch=$(arch)

###########################################################

.PHONY : clean
clean : clean-program clean-library clean-modules clean-commands

.PHONY : cleanall
cleanall : clean
	@echo " "
	@echo "++++++++++++++ clean binaries ++++++++++++++"
	@$(REMOVE) bin.$(NAME)

.PHONY : clean-program
clean-program:
	@echo " "
	@echo "++++++++++++++ clean program +++++++++++++++"
	@$(MAKE) -C Program -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-library
clean-library:
	@echo " "
	@echo "++++++++++++++ clean library +++++++++++++++"
	@$(MAKE) -C Library -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-modules
clean-modules:
	@echo " "
	@echo "++++++++++++++ clean modules +++++++++++++++"
	@$(MAKE) -C Modules -s $(NAME) clean arch=$(arch)

.PHONY : clean-commands
clean-commands:
	@echo " "
	@echo "++++++++++++++ clean commands ++++++++++++++"
	@$(MAKE) -C Misc/C -s $(NAME) clean arch=$(arch)

###########################################################

.PHONY : release
release: all release-package

.PHONY : release-package
release-package:
	@if [ ! -f bin.$(NAME)/dopus5.library ]; then \
		echo ""; \
		echo "*** Error: bin.$(NAME)/dopus5.library is missing."; \
		echo "*** Run 'make $(NAME) all' first, then 'make $(NAME) release-package'."; \
		echo "*** If only a different bin.<arch>-aros/ got populated, the matching"; \
		echo "*** cross-compiler is probably not installed and the build silently"; \
		echo "*** fell back to the host toolchain."; \
		exit 1; \
	fi
	@echo " "
	@echo ">>>>>Creating release archive: DOpus5$(VERSION)$(NAME)$(TYPE).lha"
	@$(REMOVE) $(RELEASE_DIR)
	@$(MAKEDIR) $(RELEASE_ROOT)/Libs
	@$(MAKEDIR) $(RELEASE_ROOT)/Modules
	@$(MAKEDIR) $(RELEASE_ROOT)/C
	@$(MAKEDIR) $(RELEASE_ROOT)/Help
	@$(MAKEDIR) $(DEST)
ifeq ($(PLATFORM), os4)
	@$(MAKEDIR) $(RELEASE_ROOT)/WBStartup
	@$(COPY) ../archive/DoWBStartup $(RELEASE_ROOT)/WBStartup
	@$(COPY) ../archive/DoWBStartup.info  $(RELEASE_ROOT)/WBStartup
endif
	@$(COPY) bin.$(NAME)/dopus5.library $(RELEASE_ROOT)/Libs
	@$(COPY) bin.$(NAME)/*.module $(RELEASE_ROOT)/Modules
	@$(COPY) bin.$(NAME)/DirectoryOpus $(RELEASE_ROOT)
	@$(COPY) bin.$(NAME)/DOpusRT5 $(RELEASE_ROOT)/C
	@$(COPY) bin.$(NAME)/LoadDB $(RELEASE_ROOT)/C
	@$(COPY) bin.$(NAME)/viewfont $(RELEASE_ROOT)/C
	@$(COPY) ../documents/*.guide $(RELEASE_ROOT)/Help
	@$(COPY) ../THIRD_PARTY_NOTICES.md $(RELEASE_ROOT)/Help
ifeq ($(log), yes)
	@$(COPY) ../ChangeLog $(RELEASE_ROOT)
endif
ifeq ($(SYSTEM), linux)
# Ensure Amiga executables carry the Unix +x bit so lha records it
# (Amiga LHA maps it back to the Amiga `e` protection flag on extract).
	@chmod +x $(RELEASE_ROOT)/DirectoryOpus $(RELEASE_ROOT)/Libs/dopus5.library $(RELEASE_ROOT)/Modules/*.module \
		$(RELEASE_ROOT)/C/DOpusRT5 $(RELEASE_ROOT)/C/LoadDB $(RELEASE_ROOT)/C/viewfont
endif
	@$(COPY) ../archive/$(BASEARC) $(DEST)DOpus5$(VERSION)$(NAME)$(TYPE).lha
	@cd $(RELEASE_DIR) && $(LHA) $(RELEASE_ARCDEST)/DOpus5$(VERSION)$(NAME)$(TYPE).lha DOpus5
	@$(REMOVE) $(RELEASE_DIR)
	@echo ">>>>>Release date:" $(shell $(DATE) "+%m/%d/%Y")

###########################################################

.PHONY : clean-release
clean-release: cleanall
	@echo " "
	@echo "++++++++++++++ clean release +++++++++++++++"
	@$(REMOVE) $(DEST)
	@$(REMOVE) bin.*

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

.PHONY: aros-void-headers
aros-void-headers:
	$(PYTHON) Include/defines/patch_aros_voidcalls.py

.PHONY: Include/inline/dopus5.h

Include/inline/dopus5.h: Include/fd/dopus_lib.fd Include/clib/dopus5_protos.h
	fd2sfd Include/fd/dopus_lib.fd Include/clib/dopus5_protos.h Include/fd/dopus_lib.sfd
	sfdc --output=Include/inline/dopus5.h --target=m68k-amigaos --mode=macros Include/fd/dopus_lib.sfd

.PHONY: Include/inline/configopus.h

Include/inline/configopus.h: Include/fd/configopus.fd Include/clib/configopus_protos.h
		fd2sfd Include/fd/configopus.fd Include/clib/configopus_protos.h Include/fd/configopus.sfd
		sfdc --output=Include/inline/configopus.h --target=m68k-amigaos --mode=macros Include/fd/configopus.sfd

headers: Include/inline/dopus5.h Include/inline/configopus.h aros-void-headers

