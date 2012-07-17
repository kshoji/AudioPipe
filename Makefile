PROGRAM = audiopipe

$(PROGRAM): main.o
	gcc -framework CoreServices -framework CoreAudio -framework AudioUnit -o $(PROGRAM) main.o

main.o: main.c
	gcc -c main.c

clean:; rm -f *.o *~ $(PROGRAM)

install: $(PROGRAM)
	install -s $(PROGRAM) /usr/local/bin
