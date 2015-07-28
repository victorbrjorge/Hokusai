all: hokusai

hokusai: main.o countmin.o
	g++ -std=c++11 -pthread -O3 main.o countmin.o -o hokusai

main.o: main.cpp
	g++ -c -std=c++11 -pthread -O3 -Wall -pg main.cpp

countmin.o: countmin.cpp
	g++ -c -std=c++11 -pthread -O3 -Wall -pg countmin.cpp

clean:

