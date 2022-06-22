CFLAGS = -Wall -Wextra -Werror

all: claudshell.c
	gcc $(CFLAGS) -o claudshell claudshell.c

clean:
	rm -f claudshell