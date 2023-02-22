#include <benchmark/benchmark.h>
#include <execution>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <numeric>
#include <filesystem>
#include <streambuf>

// for C-style code 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using std::istreambuf_iterator; 
using std::string; 
using std::stringstream; 
using std::ifstream; 

size_t MAXLEN = 10000;

int countlines_getline (ifstream& istr){
    unsigned long long len_ctr = 0;
    unsigned long long newl_ctr = 0;
    string buffer; 
    buffer.reserve( std::filesystem::file_size("./test.txt") );
    while ( std::getline(istr, buffer, '\n') ){
        len_ctr += buffer.size(); 
        newl_ctr += 1; 
    }
    istr.clear(); 
    istr.seekg(0);         
    return newl_ctr + len_ctr;
}

int countlines_read( ifstream& istr){
    unsigned long long len_ctr = 0;
    unsigned long long newl_ctr = 0;
    std::ostringstream buf; 
    buf << istr.rdbuf();
    string buffer; buffer.reserve( std::filesystem::file_size("./test.txt") );
    buffer = buf.str(); 
    for (const auto& ch : buffer){
        len_ctr += 1; 
        newl_ctr += (ch == '\n');    
    }
    return newl_ctr + len_ctr; 
}

int countlines_memory(ifstream& istr){
    unsigned long long len_ctr = 0;
    unsigned long long newl_ctr = 0;
    string buffer;

    // get length of file to resize buffer
    istr.seekg(0, std::ios::end);
    buffer.resize(istr.tellg());
    istr.seekg(0);

    // read it into memory
    istr.read(buffer.data(), buffer.size());
    for ( const char ch : buffer ){
        len_ctr += 1; 
        newl_ctr += (ch == '\n');
    }
    return newl_ctr + len_ctr;

}


int countlines_itr( ifstream& istr ) {
    unsigned long long init = 0;
    return std::reduce(
            std::execution::par_unseq,
            istreambuf_iterator<char>(istr),
            istreambuf_iterator<char>(), 
            0, 
            [](const char left, const char right){ 
            return (left == '\n') + (right == '\n');
            });
}

extern "C"{
    // Driver code
    int countlines_c_getline(const char* filename)
    {
        FILE* ptr;
        char* buffer;
        buffer = (char*) malloc( MAXLEN * sizeof(char)); 
        unsigned long long ctr = 0;

        // Opening file in reading mode
        ptr = fopen(filename, "r");
        if (NULL == ptr) {
            printf("file can't be opened \n");
        }

        // Printing what is written in file
        // character by character using loop.
        do {
            getline(&buffer,&MAXLEN,ptr); 
            ctr += strlen(buffer) + (buffer[MAXLEN-1] == '\n');
            // Checking if character is not EOF.
            // If it is EOF stop reading.
        } while (!feof(ptr));

        // Closing the file
        fclose(ptr);
        free(buffer);
        return ctr;
    }
    
    int countlines_c_char(const char* filename)
    {
        FILE* ptr;
        char ch;
        unsigned long long ctr = 0;

        // Opening file in reading mode
        ptr = fopen(filename, "r");
        if (NULL == ptr) {
            printf("file can't be opened \n");
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

    int countlines_c_in_memory(const char* filename){
        FILE* ptr;
        char* buffer;
        unsigned long long ctr = 0, length = 0;

        // Opening file in reading mode
        ptr = fopen(filename, "r");
        if (NULL == ptr) {
            printf("file can't be opened \n");
        }
        
        //get len of bufferif (f)
        if (ptr) {
          fseek (ptr, 0, SEEK_END);
          length = ftell (ptr);
          fseek (ptr, 0, SEEK_SET);
          buffer = (char*) malloc (length);
          if (buffer) {
            fread (buffer, 1, length, ptr);
          }
          fclose (ptr);
        }
        
        for ( int i = 0; i < length; i++){
            ctr += (buffer[i] == '\n');
        }
        ctr += length;
        return ctr;

    }
}//end of extern


// Benchmark Generators
// For C++ code
template<typename Callable>
static void CPPBenchmark(benchmark::State &state, Callable&& func, const char* filename){
    ifstream in_stream {filename};
    for (auto _ : state){
        // measure the result !!!
        benchmark::DoNotOptimize( func(in_stream) ); 
        in_stream.clear(); 
        in_stream.seekg(0);
    }
}
#define RegisterCPP(name, func)           \
    benchmark::RegisterBenchmark(name, \
            CPPBenchmark<decltype(func)>, func, argv[1]);

// and for C-style code
template<typename Callable>
static void CBenchmark(benchmark::State& state, Callable&& func, const char* filename) {
    unsigned long long result = 0;
	for (auto _ : state) {
		benchmark::DoNotOptimize( 
                result = func(filename) 
        );
	}
}
#define RegisterC(name, func)           \
    benchmark::RegisterBenchmark(name, \
            CBenchmark<decltype(func)>, func, argv[1]);

int main(int argc, char** argv){
    std::ios::sync_with_stdio(false);
    RegisterCPP("Getline", countlines_getline);
    RegisterCPP("Read", countlines_read);
    RegisterCPP("In Memory", countlines_memory);
    RegisterCPP("istreambuf_iterator", countlines_itr);
    RegisterC("C Getline", countlines_c_getline);
    RegisterC("C Char-by-Char", countlines_c_char);
    RegisterC("C in Memory", countlines_c_char);

    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
}