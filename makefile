
all: flatton.a

clean:
	-rm *.o *.a *.obj

flatton.a: flatton.o lodepng.o
	g++ -o flatton.a flatton.o lodepng.o

flatton.o: flatton.cpp
	g++ -c -o flatton.o flatton.cpp

lodepng.o: lodepng.cpp
	g++ -c -o lodepng.o lodepng.cpp



#
# Experimental: windows build using mingw.
# 1. install with 'sudo apt-get install mingw-w64'
# 2. type make windows
#

windows:
	i686-w64-mingw32-g++ -c -o lodepng32.obj lodepng.cpp
	i686-w64-mingw32-g++ -c -o flatton32.obj flatton.cpp
	i686-w64-mingw32-g++ -o flatton32.exe flatton32.obj lodepng32.obj
	x86_64-w64-mingw32-g++ -c -o lodepng64.obj lodepng.cpp
	x86_64-w64-mingw32-g++ -c -o flatton64.obj flatton.cpp
	x86_64-w64-mingw32-g++ -o flatton64.exe flatton64.obj lodepng64.obj
	
