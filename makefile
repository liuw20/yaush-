cc=gcc
OBJ=final_version
all:
	$(cc) -c ./*c
	$(cc) -o $(OBJ) ./*.o -lreadline
