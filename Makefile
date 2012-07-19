PROGRAM = all

all: videopipe audiopipe

audiopipe: main.o
	gcc -framework CoreServices -framework CoreAudio -framework AudioUnit -o audiopipe main.o

main.o: main.c
	gcc -c main.c

videopipe: videopipe.o
	gcc -framework OpenGL -framework GLUT -o videopipe videopipe.o

videopipe.o: videopipe.c
	gcc -c videopipe.c

clean:; rm -f *.o *~ audiopipe videopipe

install:
	install -s audiopipe /usr/local/bin
	install -s videopipe /usr/local/bin
