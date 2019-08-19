
fp:as608.o utils.o main.c
	gcc -g -o fp main.c as608.o utils.o -lwiringPi -lm

as608.o:../as608.c ../as608.h
	gcc -o as608.o -c ../as608.c

utils.o:./utils.c ./utils.h
	gcc -o utils.o -c ./utils.c

.PHONY:clean
clean:
	rm ./fp ./as608.o ./utils.o
