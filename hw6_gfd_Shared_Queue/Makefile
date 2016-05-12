

all :
	gcc main.c  occurance_list.c -o exec -pthread

debug:
	gcc main.c occurance_list.c -DDEBUG  -o exec -pthread 

valgrind :
	valgrind --leak-check=full --tool=memcheck --show-leak-kinds=all ./exec hm a
