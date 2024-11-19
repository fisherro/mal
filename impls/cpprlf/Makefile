CXX=g++-mp-14
CXXFLAGS=-O3 -Wall -std=c++2b
LD=$(CXX)

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

$(TARGETS): %: %.o $(OBJECTS)
	$(LD) $^ -o $@ $(LDFLAGS)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf *.o $(TARGETS)
