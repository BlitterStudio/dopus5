# Dopus5 modules makefile

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
# Compiles normal version of dopus5 components for plain 68000 OS3 systems.

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


MODULES := about cleanup configopus diskcopy diskinfo filetype fixicons \
     format ftp icon join listerformat misc pathformat play print \
     read show themes xadopus

.PHONY : all
all: $(MODULES)
	@echo " "

.PHONY : about
about:
	@echo " "
	@echo "=============== about_module ==============="
	@$(MAKE) --no-print-directory -C about -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : cleanup
cleanup:
	@echo " "
	@echo "============== cleanup_module =============="
	@$(MAKE) --no-print-directory -C cleanup -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : configopus
configopus:
	@echo " "
	@echo "============= configopus_module ============"
	@$(MAKE) --no-print-directory -C configopus -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : diskcopy
diskcopy:
	@echo " "
	@echo "============= diskcopy_module =============="
	@$(MAKE) --no-print-directory -C diskcopy -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : diskinfo
diskinfo:
	@echo " "
	@echo "============= diskinfo_module =============="
	@$(MAKE) --no-print-directory -C diskinfo -f makefile.$(PLATFORM) arch=$(arch)
		
.PHONY : filetype
filetype:
	@echo " "
	@echo "============= filetype_module =============="
	@$(MAKE) --no-print-directory -C filetype -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : fixicons
fixicons:
	@echo " "
	@echo "============= fixicons_module =============="
	@$(MAKE) --no-print-directory -C fixicons -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : format
format:
	@echo " "
	@echo "============== format_module ==============="
	@$(MAKE) --no-print-directory -C format -f makefile.$(PLATFORM) arch=$(arch)
	
.PHONY : ftp
ftp:
	@echo " "
	@echo "================ ftp_module ================"
	@$(MAKE) --no-print-directory -C ftp -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : icon
icon:
	@echo " "
	@echo "=============== icon_module ================"
	@$(MAKE) --no-print-directory -C icon -f makefile.$(PLATFORM) arch=$(arch)
	
.PHONY : join
join:
	@echo " "
	@echo "=============== join_module ================"
	@$(MAKE) --no-print-directory -C join -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : listerformat
listerformat:
	@echo " "
	@echo "============ listerformat_module ==========="
	@$(MAKE) --no-print-directory -C listerformat -f makefile.$(PLATFORM) arch=$(arch)
	
.PHONY : misc
misc:
	@echo " "
	@echo "================ misc_module ==============="
	@$(MAKE) --no-print-directory -C misc -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : pathformat
pathformat:
	@echo " "
	@echo "============= pathformat_module ============"
	@$(MAKE) --no-print-directory -C pathformat -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : play
play:
	@echo " "
	@echo "================ play_module ==============="
	@$(MAKE) --no-print-directory -C play -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : print
print:
	@echo " "
	@echo "=============== print_module ==============="
	@$(MAKE) --no-print-directory -C print -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : read
read:
	@echo " "
	@echo "=============== read_module ================"
	@$(MAKE) --no-print-directory -C read -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : show
show:
	@echo " "
	@echo "=============== show_module ================"
	@$(MAKE) --no-print-directory -C show -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : themes
themes:
	@echo " "
	@echo "============== themes_module ==============="
	@$(MAKE) --no-print-directory -C themes -f makefile.$(PLATFORM) arch=$(arch)

.PHONY : xadopus
xadopus:
	@echo " "
	@echo "============== xadopus_module =============="
	@$(MAKE) --no-print-directory -C xadopus -f makefile.$(PLATFORM) arch=$(arch)
	
	
###########################################################

.PHONY : clean
clean: clean-about clean-cleanup clean-configopus clean-diskcopy \
       clean-diskinfo clean-filetype clean-fixicons clean-format \
       clean-ftp clean-icon clean-join clean-listerformat clean-misc \
       clean-pathformat clean-play clean-print clean-read clean-show \
       clean-themes clean-xadopus
	@echo " "

.PHONY : clean-about
clean-about:
	@echo " "
	@echo "------------ clean about_module ------------"
	@$(MAKE) --no-print-directory -C about -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-cleanup
clean-cleanup:
	@echo " "
	@echo "----------- clean cleanup_module -----------"
	@$(MAKE) --no-print-directory -C cleanup -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-configopus
clean-configopus:
	@echo " "
	@echo "---------- clean configopus_module ---------"
	@$(MAKE) --no-print-directory -C configopus -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-diskcopy
clean-diskcopy:
	@echo " "
	@echo "----------- clean diskcopy_module ----------"
	@$(MAKE) --no-print-directory -C diskcopy -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-diskinfo
clean-diskinfo:
	@echo " "
	@echo "----------- clean diskinfo_module ----------"
	@$(MAKE) --no-print-directory -C diskinfo -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-filetype
clean-filetype:
	@echo " "
	@echo "----------- clean filetype_module ----------"
	@$(MAKE) --no-print-directory -C filetype -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-fixicons
clean-fixicons:
	@echo " "
	@echo "----------- clean fixicons_module ----------"
	@$(MAKE) --no-print-directory -C fixicons -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-format
clean-format:
	@echo " "
	@echo "------------ clean format_module -----------"
	@$(MAKE) --no-print-directory -C format -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-ftp
clean-ftp:
	@echo " "
	@echo "------------- clean ftp_module -------------"
	@$(MAKE) --no-print-directory -C ftp -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-icon
clean-icon:
	@echo " "
	@echo "------------- clean icon_module ------------"
	@$(MAKE) --no-print-directory -C icon -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-join
clean-join:
	@echo " "
	@echo "------------- clean join_module ------------"
	@$(MAKE) --no-print-directory -C join -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-listerformat
clean-listerformat:
	@echo " "
	@echo "--------- clean listerformat_module --------"
	@$(MAKE) --no-print-directory -C listerformat -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-misc
clean-misc:
	@echo " "
	@echo "------------- clean misc_module ------------"
	@$(MAKE) --no-print-directory -C misc -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-pathformat
clean-pathformat:
	@echo " "
	@echo "---------- clean pathformat_module ---------"
	@$(MAKE) --no-print-directory -C pathformat -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-play
clean-play:
	@echo " "
	@echo "------------- clean play_module ------------"
	@$(MAKE) --no-print-directory -C play -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-print
clean-print:
	@echo " "
	@echo "------------ clean print_module ------------"
	@$(MAKE) --no-print-directory -C print -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-read
clean-read:
	@echo " "
	@echo "------------- clean read_module ------------"
	@$(MAKE) --no-print-directory -C read -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-show
clean-show:
	@echo " "
	@echo "------------- clean show_module ------------"
	@$(MAKE) --no-print-directory -C show -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
.PHONY : clean-themes
clean-themes:
	@echo " "
	@echo "------------ clean themes_module -----------"
	@$(MAKE) --no-print-directory -C themes -s -f makefile.$(PLATFORM) arch=$(arch) clean

.PHONY : clean-xadopus
clean-xadopus:
	@echo " "
	@echo "----------- clean xadopus_module -----------"
	@$(MAKE) --no-print-directory -C xadopus -s -f makefile.$(PLATFORM) arch=$(arch) clean
	
	
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
