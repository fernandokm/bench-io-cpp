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

int countlines_getline(std::istream& istr) {
    unsigned long long len_ctr = 0;
    unsigned long long newl_ctr = 0;
    string buffer;

    // get length of file to reserve space in the buffer
    istr.seekg(0, std::ios::end);
    buffer.reserve(istr.tellg());
    istr.seekg(0);

    while (std::getline(istr, buffer, '\n')) {
        len_ctr += buffer.size();
        newl_ctr += 1;
    }
    istr.clear();
    istr.seekg(0);
    return newl_ctr + len_ctr;
}

int countlines_read(std::istream& istr) {
    unsigned long long len_ctr = 0;
    unsigned long long newl_ctr = 0;
    std::ostringstream buf;
    buf << istr.rdbuf();
    string buffer{std::move(buf.str())};
    for (const auto& ch : buffer) {
        len_ctr += 1;
        newl_ctr += (ch == '\n');
    }
    return newl_ctr + len_ctr;
}

int countlines_memory(std::istream& istr) {
    string buffer;

    // get length of file to resize buffer
    istr.seekg(0, std::ios::end);
    buffer.resize(istr.tellg());
    istr.seekg(0);

    // read it into memory
    istr.read(buffer.data(), buffer.size());
    for (const char ch : buffer) {
        len_ctr += 1;
        newl_ctr += (ch == '\n');
    }
    return newl_ctr + len_ctr;
}

int countlines_itr(std::istream& istr) {
    return std::reduce(std::execution::par_unseq, istreambuf_iterator<char>(istr),
                       istreambuf_iterator<char>(), 0,
                       [](const char left, const char right) {
                           return (left == '\n') + (right == '\n');
                       });
}

extern "C" {
// Driver code
int countlines_c_getline(const char* filename) {
    FILE* ptr;
    unsigned long long ctr = 0;

    // Opening file in reading mode
    ptr = fopen(filename, "r");
    if (NULL == ptr) {
        printf("file can't be opened \n");
        return -1;
    }

    std::size_t len = MAXLEN;
    char* buffer = (char*)malloc(len * sizeof(char));

    // Printing what is written in file
    // character by character using loop.
    do {
        getline(&buffer, &len, ptr);
        ctr += strlen(buffer) + (buffer[len - 1] == '\n');
        // Checking if character is not EOF.
        // If it is EOF stop reading.
    } while (!feof(ptr));

    // Closing the file
    fclose(ptr);
    free(buffer);
    return ctr;
}

int countlines_c_char(const char* filename) {
    FILE* ptr;
    char ch;
    unsigned long long ctr = 0;

    // Opening file in reading mode
    ptr = fopen(filename, "r");
    if (NULL == ptr) {
        printf("file can't be opened \n");
        return -1;
    }

    // Printing what is written in file
    // character by character using loop.
    do {
        ch = fgetc(ptr);
        ctr += 1 + (ch == '\n');
        // Checking if character is not EOF.
        // If it is EOF stop reading.
    } while (!feof(ptr));

    // Closing the file
    fclose(ptr);
    return ctr;
}

int countlines_c_in_memory(const char* filename) {
    FILE* ptr;
    char* buffer;
    unsigned long long ctr = 0, length = 0;

    // Opening file in reading mode
    ptr = fopen(filename, "r");
    if (NULL == ptr) {
        printf("file can't be opened \n");
        return -1;
    }

    // get len of bufferif (f)
    fseek(ptr, 0, SEEK_END);
    length = ftell(ptr);
    fseek(ptr, 0, SEEK_SET);
    buffer = (char*)malloc(length);
    if (buffer) {
        fread(buffer, 1, length, ptr);
        for (int i = 0; i < length; i++) {
            ctr += (buffer[i] == '\n');
        }
        free(buffer);
    }
    fclose(ptr);

    ctr += length;
    return ctr;
}
}  // end of extern

// Benchmark Generators
// For C++ code
template <typename Callable>
static void CPPBenchmark(benchmark::State& state, Callable&& func, const char* filename) {
    ifstream in_stream{filename};
    for (auto _ : state) {
        // measure the result !!!
        benchmark::DoNotOptimize(func(in_stream));
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
        benchmark::DoNotOptimize(func(input));
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
    unsigned long long result = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(result = func(filename));
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
