// Copyright © 2015 Bo Rydberg

#include <cerrno>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

using std::ios_base;

char const cmakeListName[] = "CMakeLists.txt";
char const headerExt[] = ".h";
char const jbMakefileName[] = "JbMakefile";
char const libNamePostfix[] = "core";
char const libExt[] = ".a";
char const nonHarmfulMakefileName[] = "NonHarmful.mk";
char const objExt[] = ".o";
char const recursiveMakefileName[] = "Recursive.mk";
char const srcExt[] = ".cpp";
ios_base::iostate const osExceptions = ios_base::badbit | ios_base::eofbit | ios_base::failbit;
std::string MODULES;

char const mainMk[] = R"(###########################################################
#
# General make-include
# Johan Bezem, JB Enterprises, © 2008.
#
# Prerequisites:
#  - GNUmake   3.79.1 or higher
#  - Bash      2.05a or higher
#    or
#  - CMD.EXE   Windows XP or higher
#
# Version 005.000
# Saved under "005Differentiate.zip"

# Since 'make' is called recursively, every line in this Makefile
# will get executed again every time, in every directory anew.
# Therefore, we'll create a segment where several variables are 
# determined and exported, since exporting them already makes them 
# available to all child processes, and doesn't burden every run anew.
ifeq ($(MAKELEVEL),0)
  include $(YT_PBASE)/make/level0.mk
endif

# Provide a different version of 'mkdir' depending on the platform
ifeq (1,$(YT_USE_CMDEXE))
  makedirectory=if not exist $(1) md $(subst /,\\,$(1))
else
  makedirectory=test -d $(1) || mkdir -p $(1)
endif

ifeq (,$(findstring $(YT_OBJDIRNAME),$(CURDIR)))
  include $(YT_PBASE)/make/switch.mk
else

  # Compile the include path to refer to all include
  # directories recursively; use $(wildcard to filter-out
  # non-existing directories
  export YT_INCLUDEPATH := $(strip $(wildcard $(YT_SRCVPATH)/include) $(YT_INCLUDEPATH))

  # Now we can add the vpath directive for include files
  # (Add other extensions as necessary). This vpath is only necessary
  # to find/recognize the various *.h files specified as dependencies
  # by the call to maxdepend, generating a set of *.d files that are
  # included at the bottom of this file.
  # See the definition of YT_CPPFLAGS for a description of all
  # used directories.
  vpath %.h $(YT_SRCVPATH) $(YT_INCLUDEPATH)

  # Now set the various option flags; use YT_ as prefix
  # to avoid name clashes with the default
  # variable names
  YT_CPPFLAGS := $(patsubst %,-I%,$(YT_INCLUDEPATH))
  YT_CFLAGS   := $(CFLAGS)

  # Determine all sources, objects, etc.
  # Be sure not to retain any pathnames
  YT_SOURCES:=$(notdir $(wildcard $(YT_SRCVPATH)/*.cpp))

  # Create the objects' filenames from the source filenames
  YT_OBJECTS := $(patsubst %.cpp,%.obj,$(YT_SOURCES))

  # Default target
  .PHONY: all
  all: RECURSE $(YT_OBJECTS)

  # Clean target
  .PHONY: clean
  clean: RECURSE
	$(YT_S)$(YT_RM) *.obj

  # List_dirs will contain a space delimited list of all directories
  # containing a Makefile.
  list_dirs := $(dir $(wildcard $(YT_SRCVPATH)/*/JbMakefile))
  # patsubst retains only the directory names, not the full paths
  list_dirs := $(patsubst $(YT_SRCVPATH)/%,%,$(list_dirs))

  # We define RECURSE as PHONY here, so we don't need to specify it in
  # case we don't need recursion anymore (the else-clause for the following
  # ifneq, when no more subdirectories containing a Makefile can be found).
  # The directories are to be phony, in order to execute the
  # recursion commands for all directories in all cases.
  .PHONY: RECURSE $(list_dirs)

  # If list_dirs is not empty, we need to recurse through one or more 
  # subdirectories
  ifneq ($(strip $(list_dirs)),)

    # The RECURSE target can be used as a dependency for all targets
    # that need to be made recursively. Put it as the first dependency
    # for a depth-first usage.
    RECURSE: $(list_dirs)

    # To 'create' the subdirectories, we execute the commands listed,
    # one of which is reexecuting 'make' in the target subdirectory '$@'
    $(list_dirs):
	+@$(call makedirectory,$(YT_OBJBASE)/$@)
	+@echo Make[$(MAKELEVEL)]: $(patsubst %/,%,$@)
	+@$(MAKE) -C $(YT_OBJBASE)/$(patsubst %/,%,$@) YT_SRCVPATH=$(YT_SRCVPATH)/$(patsubst %/,%,$@) YT_OBJBASE=$(YT_OBJBASE)/$(patsubst %/,%,$@) -f $(YT_SRCVPATH)/$(patsubst %/,%,$@)/JbMakefile --no-print-directory $(MAKECMDGOALS)
    # Watch the empty line before the endif, otherwise it would be an (illegal)
    # part of the commands to $(list_dirs): !!

  endif

  # Rules are mostly self-defined, since different compilers
  # have different customs. So here we clear the list of implicit
  # pattern rules and known suffixes.
  .SUFFIXES:
  # A .SUFFIXES: rule with extensions apparently only means anything 
  # to old style suffix rules, but we define it anyway
  # For now we only recognize C++-files.
  .SUFFIXES: .cpp

  vpath %.cpp $(YT_SRCVPATH)

  %.obj: %.cpp
	@echo .cpp   to $(YT_OBJEXT): $(notdir $<)
	$(YT_S)$(YT_CC) -c $(subst $(YT_PBASE),$(YT_PBASE_WDL),$(YT_CPPFLAGS)) $(YT_CFLAGS) $(subst $(YT_PBASE),$(YT_PBASE_WDL),$<) -o $@

