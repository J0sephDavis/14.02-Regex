regex: regex.c
	$(CC) regex.c -o regex -Wall -Wextra -pedantic -Wshadow -Werror -std=c99
clean: regex
	rm -f regex
