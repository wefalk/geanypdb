SRC = src
all:
	gcc -c $(SRC)/geanypdb.c -fPIC `pkg-config --cflags geany` -o $(SRC)/geanypdb.o
	gcc $(SRC)/geanypdb.o -o $(SRC)/geanypdb.so -shared `pkg-config --libs geany`
install:
	cp -f $(SRC)/geanypdb.so /usr/lib/geany
clean:
	rm $(SRC)/geanypdb.o
	rm $(SRC)/geanypdb.so
