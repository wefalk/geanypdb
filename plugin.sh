gcc -c geanypdb.c -fPIC `pkg-config --cflags geany`
gcc geanypdb.o -o geanypdb.so -shared `pkg-config --libs geany`
cp -f geanypdb.so /usr/lib/geany
