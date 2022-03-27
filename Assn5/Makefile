demo2: demo2.cpp libmemlab.a
	g++ -g -o demo2 demo2.cpp -L. -lmemlab -lpthread
	./demo2 2> error.log

demo1: demo1.cpp libmemlab.a
	g++ -g -o demo1 demo1.cpp -L. -lmemlab -lpthread
	./demo1 2> error.log
	
ourdemo: ourdemo.cpp libmemlab.a
	g++ -g -o ourdemo ourdemo.cpp -L. -lmemlab -lpthread
	./ourdemo
libmemlab.a: memlab.o
	ar -rcs libmemlab.a memlab.o

memlab.o: memlab.h memlab.cpp
	g++ -Wall -c -g memlab.cpp

play: play.cpp
	g++ -o play play.cpp
	./play

play2: play.cpp libmemlab.a
	g++ -g -o play play.cpp -L. -lmemlab -lpthread
	./play 2> error.log

clean:
	rm -f ourdemo *.out libmemlab.a memlab.o play demo1 demo2
