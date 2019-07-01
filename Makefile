CXX ?= g++
INCLUDEDIR := -I./include -I./include/FunctionalPlus/include
CXXFLAGS := -Og -std=c++11 -g -fno-omit-frame-pointer -Wall -Wextra ${INCLUDEDIR}

HDR := include/xcs.hpp \
	include/xcs_types.hpp \
	include/utils.hpp

SRC := src/xcs.cpp \
	src/utils.cpp \
	src/XCSLearner.cpp

TESTSRC := tests/XCS.test.cpp
MULTIPLEXER_SRC := tests/Multiplexer.test.cpp

OBJ := $(SRC:.cpp=.o)
TESTS := $(TESTSRC:.test.cpp=.test)
TESTS := $(TESTS) $(MULTIPLEXER_SRC:.test.cpp=.test)
MULTIPLEXER := $(MULTIPLEXER_SRC:.test.cpp=.test)

%.o: %.cpp
	@echo CXX $<
	@${CXX} -MMD ${CXXFLAGS} -o $@ -c $<

%.test.o: %.test.cpp
	@echo CXX $<
	@${CXX} ${CXXFLAGS} -o $@ -c $<

%.test: %.test.o ${OBJ}
	@echo LD $<
	@${CXX} ${CXXFLAGS} $< ${OBJ} -o $@

all: ${OBJ} ${MULTIPLEXER}

-include src/*.d

unittests: ${TESTS}
	@echo TESTING $@
	@./$<

multiplexer: ${MULTIPLEXER}
	@echo TESTING $@
	@./$<

tests: multiplexer

fmt:
	@echo FMT ${SRC} ${MULTIPLEXER_SRC} ${HDR}
	@clang-format -i ${SRC} ${MULTIPLEXER_SRC} ${HDR}

obj:
	@echo ${TESTS}
	@echo ${OBJ}

src:
	@echo ${SRC} ${HDR}

clean:
	rm -f tests/*.o
	rm -f *.csv
	rm -f ${OBJ}
	rm -f ${TESTS}

.PHONY: tests clean obj fmt src
.SECONDARY:

