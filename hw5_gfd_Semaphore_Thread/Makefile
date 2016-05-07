

all :
	gcc main.c -o exec -pthread

debug:
	gcc main.c -DDEBUG -DDEBUG_FILE_READ -o exec -pthread 

valgrind :
	valgrind --leak-check=full --tool=memcheck --show-leak-kinds=all ./exec hm a
