testfile=prac3.cpp

a.out: Ass2_54_19CS10069_19CS30007.cpp
	g++ -W -o shell Ass2_54_19CS10069_19CS30007.cpp

ref: ref.c
	cc -o sibashell ref.c
	./sibashell

temp: temp.cpp
	g++ -o t temp.cpp
	./t

prac: $(testfile)
	g++ -o prac $(testfile)
	./prac

clean:
	rm -f sibashell shell t output.txt prac

