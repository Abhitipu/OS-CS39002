a.out: Ass3_Part2*.cpp
	g++ -W -o prog2 Ass3_Part2*.cpp -lpthread
	./prog2 < input.txt > output.txt 2>error.log

ref: sir.c
	cc -o ref sir.c -lpthread
	./ref

clean:
	rm -f ref prog2 output.txt error.log
