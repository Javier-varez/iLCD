all:
	gcc -Os -Wall -o iLCD main.c

clean:
	rm -f iLCD
