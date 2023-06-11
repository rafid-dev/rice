_THIS     := $(realpath $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
_ROOT     := $(_THIS)
CXX       := g++
TARGET    := Rice
CXXFLAGS  :=  -Wall -Wcast-qual -fno-exceptions -std=c++17 -pedantic -Wextra -Wdouble-promotion -Wformat=2 -Wnull-dereference -Wunused -Wundef -DNDEBUG
NATIVE     = -march=native

# engine name
NAME      := Rice

# Detect Windows
ifeq ($(OS), Windows_NT)
	uname_S := Windows
	SUFFIX  := .exe

	ifeq ($(CXX),g++)
  		FLAGS    = -pthread -lstdc++ -static
		CXXFLAGS += -static -static-libgcc -static-libstdc++ -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
	else ifeq ($(CXX),clang++)
  		CXXFLAGS += -static -fuse-ld=lld
	endif
	
else

	SUFFIX  :=
	uname_S := $(shell uname -s)

	ifeq ($(CXX),g++)
		FLAGS   = -lpthread -lstdc++
	else ifeq ($(CXX),clang++)
  		CXXFLAGS += -static -fuse-ld=lld
	endif

endif

# Different native flag for macOS
ifeq ($(uname_S), Darwin)
	NATIVE = -mcpu=apple-a14	
	FLAGS = 
endif

# Remove native for builds
ifdef build
	NATIVE =
endif

# SPECIFIC BUILDS
ifeq ($(build), native)
	NATIVE   = -march=native
	ARCH     = -x86-64-native
	CXXFLAGS += -w
endif

ifeq ($(build), profile)
	NATIVE   = -march=native
	ARCH     = -x86-64-native
	CXXFLAGS += -w -pg
endif

ifeq ($(build), x86-64)
	NATIVE       = -mtune=znver1
	INSTRUCTIONS = -msse -msse2 -mpopcnt
	ARCH         = -x86-64
endif

ifeq ($(build), x86-64-modern)
	NATIVE       = -mtune=znver2
	INSTRUCTIONS = -m64 -msse -msse3 -mpopcnt
	ARCH         = -x86-64-modern
endif

ifeq ($(build), x86-64-avx2)
	NATIVE       = -mtune=znver3
	INSTRUCTIONS = -m64 -msse -msse3 -mpopcnt -mavx -mavx2 -mssse3 -msse2
	ARCH         = -x86-64-avx2
endif

ifeq ($(build), x86-64-bmi2)
	NATIVE       = -mtune=znver3
	INSTRUCTIONS = -m64 -msse -msse3 -mpopcnt -mavx -mavx2 -msse4.1 -mssse3 -msse2 -mbmi -mbmi2
	ARCH         = -x86-64-bmi2
endif

ifeq ($(build), debug)
	CXXFLAGS = -g3 -fno-omit-frame-pointer -std=c++20 -fsanitize=address
	NATIVE   = -msse -msse3 -mpopcnt -mavx -mavx2
	FLAGS    = -lpthread -lstdc++
endif

SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,%.o,$(SOURCES))
DEPENDS := $(patsubst %.cpp,%.d,$(SOURCES))
EXE     := $(NAME)$(SUFFIX)

Rice: $(TARGET)
clean:
	rm -rf *.o $(DEPENDS)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(EXE) $^ $(CXXFLAGS) $(NATIVE) $(INSTRUCTIONS) $(FLAGS) -flto

-include $(DEPENDS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(NATIVE) $(INSTRUCTIONS) -funroll-loops -O3 -flto -MMD -MP -c -o $@ $< $(FLAGS) -w