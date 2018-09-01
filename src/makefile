# origin makefile
build: util.o MLList.o SFS.o SFSDemo.o
	gcc -g -o build util.o MLList.o SFS.o SFSDemo.o
SFSDemo.o: SFSDemo.c SFS.h
	gcc -g -c SFSDemo.c
SFS.o: SFS.h SFS.c MLList.h util.h
	gcc -g -c SFS.c
MLList.o: MLList.h MLList.c
	gcc -g -c MLList.c
util.o: util.h util.c
	gcc -g -c util.c
clean:
	rm *.o


# gcc -g -Wall test.c -o test -Wl,-rpath=/usr/lib/debug
# debug mode
# build: util.o MLList.o SFS.o
# 	gcc -g -o -Wall build util.o MLList.o SFS.o -Wl, -rpath=/usr/lib/debug
# SFS.o: SFS.c MLList.h util.h
# 	gcc -g -c -Wall SFS.c -Wl, -rpath=/usr/lib/debug
# MLList.o: MLList.h MLList.c
# 	gcc -g -c -Wall MLList.c -Wl, -rpath=/usr/lib/debug
# util.o: util.h util.c
# 	gcc -g -c -Wall util.c -Wl, -rpath=/usr/lib/debug
# clean:
# 	rm *.o
