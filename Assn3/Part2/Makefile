runprog2: prog2
	./prog2 < input.txt > output.txt 2>error.log
	
prog2: Ass3_Part2*.cpp
	g++ -W -o prog2 Ass3_Part2*.cpp -lpthread

clean:
	rm -f prog2 output.txt error.log
