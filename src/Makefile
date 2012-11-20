CPP=g++
CPPFLAG=-g -Wall

recovery: main.o func.o
	$(CPP) -o $@ $^

main.o: entry.h func.h

func.o: entry.h

clean:
	$(RM) -rf *.o recovery
