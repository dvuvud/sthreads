DEBUG   := n
CC      := gcc
OS      := $(shell uname)
CFLAGS  := -std=gnu99 -Werror -Wall  -Wno-deprecated-declarations -I ../psem
LDFLAGS :=

ifeq ($(DEBUG), y	)
	CFLAGS += -DDEBUG -g
endif

ifeq ($(OS), Linux)
    CFLAGS += -pthread
    LDLIBS += -pthread -lrt
endif

.PHONY: all clean

all: bin/sthreads_test
	
bin/sthreads_test: obj/sthreads_test.o obj/sthreads.o src/sthreads.h 
	$(CC) $(CFLAGS) $(LDLIBS) $(filter-out src/sthreads.h, $^) -o $@

obj/sthreads.o: src/sthreads.c src/sthreads.h
	$(CC) $(CFLAGS) -c  $(filter-out src/sthreads.h, $^) -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c  $< -o $@

clean:
	$(RM) *~ src/*~ src/#* obj/*.o bin/*
	$(RM) -rf bin/*.dSYM

