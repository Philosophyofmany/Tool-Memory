CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lpapi  # Link with the PAPI library

all: profiler

profiler: main.o profiling.o user_code.o
	$(CC) $(CFLAGS) -o profiler main.o profiling.o user_code.o $(LIBS)  # Link against PAPI

main.o: main.c profiling.h user_code.h
	$(CC) $(CFLAGS) -c main.c

profiling.o: profiling.c profiling.h
	$(CC) $(CFLAGS) -c profiling.c

user_code.o: user_code.c user_code.h
	$(CC) $(CFLAGS) -c user_code.c

clean:
	rm -f *.o profiler
