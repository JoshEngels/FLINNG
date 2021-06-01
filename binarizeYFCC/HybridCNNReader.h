#pragma once

#include <string>
#include <fstream>

class HybridCNNReader {

    std::ifstream _ifs;

    const uint32_t _dim = 4096;

public:
    HybridCNNReader(){}
    HybridCNNReader(std::string filename);

    void open(std::string filename);
    int read(int vectors, float* buff, size_t buf_size);
    int read_with_id(int vectors, float* buff, size_t buf_size,
                     long* ids, size_t id_buf_size);

    void convert_to_bin(std::string filename, std::string output);

    void write_bin(float* buff, size_t buf_size, std::string file);
    void read_bin(float* buff, size_t buf_size, std::string file);
};

class BinaryReader {

    std::ifstream _ifs;

    const uint32_t _dim = 4096;

    std::string _prefix;
    int _file_counter;
    long _read_counter;

public:
    BinaryReader(std::string prefix);
    BinaryReader(int file_num, std::string prefix);

    void open(std::string filename);
    int read(int vectors, float* buff, size_t buf_size,
                     long* ids, size_t id_buf_size);
};
