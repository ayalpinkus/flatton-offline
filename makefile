
all: flatton.a

clean:
	-rm *.o *.a

flatton.a: flatton.o lodepng.o
	g++ -o flatton.a flatton.o lodepng.o

flatton.o: flatton.cpp
	g++ -c -o flatton.o flatton.cpp

lodepng.o: lodepng.cpp
	g++ -c -o lodepng.o lodepng.cpp

