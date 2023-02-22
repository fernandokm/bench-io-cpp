size=10M
BUILD_DIR=build
PRGM=main
FLAGS=-O3 -g --std=c++17 -isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread -pedantic -Wall -Wextra -Wfloat-equal -Wundef  -Wshadow=local -Wpointer-arith -Wcast-align -Wwrite-strings -Wcast-qual -Wunreachable-code
BENCH_FLAGS=--benchmark_time_unit=ms --benchmark_color=true

.PHONY: run run-sanitized perf file clean

run: ./$(BUILD_DIR)/$(PRGM)
	./$(BUILD_DIR)/$(PRGM) ./test.txt $(BENCH_FLAGS) --benchmark_min_time=5

run-sanitized: ./$(BUILD_DIR)/$(PRGM)-sanitized
	./$(BUILD_DIR)/$(PRGM)-sanitized ./test.txt $(BENCH_FLAGS) --benchmark_min_time=0

file: 
	head -c $(size) </dev/urandom >test.txt

perf: ./$(BUILD_DIR)/$(PRGM)
	perf record --call-graph dwarf ./$(BUILD_DIR)/$(PRGM) ./test.txt $(BENCH_FLAGS) --benchmark_min_time=5
	perf script -F +pid > perf.txt

clean:
	rm *perf*
	rm ./$(BUILD_DIR)/*

./$(BUILD_DIR)/$(PRGM): $(PRGM).cpp
	g++ $(PRGM).cpp $(FLAGS) -o ./$(BUILD_DIR)/$(PRGM)

./$(BUILD_DIR)/$(PRGM)-sanitized: $(PRGM).cpp
	g++ $(PRGM).cpp $(FLAGS) -fsanitize=address,leak,undefined -o ./$(BUILD_DIR)/$(PRGM)-sanitized