endif
)";

char const level0Mk[] = R"(###########################################################
#
# General make-include
# Johan Bezem, JB Enterprises, © 2008.
#
# Prerequisites:
#  - GNUmake   3.79.1 or higher
#  - Bash      2.05a or higher
#    or
#  - CMD.EXE   Windows XP or higher
#
# Version 005.000
# Saved under "005Differentiate.zip"

# A few helpful variables go here
null:=
space:=$(strip $(null)) $(strip $(null))

# First, determine the name of the objects directory
# This name should be in such a way unique, that a pathname
# containing that string can only refer to an objects resp.
# intermediates' directory. This is of vital importance for
# the structure of the makefile.
export YT_OBJDIRNAME:=objects

# Record the directory from where we started, for later reference.
export YT_STARTDIR := $(CURDIR)

# Collect the components for the intermediates' directory
# in YT_DIFFDIR. Start out with 'Im' to avoid starting with a dash
# Use a simply expanded variable to enble self-references
YT_DIFFDIR := Im

# We must determine the operating system used.
# Initialize first:
YT_USE_WINDOWS:=0
YT_USE_LINUX  :=0
YT_USE_CMDEXE :=0
YT_USE_BASH   :=0

# If we are using Windows NT or later, the environment variable 
# OS will be set to 'Windows_NT'
ifneq (Windows_NT,$(OS))
# We'll assume Linux for the time being
  YT_USE_LINUX:=1
  YT_NAME_PLATFORM:=LINUX
  YT_USE_BASH:=1
  YT_NAME_SHELL:=BASH
else
  YT_USE_WINDOWS:=1
  YT_NAME_PLATFORM:=WINDOWS
  # We need to differentiate between Cygwin using bash, and 
  # Windows using CMD.EXE. For that, we look into the PATH variable, 
  # and search for semicolons; only in CMD.EXE, semicolons are 
  # allowed as path separators.
  ifeq (;,$(findstring ;,$(PATH)))
    YT_USE_CMDEXE:=1
    YT_NAME_SHELL:=CMDEXE
  else
    YT_USE_BASH:=1
    YT_NAME_SHELL:=BASH
  endif
endif

# And now export our findings for all recursions
export YT_USE_WINDOWS YT_USE_LINUX YT_USE_CMDEXE YT_USE_BASH
export YT_NAME_PLATFORM YT_NAME_SHELL

# Here we start with the host platform to define the architecure
include $(YT_PBASE)/make/platform_$(YT_NAME_PLATFORM).mk

# And then the shell's own specific definitions
include $(YT_PBASE)/make/shell_$(YT_NAME_SHELL).mk

# Now define the compiler (toolchain) platform
# See if the configuration requires a specific implementation
ifeq ($(origin YT_TC_SELECT),undefined)
  # If not, check for a default
  # CMD.EXE:
  ifeq (1,$(YT_USE_CMDEXE))
    # Take the newest supported VS
    YT_TC_SELECT := VS
  endif

  # Bash:
  ifeq (1,$(YT_USE_BASH))
    # Take the newest supported GCC
    YT_TC_SELECT := GCC
  endif

  # Test for a supported TC
  ifeq ($(origin YT_TC_SELECT),undefined)
    $(error No supported shell for default toolchain detection)
  endif
