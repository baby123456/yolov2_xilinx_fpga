

all:main


main: main.o cnn.o
	g++ main.o cnn.o -o main
main.o:
	g++ -c main.cpp -o main.o
cnn.o:
	g++ -c cnn.cpp -o cnn.o
clean:
	rm -rf cnn.o main.o main
