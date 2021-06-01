#pragma once
#include <iostream>

using namespace std;

void readDataAndQueries(string basefile, uint numQuery, uint numBase, 
                     int **sparse_data_indice, float **sparse_data_val, int **sparse_data_marker,
                     int **sparse_query_indice, float **sparse_query_val, int **sparse_query_marker);

void anshuReadSparse(string fileName, int *indices, int *markers, unsigned int n, unsigned int bufferlen);
void readSparse(string fileName, int offset, int n, int *indices, float *values, int *markers, unsigned int bufferlen);

void Read_MNIST(string fileName, int NumberOfImages, int DataOfAnImage, float *data);
void Read_MNISTLabel(string fileName, int numToRead, int *labels);

void fvecs_read(const std::string& file, int offset, int readsize, float *out);
void bvecs_read(const std::string& file, int offset, int readsize, float *out);

void readGroundTruthInt(const std::string& file, int numQueries, int availableTopK, unsigned int *out);
void readGroundTruthFloat(const std::string& file, int numQueries, int availableTopK, float *out);

void fvecs_yfcc_read_data(const std::string& file_prefix, int offset, int readsize, float* start);
void fvecs_yfcc_read_queries(const std::string& file, int dim, int readsize, float* out);