endif

# We now look for the concrete implementation
# We may find more than one, so sort the list and take the last one
# (with the highest version number)
# Using GNU make 3.81, the following line will work and be faster:
YT_TC_MAKE_INCLUDE := $(lastword $(sort $(wildcard $(YT_PBASE)/make/tc_$(YT_TC_SELECT)*.mk)))
# For pre-3.81, the following equivalent is necessary
#YT_TC_MAKE_INCLUDE := $(word $(words $(sort $(wildcard $(YT_PBASE)/make/tc_$(YT_TC_SELECT)*.mk))), $(sort $(wildcard $(YT_PBASE)/make/tc_$(YT_TC_SELECT)*.mk)))

ifeq (,$(strip $(YT_TC_MAKE_INCLUDE)))
  $(error Toolchain make include not found.)
endif

# And include the selected file
include $(YT_TC_MAKE_INCLUDE)

# All components for the intermediates' directory have been collected,
# so no make sure all make instances will inherit this value
export YT_DIFFDIR
)";

char const platformLinuxMk[] =R"(###########################################################
#
# General make-include
# Johan Bezem, JB Enterprises, © 2008.
#
# Prerequisites:
#  - GNUmake   3.79.1 or higher
#  - Bash      2.05a or higher
#    or
#  - CMD.EXE   Windows XP or higher
#
# Version 005.000
# Saved under "005Differentiate.zip"

# Augment the intermediates' directory
YT_DIFFDIR := $(YT_DIFFDIR)-pfLINUX
)";

char const platformWindowsMk[] = R"(###########################################################
#
# General make-include
# Johan Bezem, JB Enterprises, © 2008.
#
# Prerequisites:
#  - GNUmake   3.79.1 or higher
#  - Bash      2.05a or higher
#    or
#  - CMD.EXE   Windows XP or higher
#
# Version 005.000
# Saved under "005Differentiate.zip"

# Augment the intermediates' directory
YT_DIFFDIR := $(YT_DIFFDIR)-pfWIN
)";

char const shellBashMk[] = R"(###########################################################
#
# General make-include
# Johan Bezem, JB Enterprises, © 2008.
#
# Prerequisites:
#  - GNUmake   3.79.1 or higher
#  - Bash      2.05a or higher
#    or
#  - CMD.EXE   Windows XP or higher
#
# Version 005.000
# Saved under "005Differentiate.zip"

export YT_RM := rm -f

# When using Cygwin on Windows, we have to take care of 
# /cygdrive pathnames when they are to be used in windows tools
ifeq (1,$(YT_USE_WINDOWS))
  export YT_PBASE_WDL := $(word 2,$(subst /,$(space),$(YT_PBASE))):/$(subst $(space),/,$(wordlist 3,99,$(subst /,$(space),$(YT_PBASE))))
else
  # If using bash on Linux, we have no problem.
  export YT_PBASE_WDL := $(YT_PBASE)
endif
)";

char const shellCmdexeMk[] = R"(###########################################################
#
# General make-include
# Johan Bezem, JB Enterprises, © 2008.
#
# Prerequisites:
#  - GNUmake   3.79.1 or higher
#  - Bash      2.05a or higher
#    or
#  - CMD.EXE   Windows XP or higher
#
# Version 005.000
# Saved under "005Differentiate.zip"

export YT_RM := del /q 2>NUL

# We need to define all mandatory variables

# For using DOS drive letters (With Drive Letters)
# in bash-shells, we need:
export YT_PBASE_WDL := $(YT_PBASE)
)";

char const switchMk[] = R"(###########################################################
#
# General make-include
# Johan Bezem, JB Enterprises, © 2008.
#
# Prerequisites:
#  - GNUmake   3.79.1 or higher
#  - Bash      2.05a or higher
#    or
#  - CMD.EXE   Windows XP or higher
#
# Version 005.000
# Saved under "005Differentiate.zip"

YT_OBJDIR := $(YT_PBASE)/$(YT_OBJDIRNAME)/$(YT_DIFFDIR)

# Disable all built-in rules; we don't need them on this run
.SUFFIXES:

# The object directory is the only target here; since it's phony,
# when the commands have been executed, it is considered built.
# This calls make recursively, but now from the objects directory.
.PHONY: $(YT_OBJDIR)
$(YT_OBJDIR):
	+@$(call makedirectory,$@)
	+@echo Make : $@
	+@$(MAKE) -C $@ -f $(CURDIR)/JbMakefile YT_SRCVPATH=$(CURDIR) YT_OBJBASE=$(YT_OBJDIR) --no-print-directory $(MAKECMDGOALS)

