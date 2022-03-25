ourdemo.out: ourdemo.cpp libmemlab.a
	g++ -o ourdemo.out ourdemo.cpp -L. -lmemlab -lpthread
	./ourdemo.out

libmemlab.a: memlab.o
	ar -rcs libmemlab.a memlab.o

memlab.o: memlab.h memlab.cpp
	g++ -Wall -c memlab.cpp

clean:
	rm *.out libmemlab.a memlab.o