#include <benchmark/benchmark.h>

#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <streambuf>
#include <string>

// for C-style code
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using std::ifstream;
using std::istreambuf_iterator;
using std::string;
using std::stringstream;

size_t MAXLEN = 10000;

void countlines_getline(std::istream& istr) {
    string buffer;

    // get length of file to reserve space in the buffer
    istr.seekg(0, std::ios::end);
    buffer.reserve(istr.tellg());
    istr.seekg(0);

    while (std::getline(istr, buffer, '\n')) {
        benchmark::DoNotOptimize(buffer);
    }
}

void countlines_read(std::istream& istr) {
    std::ostringstream buf;
    buf << istr.rdbuf();
    benchmark::DoNotOptimize(buf);
}

void countlines_memory(std::istream& istr) {
    string buffer;

    // get length of file to resize buffer
    istr.seekg(0, std::ios::end);
    buffer.resize(istr.tellg());
    istr.seekg(0);

    // read it into memory
    istr.read(buffer.data(), buffer.size());
    benchmark::DoNotOptimize(buffer);
}

void countlines_itr(std::istream& istr) {
    int result = std::reduce(std::execution::par_unseq, istreambuf_iterator<char>(istr),
                             istreambuf_iterator<char>(), 0,
                             [](const char left, const char right) {
                                 return (left == '\n') + (right == '\n');
                             });
    benchmark::DoNotOptimize(result);
}

extern "C" {
// Driver code
void countlines_c_getline(const char* filename) {
    FILE* ptr;

    // Opening file in reading mode
    ptr = fopen(filename, "r");
    if (NULL == ptr) {
        printf("file can't be opened \n");
        return;
    }

    std::size_t len = MAXLEN;
    char* buffer = (char*)malloc(len * sizeof(char));

    // Printing what is written in file
    // character by character using loop.
    do {
        getline(&buffer, &len, ptr);
        benchmark::DoNotOptimize(*buffer);
        // Checking if character is not EOF.
        // If it is EOF stop reading.
    } while (!feof(ptr));

    // Closing the file
    fclose(ptr);
    free(buffer);
}

void countlines_c_char(const char* filename) {
    FILE* ptr;
    char ch;

    // Opening file in reading mode
    ptr = fopen(filename, "r");
    if (NULL == ptr) {
        printf("file can't be opened \n");
        return;
    }

    // Printing what is written in file
    // character by character using loop.
    do {
        ch = fgetc(ptr);
        benchmark::DoNotOptimize(ch);
        // Checking if character is not EOF.
        // If it is EOF stop reading.
    } while (!feof(ptr));

    // Closing the file
    fclose(ptr);
}

void countlines_c_in_memory(const char* filename) {
    FILE* ptr;
    char* buffer;
    std::size_t length = 0;

    // Opening file in reading mode
    ptr = fopen(filename, "r");
    if (NULL == ptr) {
        printf("file can't be opened \n");
        return;
    }

    // get len of bufferif (f)
    fseek(ptr, 0, SEEK_END);
    length = ftell(ptr);
    fseek(ptr, 0, SEEK_SET);
    buffer = (char*)malloc(length);
    if (buffer) {
        fread(buffer, 1, length, ptr);
        benchmark::DoNotOptimize(*buffer);
        free(buffer);
    }
    fclose(ptr);
}
}  // end of extern

// Benchmark Generators
// For C++ code
template <typename Callable>
static void CPPBenchmark(benchmark::State& state, Callable&& func, const char* filename) {
    ifstream in_stream{filename};
    for (auto _ : state) {
        // measure the result !!!
        func(in_stream);
        in_stream.clear();
        in_stream.seekg(0);
    }
}

template <typename Callable>
static void CPPBenchmarkSS(benchmark::State& state, Callable&& func,
                           const char* filename) {
    ifstream in_stream{filename};
    std::ostringstream oss;
    oss << in_stream.rdbuf();
    in_stream.close();

    for (auto _ : state) {
        // measure the result !!!
        std::istringstream input{oss.str()};
        func(input);
        in_stream.clear();
        in_stream.seekg(0);
    }
}

#define RegisterCPP(name, func) \
    benchmark::RegisterBenchmark(name, CPPBenchmark<decltype(func)>, func, argv[1]);

#define RegisterCPPWithSS(name, func)                                                    \
    {                                                                                    \
        benchmark::RegisterBenchmark(name, CPPBenchmark<decltype(func)>, func, argv[1]); \
        benchmark::RegisterBenchmark(name " (ss)", CPPBenchmarkSS<decltype(func)>, func, \
                                     argv[1]);                                           \
    }

// and for C-style code
template <typename Callable>
static void CBenchmark(benchmark::State& state, Callable&& func, const char* filename) {
    for (auto _ : state) {
        func(filename);
    }
}
#define RegisterC(name, func) \
    benchmark::RegisterBenchmark(name, CBenchmark<decltype(func)>, func, argv[1]);

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    RegisterCPP("Getline", countlines_getline);
    RegisterCPP("Read", countlines_read);
    RegisterCPP("In Memory", countlines_memory);
    RegisterCPP("istreambuf_iterator", countlines_itr);
    RegisterC("C Getline", countlines_c_getline);
    RegisterC("C Char-by-Char", countlines_c_char);
    RegisterC("C in Memory", countlines_c_in_memory);

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
}