# Since we'll provide just one rule, but a 'match-anything' rule,
# when make tries to remake the makefiles involved, the rule will
# also apply, and make would recursively call itself from the
# same directory... Endless recursion results in chaos.
JbMakefile : ; @:
%.mk :: ;

# No matter what goals are given, this dependency will make sure,
# make is recursively called.
% :: $(YT_OBJDIR) ; @:
)";

char const tc_Gcc344Mk[] = R"(###########################################################
#
# General make-include
# Johan Bezem, JB Enterprises, © 2008.
#
# Prerequisites:
#  - GNUmake   3.79.1 or higher
#  - Bash      2.05a or higher
#    or
#  - CMD.EXE   Windows XP or higher
#
# Version 005.000
# Saved under "005Differentiate.zip"

# If environment variable CC is not defined, or defaulted by make,
# use the GNU compiler frontend gcc.
# Otherwise use the indicated compiler.
ifeq (,$(strip $(filter-out undefined default,$(origin CC))))
  export YT_CC := gcc
else
  export YT_CC := $(CC)
endif

# If shell_BASH.mk has set YT_PBASE_WDL for translating
# Cygwin paths into DOS, we can take it back here
# GCC will work with native pathnames just as well.
export YT_PBASE_WDL := $(YT_PBASE)

# Augment the intermediates' directory
YT_DIFFDIR := $(YT_DIFFDIR)-tcGCC
)";

char const tcVS9Mk[] = R"(###########################################################
#
# General make-include
# Johan Bezem, JB Enterprises, © 2008.
#
# Prerequisites:
#  - GNUmake   3.79.1 or higher
#  - Bash      2.05a or higher
#    or
#  - CMD.EXE   Windows XP or higher
#
# Version 005.000
# Saved under "005Differentiate.zip"

# If environment variable CC is not defined, or defaulted by make,
# use the Microsoft compiler frontend CL.EXE
# Otherwise use the indicated compiler.
ifeq (,$(strip $(filter-out undefined default,$(origin CC))))
  export YT_CC := CL.EXE
else
  export YT_CC := $(CC)
endif

# Augment the intermediates' directory
YT_DIFFDIR := $(YT_DIFFDIR)-tcVS
)";

void mkheader(std::string const & dirbase, std::string const & namebase)
{
	std::string const fname("f" + namebase);
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + fname + headerExt);
	std::string const incguard(fname + "_H_INCLUDED_");
	os << "#ifndef " << incguard << "\n"
	      "#define " << incguard << "\n";
	std::string const className("K" + namebase);
	os << "class " << className << "\n"
	      "{\n"
	      "public:\n"
	      "\tvoid Work_" << namebase << "();\n"
	      "private:\n"
	      "\tint m_" << namebase << ";\n"
	      "};\n"
	      "#endif // " << incguard << "\n";
}

void mksource(std::string const & dirbase, std::string const & namebase)
{
	std::string const fname("f" + namebase);
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + fname + srcExt);
	os << "#include \"" << fname << headerExt << "\"\n"
	      "\n";
	std::string const className("K" + namebase);
	os << "void " << className << "::Work_" << namebase << "()\n"
	      "{\n"
	      "\t++m_" << namebase << ";\n"
	      "}\n";
}

void mkRecuriveMakefile(std::string const & dirbase, std::string const & namebase)
{
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + recursiveMakefileName);
	std::string const filename("f" + namebase);
	os << ".PHONY: all\n"
	      "all : lib" << namebase << libNamePostfix << libExt << "\n"
	      "lib" << namebase << libNamePostfix << libExt << " : " << filename << objExt << "\n"
	      "\tar cr $@ $<\n"
	   << filename << objExt << " : " << filename << srcExt << " " << filename << headerExt << "\n";
	MODULES += " " + dirbase.substr(2);
}

void mkCMakeLists(std::string const & dirbase, std::string const & namebase){
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + cmakeListName);
	os << "project(Prg" << namebase << ")\n"
	      "add_library(" << namebase << libNamePostfix << " f" << namebase << srcExt << ")\n";
}

void mkNonHarmfulMakefile(std::string const & dirbase, std::string const & namebase)
{
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + nonHarmfulMakefileName);
	std::string const filename(dirbase.substr(2) + "/f" + namebase);
	os << "LIBS += " << dirbase.substr(2) << "/lib" << namebase << libNamePostfix << libExt << "\n"
	   << dirbase.substr(2) << "/lib" << namebase << libNamePostfix << libExt << " : " << filename << objExt << "\n"
	      "\tar cr $@ $<\n"
	   << filename << objExt << " : " << filename << srcExt << " " << filename << headerExt << "\n";
}

