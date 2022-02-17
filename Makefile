CFLAGS ?= -Os -flto -Wl,--gc-sections -g -Wall

target := retroinject

ifeq ($(OS),Windows_NT)
    target := $(target).exe
endif

all: $(target)

objects := retroinject.o

$(target): $(objects)
	$(LINK.c) $^ -o $@

clean:
	rm -f $(objects) $(target)
