CC = gcc
CXX = g++ -std=c++11
# Additional include directory
INCLUDES = 

# Compilation options:
# -g for degging infor and -Wall enables all warnings
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

# Linking options:
# -g for debugging info
LDFLAGS = -g

# List the libraries you need to link with in LDLIBS
# -lm for the math library
LDLIBS = -L. 

SOURCES=globalstate.cpp pverify.cpp statemachine.cpp stoppingstate.cpp errorstate.cpp sync.cpp cycle.cpp service.cpp lookup.h

OBJECTS=$(SOURCES:.cpp=.o)


# The 1st target gets build when typing "make"
libpverify.a: $(OBJECTS)
	ar rc libpverify.a $(OBJECTS)
	ranlib libpverify.a

.PHONY: clean
clean:
	rm -f *.o *.a a.out core test 

.PHONY: all
all: clean libpverify.a

