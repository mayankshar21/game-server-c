game: image_tagger.o server.o
	gcc -Wall -g -O3 image_tagger.o server.o -o image_tagger

server.o: server.c server.h
	gcc -Wall -g -O3 -c server.c 

image_tagger.o: image_tagger.c server.h
	gcc -Wall -g -O3 -Wextra -std=gnu99 -c image_tagger.c 

clean:
	rm *.o image_tagger
