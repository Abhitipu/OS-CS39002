ourdemo: ourdemo.cpp libmemlab.a
	g++ -g -o ourdemo ourdemo.cpp -L. -lmemlab -lpthread

libmemlab.a: memlab.o
	ar -rcs libmemlab.a memlab.o

memlab.o: memlab.h memlab.cpp
	g++ -Wall -c -g memlab.cpp

clean:
	rm *.out libmemlab.a memlab.o