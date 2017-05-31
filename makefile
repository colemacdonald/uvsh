all:
	gcc -Wall uvsh.c -o uvsh
clean:
	rm uvsh uvshp
plus:
	gcc -Wall uvshplus.c -o uvshp