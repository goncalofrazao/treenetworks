cot:
	clang -fsanitize=signed-integer-overflow -fsanitize=undefined -ggdb3 -O0 -Qunused-arguments -std=c11 -Wall -Werror -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wshadow -o cot main.c requests.c

clear:
	rm -r cot cot.dSYM