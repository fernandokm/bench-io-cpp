size=500K
BUILD_DIR=build
PRGM=main

.PHONY: run build

build:
	g++ $(PRGM).cpp -O3 -isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread -o ./$(BUILD_DIR)/$(PRGM).out

run: build
	./$(BUILD_DIR)/$(PRGM).out ./test.txt --benchmark_time_unit=ms --benchmark_min_time=2

file: 
	head -c $(size) </dev/urandom >test.txt

clean:
	rm *perf*
	rm ./$(BUILD_DIR)/*