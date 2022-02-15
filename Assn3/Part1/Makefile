a.out: Ass3_Part1*.cpp
	g++ -o prog1 Ass3_Part1*.cpp -lpthread
	./prog1 < genout.txt > output.txt

ref: sir.c
	cc -o ref sir.c -lrt
	./ref

gen: gen.cpp
	g++ -o gen gen.cpp 
	./gen > genout.txt

clean:
	rm -f ref prog1 genout.txt gen
