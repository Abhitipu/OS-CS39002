test: a.out
	./a.out < input.txt > output.txt 2>error.log

a.out: Assignment_4_54.cpp
	g++ -W Assignment_4_54.cpp -lpthread

clean:
	rm -f a.out test