void mkfiles(std::string const & dirbase, std::string const & namebase)
{
	mkheader(dirbase, namebase);
	mksource(dirbase, namebase);
	mkRecuriveMakefile(dirbase, namebase);
	mkNonHarmfulMakefile(dirbase, namebase);
	mkCMakeLists(dirbase, namebase);
}

void mkJbLocalMakefile(std::string const & dirbase)
{
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(dirbase + "/" + jbMakefileName);
	os << "include $(YT_PBASE)/make/main.mk\n";
}

void mkDir(std::string const & dirLocation)
{
	if (mkdir(dirLocation.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
		auto const e = errno;
		std::ostringstream oss;
		oss << "`mkdir(\"" << dirLocation
				<< "\", S_IRWXU | S_IRWXG | S_IRWXO)' failed with " << e;
		throw std::runtime_error(oss.str());
	}
}

void mkDirRange(int const depth, std::string const & dirbase, std::string const & namebase,
		char const a, char const z)
{
	mkJbLocalMakefile(dirbase);
	if (depth <= 0) {
		return mkfiles(dirbase, namebase);
	}
	std::string d(dirbase + "/d" + namebase);
	auto const len = d.length();
	for (auto i = a; i <= z; ++i) {
		d.resize(len);
		d += i;
		mkDir(d);
		mkDirRange(depth - 1, d, namebase + i, a, z);
	}
}

void mkMainRecursiveMakefile()
{
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(&recursiveMakefileName[0]);
	os << "MODULES =" << MODULES
	   << R"Frogzy(

.PHONY : all
all :
	for dir in $(MODULES); do \
		cd $$dir && ${MAKE} -f )Frogzy" << recursiveMakefileName << R"Xyzzy( all && cd -; \
	done
)Xyzzy";
}

void mkMainCMakeListsFile()
{
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(&cmakeListName[0]);
	os << "cmake_minimum_required(VERSION 2.8)\n"
	      "project(BigThing)\n";
	std::istringstream iss(MODULES);
	std::string mod;
	while (iss >> mod) {
		os << "add_subdirectory(" << mod << ")\n";
	}
}

void mkMainNonHarmfulMakefile()
{
	std::ofstream os;
	os.exceptions(osExceptions);
	os.open(&nonHarmfulMakefileName[0]);
	os << "MODULES :=" << MODULES
			<< R"Frogzy(

.PHONY : all
all :

# include the description for each module
include $(patsubst %,%/)Frogzy" << nonHarmfulMakefileName << R"Xyzzy(,$(MODULES))

all : $(LIBS)
)Xyzzy";
}

int getDepth(int const argc, char *argv[])
{
	if (1 < argc) {
		std::istringstream iss(argv[1]);
		int dept;
		if (iss >> dept) {
			return dept;
		}
	}
	std::cerr << "Using default depth of 2\n";
	return 2;
}

void mkFileWithContent(std::string const & fileName, std::string const & fileContent){
	std::ofstream ofs;
	ofs.exceptions(osExceptions);
	ofs.open(fileName);
	ofs << fileContent;
}

void mkMainJbMakesystem(std::string basedir)
{
	basedir += "/make";
	mkDir(basedir);
	mkFileWithContent(basedir + "/tc_VS9.mk", tcVS9Mk);
	mkFileWithContent(basedir + "/tc_GCC344.mk", tc_Gcc344Mk);
	mkFileWithContent(basedir + "/switch.mk", switchMk);
	mkFileWithContent(basedir + "/shell_CMDEXE.mk", shellCmdexeMk);
	mkFileWithContent(basedir + "/shell_BASH.mk", shellBashMk);
	mkFileWithContent(basedir + "/platform_WINDOWS.mk", platformWindowsMk);
	mkFileWithContent(basedir + "/platform_LINUX.mk", platformLinuxMk);
	mkFileWithContent(basedir + "/main.mk", mainMk);
	mkFileWithContent(basedir + "/level0.mk", level0Mk);
}

} // namespace

int main(int argc, char *argv[])
{
	auto const depth = getDepth(argc, argv);
	mkDirRange(depth, ".", "", 'a', 'z');
	mkMainRecursiveMakefile();
	mkMainNonHarmfulMakefile();
	mkMainCMakeListsFile();
	mkMainJbMakesystem(".");
	return EXIT_SUCCESS;
}
