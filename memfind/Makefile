NAME=memfind
RELEASE="0.0a"
COMMIT="$(shell test -d .git && git rev-parse HEAD || echo)"

CFLAGS= -march=native -O0 \
	-ftrapv -g \
	-std=c11 \
	-Wall -Wextra -Wshadow -Wformat -Wconversion -Wdouble-promotion \
	-Waggregate-return -Wstrict-prototypes \
	-pipe
CMACROS=-D __RELEASE__=$(RELEASE) \
	-D __COMMIT__=$(COMMIT)
LDFLAGS=-lOpenCL -lm

TESTS=$(wildcard tests/*.test)
AMALG=src/.amalg.c
SRC=memf.c
OBJ=$(SRC:.c=.o)
OUT=$(NAME)

.PHONY: default all amalg test clean

default all: $(OBJ)
	@$(CC) $(LDFLAGS) $^ -o $(OUT)

amalg: $(AMALG)
	@$(CC) $(CMACROS) $(CFLAGS) $(LDFLAGS) $< -o $(OUT)

test:
	@$(foreach x,$(TESTS), \
		echo TEST $(x); \
		./$(x) && echo OK || echo FAIL)

clean:
	@rm -f $(OUT) $(AMALG) $(OBJ)


%.o: %.c
	@$(CC) $(CMACROS) $(CFLAGS) -c $< -o $@

$(AMALG): $(SRC)
	@cat $^ > $@
