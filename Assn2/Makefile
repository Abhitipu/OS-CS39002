testfile=prac3.cpp

a.out: Ass2_54_19CS10069_19CS30007.cpp
	g++ -W -o shell Ass2_54_19CS10069_19CS30007.cpp


temp: temp.cpp
	g++ -o t temp.cpp
	./t

prac: $(testfile)
	g++ -o prac $(testfile)
	./prac

clean:
	rm -f shell t output.txt prac temp3 temp2

