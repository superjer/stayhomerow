
jcodes: jcodes.c
	rm -rf jcodes
	gcc -std=c99 -o jcodes jcodes.c -lX11
