OUTPUT=
INSTALLOUTPUT=./installer
INCDIR=./include
SOURCEDIR=./source
deps=./depsAndObjects
CPPC=g++
C++standart=-std=c++23
OPT=-O2
DEPFLAGS=-MP -MD
GENERALFLAGS=$(C++standart) -g3 -w
CFLAGS=$(GENERALFLAGS) $(OPT) $(DEPFLAGS)
CFILES=$(foreach D, $(SOURCEDIR), $(wildcard $(D)/*.cpp))
OBJECTS=$(patsubst $(SOURCEDIR)%.cpp, $(deps)%.o, $(CFILES))
DEPFILES= $(patsubst $(SOURCEDIR)%.cpp, $(deps)%.d, $(CFILES)) $(deps)/installer.d $(deps)/tests.d
INSTALLOBJECTS= $(deps)/installer.o $(deps)/alias.o $(deps)/BuilderFilework.o $(deps)/uninstall.o $(deps)/filework.o $(deps)/algs.o $(deps)/Mapping.o

BELDERBINARY = ./belder
TESTSDIR     = ./tests
TESTBIN      = $(TESTSDIR)/test_runner
TESTSOURCES  = $(wildcard $(TESTSDIR)/test_*.cpp)
TESTMAIN     = $(TESTSDIR)/main.cpp
TESTOBJECTS  = $(patsubst $(TESTSDIR)/%.cpp, $(deps)/t_%.o, $(TESTSOURCES)) $(deps)/t_main.o

GTEST_CFLAGS := $(shell pkg-config --cflags gtest 2>/dev/null)
GTEST_LIBS   := $(shell pkg-config --libs gtest 2>/dev/null || echo "-lgtest")

all:$(OBJECTS)

link:$(OUTPUT)

pocket:$(INSTALLOUTPUT)
	@$(INSTALLOUTPUT) pocket

install:$(INSTALLOUTPUT)
	@$(INSTALLOUTPUT)

$(INSTALLOUTPUT):$(INSTALLOBJECTS)
	$(CPPC) $^ -o $@

$(OUTPUT):$(OBJECTS)
	$(CPPC) $^ -o $@

$(BELDERBINARY): $(OBJECTS)
	$(CPPC) $^ -o $@ #-lpthread

$(deps)/t_%.o: $(TESTSDIR)/%.cpp
	$(CPPC) $(C++standart) -g3 -w $(GTEST_CFLAGS) -I$(TESTSDIR) -DBELDER_BINARY='"$(abspath $(BELDERBINARY))"' -c $< -o $@

$(TESTBIN): $(TESTOBJECTS)
	#@if [ -z "$(GTEST_LIBS)" ] && ! $(CPPC) -lgtest /dev/null -o /dev/null 2>/dev/null; then \
	#    echo "ERROR: GoogleTest not found. Install with: sudo apt-get install libgtest-dev"; \
	#    exit 1; \
	#fi
	$(CPPC) $^ -o $@ $(GTEST_LIBS) #-lpthread -lstdc++fs

TEST_ARGS ?=

.PHONY: test test-verbose test-failfast
test: $(BELDERBINARY) $(TESTBIN)
	@mkdir -p $(HOME)/builder
	#@echo "Running belder test suite..."
	#@echo "WARNING: Some tests require gcc, g++, clang, clang++, as, ar."
	#@echo "         Tests for missing toolchains will be skipped with full install instructions."
	#@echo "         Pass TEST_ARGS=\"--gtest_filter=...\" to run specific tests."
	#@echo ""
	@./$(TESTBIN) $(TEST_ARGS)

test-verbose: $(BELDERBINARY) $(TESTBIN)
	@mkdir -p $(HOME)/builder
	@./$(TESTBIN) --gtest_print_time=1 --gtest_color=yes $(TEST_ARGS)

mrproper:
	rm -rf $(OBJECTS) $(DEPFILES) $(INSTALLOBJECTS) $(INSTALLOUTPUT) ./pocketbuilder $(BELDERBINARY) $(TESTBIN) $(TESTOBJECTS)

$(deps)/%.o:$(SOURCEDIR)/%.cpp
	$(CPPC) $(CFLAGS) $(foreach D,$(INCDIR),-I$(D)) -c $< -o $@

$(deps)/installer.o:./installer.cpp
	$(CPPC) $(CFLAGS) $(foreach D,$(INCDIR),-I$(D)) -c $< -o $@

-include $(DEPFILES)