

all :
	gcc main.c -o exec -pthread

debug:
	gcc main.c -DDEBUG -o exec -pthread 

valgrind :
	valgrind --leak-check=full --tool=memcheck --show-leak-kinds=all ./exec Dir ece

hardfile :
	valgrind --leak-check=full --tool=memcheck --show-leak-kinds=all ./exec hm a

2run : 
	./exec s ece

run : 
	./exec Dir ece