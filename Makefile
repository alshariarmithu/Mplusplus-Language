main:
	flex mpp.l
	gcc lex.yy.c -o a.out
	./a.out test.mpp