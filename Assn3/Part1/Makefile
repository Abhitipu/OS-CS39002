runprog1: prog1 genout.txt
	./prog1 < genout.txt > output.txt

prog1: Ass3_Part1*.cpp
	g++ -o prog1 Ass3_Part1*.cpp -lpthread

gen: gen.cpp
	g++ -o gen gen.cpp 

rungen genout.txt: gen
	./gen > genout.txt

clean:
	rm -f prog1 genout.txt gen output.txt
