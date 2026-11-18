CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
.PHONY: test clean
test:
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude src/gutc_path.c tests/test_path.c -o test_path
	./test_path
clean:
	rm -f test_path
