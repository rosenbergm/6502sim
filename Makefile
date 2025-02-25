TARGET=main.out
CC=g++
CFLAGS=-std=c++20 -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy \
	-Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations \
	-Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls \
	-Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 \
	-Wswitch-default -Wundef -Wno-unused -Wmaybe-uninitialized -Wno-strict-overflow

SOURCES=$(wildcard *.cpp)
HEADERS=$(wildcard *.h)
OBJECTS=$(SOURCES:.cpp=.o)

DEPS=$(OBJECTS:.o=.d)

-include $(DEPS)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

check:
	cppcheck --enable=all --inconclusive --std=c++20 --language=c++ --suppress=missingIncludeSystem ${SOURCES} ${HEADERS}

%.bin: %.s
	vasm6502_oldstyle -Fbin -dotdir -pad=0 -o $@ $<

run: ${TARGET}
	./${TARGET} ${ARGS}

clean:
	rm -f ${TARGET} ${OBJECTS} ${DEPS}

.PHONY: all clean run check
