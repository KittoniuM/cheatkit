NAME=memf
RELEASE="0.0a"
COMMIT="$(shell test -d .git && git rev-parse HEAD || echo)"

CFLAGS= -O3 \
	-ftrapv -g \
	-std=c11 \
	-Wall -Wextra -Wshadow -Wformat -Wconversion -Wdouble-promotion \
	-Wstrict-prototypes \
	-pipe
CMACROS=-D __RELEASE__=$(RELEASE) \
	-D __COMMIT__=$(COMMIT)
LDFLAGS=-lm

TESTS=$(wildcard tests/*.test)
AMALG=src/.amalg.c
SRC=src/main.c src/memf.c
OBJ=$(SRC:.c=.o)
OUT=$(NAME)

.PHONY: default all amalg depend test clean

default all: $(OBJ)
	@$(CC) $(LDFLAGS) $^ -o $(OUT)

amalg: $(AMALG)
	@$(CC) $(CMACROS) $(CFLAGS) $(LDFLAGS) $< -o $(OUT)

depend:
	@$(CC) -E -MM $(SRC) > Makefile.dep

test:

clean:
	@rm -f $(OUT) $(AMALG) $(OBJ)

include Makefile.dep

$(AMALG): $(SRC)
	@cat $^ > $@
