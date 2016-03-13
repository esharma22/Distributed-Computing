findgold: goldchase.cpp libmap.a goldchase.h
	g++ -std=c++11 -g goldchase.cpp -o findgold -L. -lmap -lpanel -lncurses -lrt -lpthread

libmap.a: Screen.o Map.o
	ar -r libmap.a Screen.o Map.o

Screen.o: Screen.cpp
	g++ -c Screen.cpp

Map.o: Map.cpp
	g++ -c Map.cpp

clean:
	rm -f Screen.o Map.o libmap.a findgold
