all:
	gcc rayCaster.c parser.c -o raycast

clean:
	rm -rf raycast *~