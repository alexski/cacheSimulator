X = first

all: $(X)

$(X): $(X).c
	gcc -Wall -Werror -fsanitize=address -lm -g $(X).c -o $(X)

clean:
	rm -rf $(X)
