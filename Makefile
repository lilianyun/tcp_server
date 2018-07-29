##Makefile
PROG := tcp_server

CXX ?= g++

#####################cfg#######################
DEBUG    := debug
RELEASE  := release
ifneq ($(cfg),)
	ifeq ($(findstring $(cfg), $(DEBUG) $(RELEASE)),)
		$(error Use 'make cfg=debug' or 'make cfg=release'.)
	endif
endif

VPATH   := ./src
VPATH   += ./src/session

INCLUDE_LIST := -I./include
INCLUDE_LIST += -I./include/tbb
INCLUDE_LIST += -I./include/utils

SOURCES      := $(filter %.cpp, $(foreach dir,$(VPATH),$(wildcard $(dir)/*)))

OUTPUT    := ./release
ifeq ($(cfg), $(DEBUG))
OUTPUT    := ./debug
endif

OBJECTS := $(addprefix $(OUTPUT)/,$(patsubst %.cpp,%.o,$(notdir $(SOURCES))))
DEPS    := $(addprefix $(OUTPUT)/,$(patsubst %.cpp,%.d,$(notdir $(SOURCES))))

LDFLAGS     := -L./lib

LDFLAGS     += -ltbb
LDFLAGS     += -ltbbmalloc
LDFLAGS     += -lpthread

CXXFLAGS    := -Wall -fsigned-char -Wno-unused 
CXXFLAGS    += -fprofile-arcs -ftest-coverage -std=c++1z
CXXFLAGS    += -Wall

ifeq ($(cfg), release)
CXXFLAGS    += -O2
else
CXXFLAGS    += -g
endif

CXXFLAGS    += -Wl,-rpath=./lib
CXXFLAGS    += $(INCLUDE_LIST)

all: $(PROG)

$(PROG): $(OBJECTS)
	@echo Linking $@
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

$(OUTPUT)/%.o : %.cpp
	@echo Compiling $<
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT)/%.d : %.cpp
	@set -e; rm -f $@; \
	mkdir -p $(OUTPUT); \
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(OUTPUT)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	@rm -rf *.o ./debug ./release $(PROG)

-include $(DEPS)

.PHONY: clean all

##Don't remove any intermediate files
.SECONDARY:
