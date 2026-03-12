CC = gcc
CFLAGS = -O2

SRCS = mm1.c mm2.c mm3.c mm4.c mm5_4.c mm5_8.c mm6.c mm7.c mm8.c mm9.c
BINS = $(SRCS:.c=)

all: build run

build: $(BINS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: build
	@echo ""
	@echo "Porownanie czasow wykonania (gcc -O2)"
	@echo ""
	@for prog in $(BINS); do \
		echo "$$prog"; \
		./$$prog; \
		echo ""; \
	done

clean:
	rm -f $(BINS)

.PHONY: all build run clean
