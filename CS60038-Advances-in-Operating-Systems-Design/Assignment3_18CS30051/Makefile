main: main.o disk.o sfs.o
	gcc -o main main.o disk.o sfs.o -lm
main.o: main.c disk.h sfs.h
	gcc -c -g main.c -lm
sfs.o: sfs.c sfs.h
	gcc -c -g sfs.c -lm
disk.o: disk.c disk.h
	gcc -c -g disk.c -lm
clean:
	rm -f main main.o disk.o sfs.o