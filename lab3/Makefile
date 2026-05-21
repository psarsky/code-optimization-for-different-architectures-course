CC     = gcc
OPT    = -O2
SIZE   = 1500

all: 1 2 3 4 5 6 7 8

1: ge1.c
	$(CC) ge1.c -o ge1

2: ge2.c
	$(CC) $(OPT) ge2.c -o ge2

3: ge3.c
	$(CC) $(OPT) ge3.c -o ge3

4: ge4.c
	$(CC) $(OPT) ge4.c -o ge4

5: ge5.c
	$(CC) $(OPT) ge5.c -o ge5

6: ge6.c
	$(CC) $(OPT) ge6.c -o ge6

7: ge7.c
	$(CC) $(OPT) -msse3 ge7.c -o ge7

8: ge8.c
	$(CC) $(OPT) -mavx ge8.c -o ge8

run1: 1
	./ge1 $(SIZE)

run2: 2
	./ge2 $(SIZE)

run3: 3
	./ge3 $(SIZE)

run4: 4
	./ge4 $(SIZE)

run5: 5
	./ge5 $(SIZE)

run6: 6
	./ge6 $(SIZE)

run7: 7
	./ge7 $(SIZE)

run8: 8
	./ge8 $(SIZE)

runall: all
	@echo ""
	@echo "=== ge1 (no opt) ==="
	@./ge1 $(SIZE)
	@echo ""
	@echo "=== ge2 (register counters, -O2) ==="
	@./ge2 $(SIZE)
	@echo ""
	@echo "=== ge3 (register multiplier, -O2) ==="
	@./ge3 $(SIZE)
	@echo ""
	@echo "=== ge4 (loop unroll x8, -O2) ==="
	@./ge4 $(SIZE)
	@echo ""
	@echo "=== ge5 (1D array + IDX macro, -O2) ==="
	@./ge5 $(SIZE)
	@echo ""
	@echo "=== ge6 (cache blocking + packing, -O2) ==="
	@./ge6 $(SIZE)
	@echo ""
	@echo "=== ge7 (SSE3, -O2) ==="
	@./ge7 $(SIZE)
	@echo ""
	@echo "=== ge8 (AVX, -O2) ==="
	@./ge8 $(SIZE)

clean:
	rm -f ge1 ge2 ge3 ge4 ge5 ge6 ge7 ge8

.PHONY: all clean runall \
        1 2 3 4 5 6 7 8 \
        run1 run2 run3 run4 run5 run6 run7 run8
