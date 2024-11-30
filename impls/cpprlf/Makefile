CXX=g++-mp-14
CXXFLAGS=-Wall -std=c++2b
LD=$(CXX)

## -Og for debugging
## -O3 for max optimizations

#CXXFLAGS+=-fsanitize=leak
# address sanitizer not available for arm64
# I got errors trying hwaddress...might need to install a package?
#CXXFLAGS+=-fsanitize=hwaddress
# I got errors with undefined too
#CXXFLAGS+=-fsanitize=undefined

DEBUG=-Og -g
RELEASE=-O3 -DNDEBUG

ifeq ($(BUILD),ndebug)
CXXFLAGS+=$(RELEASE)
LDFLAGS+=$(RELEASE)
else
CXXFLAGS+=$(DEBUG)
LDFLAGS+=$(DEBUG)
endif

SRCS=$(wildcard *.cpp)
MAINS=$(wildcard step*.cpp)
NOTMAINS=$(filter-out $(MAINS),$(SRCS))

TARGETS=$(MAINS:%.cpp=%)
OBJECTS=$(NOTMAINS:%.cpp=%.o)

.PHONY: all maketest clean

.SUFFIXES: .cpp .o

all: $(TARGETS)

ndebug:
	make "BUILD=ndebug"

maketest:
	@echo "MAINS: $(MAINS)"
	@echo "NOTMAINS: $(NOTMAINS)"

.deps: *.cpp *.hpp
	$(CXX) $(CXXFLAGS) -MM *.cpp > .deps

$(TARGETS): %: %.o $(OBJECTS)
	$(LD) $^ -o $@ $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf *.o $(TARGETS) .deps

-include .deps
