CC = gcc
CFLAGS = -std=c11 -Wall -Wextra

all: parser

parser.tab.c parser.tab.h: parser.y ast.h
	bison -d -o parser.tab.c parser.y

lex.yy.c: lexer.l parser.tab.h ast.h
	flex -o lex.yy.c lexer.l

parser: main.c ast.c parser.tab.c lex.yy.c ast.h
	$(CC) $(CFLAGS) -o parser main.c ast.c parser.tab.c lex.yy.c

clean:
	rm -f parser parser.exe parser.tab.c parser.tab.h lex.yy.c ast.json generated.py