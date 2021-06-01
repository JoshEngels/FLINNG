#include <sstream>
#include <vector>
#include <iostream>
#include <assert.h>

#include "HybridCNNReader.h"

HybridCNNReader::HybridCNNReader(std::string filename)
{
    open(filename);
}

void HybridCNNReader::convert_to_bin(std::string filename, std::string output)
{
    open(filename);
    std::ofstream out(output, std::ios::binary);

    int batch = 10000;
    float* buff    = new float[batch * _dim];
    long*  buff_id = new long[batch];

    // TODO: this code assumes that batch size will be multiple of the
    // total number of fv in the input file.
    // This assumption is valid in all but the last file.
    while ( batch == read_with_id(batch, buff, batch * _dim, buff_id, batch)) {

        for (int i = 0; i < batch; ++i) {
            out.write((char*)&(buff_id[i]), sizeof(long)); // write id
            out.write((char*)&(buff[i*_dim]), _dim * sizeof(float)); // write fv
            /* code */
        }
    }
}

void HybridCNNReader::open(std::string filename)
{
    _ifs.open(filename);

    if (!_ifs)
        printf("problem opening file?\n");

    std::string str;

    std::getline(_ifs, str); // First line of file is empty
    std::getline(_ifs, str);

    std::stringstream ss(str);
    std::string str_aux;
    std::getline(ss, str_aux, '\t');
    // std::cout << str_aux << std::endl;
    std::getline(ss, str_aux, '\t');
    // std::cout << str_aux << std::endl;

    float aux;
    int counter = 0;
    while (ss >> aux) {
        ++counter;
    }

    assert(counter == _dim);

    _ifs.close();
    _ifs.open(filename);
    std::getline(_ifs, str); // First line of file is empty
}

int HybridCNNReader::read(int vectors,
                          float* buff, size_t buf_size)
{
    if (buf_size < vectors * _dim) {
        printf("not enough space in buffer\n");
    }

    char str[256];

    for (int i = 0; i < vectors; ++i) {

        if (_ifs.eof())
            return i;
        _ifs.getline(str, 256, '\t'); // read id
        _ifs.getline(str, 256, '\t'); // read hasg
        // printf("%s\n",str );

        for (int j = 0; j < _dim; ++j) {
            _ifs >> *(buff + i*_dim + j);
            // std::cout << *(buff+j) << " ";
        }
        // std::cout << std::endl;
    }

    return vectors;
}

int HybridCNNReader::read_with_id(int vectors,
                                  float* buff, size_t buf_size,
                                  long* id_buff, size_t id_buf_size)
{
    if (buf_size < vectors * _dim) {
        printf("not enough space in buffer for data\n");
    }

    if (id_buf_size < vectors) {
        printf("not enough space in buffer for ids\n");
    }

    char str[256];

    for (int i = 0; i < vectors; ++i) {

        if (_ifs.eof())
            return i;
        _ifs >> *(id_buff + i);       // read id
        _ifs.getline(str, 256, '\t'); // read tab
        _ifs.getline(str, 256, '\t'); // read hash
        // std::cout << *(id_buff+i) << " ";
        // printf("%s\n",str );

        for (int j = 0; j < _dim; ++j) {
            _ifs >> *(buff + i*_dim + j);
            // std::cout << *(buff+j) << " ";
        }
        // std::cout << std::endl;
    }

    return vectors;
}

void HybridCNNReader::write_bin(float* buff, size_t buf_size, std::string file)
{
    std::ofstream out(file, std::ios::binary);
    out.write((char*)buff, buf_size);
}

void HybridCNNReader::read_bin(float* buff, size_t buf_size, std::string file)
{
    std::ifstream in(file, std::ios::binary);
    in.read((char*)buff, buf_size);
}

BinaryReader::BinaryReader(std::string prefix):
    _prefix(prefix),
    _file_counter(0),
    _read_counter(0)
{
    std::string filename = _prefix + std::to_string(_file_counter) + ".bin";
    open(filename);
}

BinaryReader::BinaryReader(int file_num, std::string prefix):
    _prefix(prefix),
    _file_counter(file_num),
    _read_counter(0)
{
    std::string filename = _prefix + std::to_string(_file_counter) + ".bin";
    open(filename);
}

void BinaryReader::open(std::string filename)
{
    if (_ifs.is_open())
        _ifs.close();

    std::cout << "Opening " << filename << std::endl;
    _ifs.open(filename);

    if (!_ifs)
        printf("problem opening file: filename\n");
}

int BinaryReader::read(int vectors,
                       float* vbuff, size_t buf_size,
                       long* idbuff, size_t ids_buf_size)
{
    if (buf_size < vectors * _dim) {
        printf("not enough space in buffer features\n");
    }

    if (ids_buf_size < vectors ) {
        printf("not enough space in buffer ids\n");
    }

    char str[256];

    for (int i = 0; i < vectors; ++i) {

        if (_read_counter % int(1e6) == 0 && _read_counter != 0) {
            _file_counter++;
            std::string filename = _prefix + std::to_string(_file_counter) + ".bin";
            open(filename);
        }

        if (_ifs.eof()) {
            return i;
        }
        _ifs.read((char*)&(idbuff[i]), sizeof(long)); // read id
        _ifs.read((char*)&(vbuff[i*_dim]), sizeof(float) * _dim);

        _read_counter++;

    }

    return vectors;
}
