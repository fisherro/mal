CXX=g++-mp-14
CXXFLAGS=-O0 -Wall -std=c++2b
LD=$(CXX)

#CXXFLAGS+=-fsanitize=leak
# address sanitizer not available for arm64
# I got errors trying hwaddress...might need to install a package?
#CXXFLAGS+=-fsanitize=hwaddress
# I got errors with undefined too
#CXXFLAGS+=-fsanitize=undefined

DEBUG=-g
CXXFLAGS+=$(DEBUG)
LDFLAGS+=$(DEBUG)

SRCS=$(wildcard *.cpp)
MAINS=$(wildcard step*.cpp)
NOTMAINS=$(filter-out $(MAINS),$(SRCS))

TARGETS=$(MAINS:%.cpp=%)
OBJECTS=$(NOTMAINS:%.cpp=%.o)

.PHONY: all maketest clean

.SUFFIXES: .cpp .o

all: $(TARGETS)

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
