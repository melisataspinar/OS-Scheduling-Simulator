scheduler: schedule.c
	gcc -o schedule schedule.c -lpthread -lm

clean:
	rm -fr schedule *dSYM
