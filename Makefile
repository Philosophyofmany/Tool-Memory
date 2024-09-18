CC = gcc
CFLAGS = -Wall -Wextra -O2

all: profiler

profiler: main.o profiling.o user_code.o
    $(CC) $(CFLAGS) -o profiler main.o profiling.o user_code.o

main.o: main.c profiling.h user_code.h
    $(CC) $(CFLAGS) -c main.c

profiling.o: profiling.c profiling.h
    $(CC) $(CFLAGS) -c profiling.c

user_code.o: user_code.c user_code.h
    $(CC) $(CFLAGS) -c user_code.c

clean:
    rm -f *.o profiler
