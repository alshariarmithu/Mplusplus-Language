all: mpp

mpp: mpp.tab.c lex.yy.c
	gcc mpp.tab.c lex.yy.c -o mpp -lfl

mpp.tab.c mpp.tab.h: mpp.y
	bison -d mpp.y

lex.yy.c: mpp.l
	flex mpp.l

clean:
	rm -f mpp mpp.tab.c mpp.tab.h lex.yy.c