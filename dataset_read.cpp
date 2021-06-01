#include "benchmarking.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "dataset.h"
#include "HybridCNNReader.h"
#include <math.h>       /* sqrt */



using namespace std;

bool load_vector_from_stream_int(std::istream &in, std::vector<size_t> &vec,
                                 size_t *first);
bool load_vector_from_stream_float(std::istream &in, std::vector<float> &vec,
                                   size_t *first);
void readSet(std::istream &data_file, uint size, int **sparse_indice, float **sparse_dist, int **sparse_marker);
																	
void fvecs_yfcc_read_data(const std::string& file_prefix, int offset, int readsize, float* start);   

void fvecs_yfcc_read_queries(const std::string& file_prefix, int offset, int readsize, float* start);                         

void readDataAndQueries(string baseFile, uint numQuery, uint numBase, 
                     int **sparse_data_indice, float **sparse_data_val, int **sparse_data_marker,
                     int **sparse_query_indice, float **sparse_query_val, int **sparse_query_marker) {
	ifstream data_file(baseFile);
#if defined(SETDATASET)
	readSet(data_file, numQuery, sparse_query_indice, sparse_query_val, sparse_query_marker);
	readSet(data_file, numBase, sparse_data_indice, sparse_data_val, sparse_data_marker);
#elif defined(SPARSEDATASET)
  *sparse_query_indice = new int[NUMQUERY * DIMENSION];
  *sparse_query_val = new float[NUMQUERY * DIMENSION];
  *sparse_query_marker = new int[NUMQUERY + 1];
	readSparse(baseFile, 0, numQuery, 
						 *sparse_query_indice, *sparse_query_val, *sparse_query_marker,
						 NUMQUERY * DIMENSION);

  *sparse_data_indice = new int[NUMBASE * DIMENSION];
  *sparse_data_val = new float[NUMBASE * DIMENSION];
  *sparse_data_marker = new int[NUMBASE + 1];
	readSparse(baseFile, numQuery, numBase, 
						 *sparse_data_indice, *sparse_data_val, *sparse_data_marker,
						 NUMBASE * DIMENSION);

#endif
	data_file.close();
}


void anshuReadSparse(string fileName, int *indices, int *markers,
                     unsigned int n, unsigned int bufferlen) {
  std::ifstream file(fileName);
  std::string str;

  int linenum = 0;
  vector<string> list;
  unsigned int totalLen = 0;
  while (getline(file, str)) {
    char *mystring = &str[0];
    char *pch;
    pch = strtok(mystring, " ");
    int track = 0;
    list.clear();
    while (pch != NULL) {
      if (track % 2 == 1)
        list.push_back(pch);
      track++;
      pch = strtok(NULL, " :");
    }

    markers[linenum] = totalLen;
    for (auto const &var : list) {
      indices[totalLen] = stoi(var);
      totalLen++;
    }
    linenum++;
    if (linenum == n) {
      break;
    }
    if (totalLen >= bufferlen) {
      std::cout << "Buffer too small!" << std::endl;
      markers[linenum] = totalLen; // Final length marker.
      file.close();
      return;
    }
  }
  markers[linenum] = totalLen; // Final length marker.
  file.close();

  std::cout << "[anshuReadSparse] Total " << totalLen << " number, " << linenum
            << " vectors. " << std::endl;
}

/** For reading sparse matrix dataset in index:value format.
        fileName - name in string
        offset - which datapoint to start reading, normally should be zero
        n - how many data points to read
        indices - array for storing indices
        values - array for storing values
        markers - the start position of each datapoint in indices / values. It
   have length(n + 1), the last position stores start position of the (n+1)th
   data point, which does not exist, but convenient for calculating the length
   of each vector.
*/

void readSparse(string fileName, int offset, int n, int *indices, float *values,
                int *markers, unsigned int bufferlen) {
  std::cout << "[readSparse]" << std::endl;

  /* Fill all the markers with the maximum index for the data, to prevent
     indexing outside of the range. */
  for (int i = 0; i <= n; i++) {
    markers[i] = bufferlen - 1;
  }

  std::ifstream file(fileName);
  std::string str;

  unsigned int ct = 0;            // Counting the input vectors.
  unsigned int totalLen = 0;      // Counting all the elements.
  while (std::getline(file, str)) // Get one vector (one vector per line).
  {
    if (ct < offset) { // If reading with an offset, skip < offset vectors.
      ct++;
      continue;
    }
    // Constructs an istringstream object iss with a copy of str as content.
    std::istringstream iss(str);
    // Removes label.
    std::string sub;
    iss >> sub;
    // Mark the start location.
    markers[ct - offset] = min(totalLen, bufferlen - 1);
    int pos;
    float val;
    int curLen = 0; // Counting elements of the current vector.
    do {
      std::string sub;
      iss >> sub;
      pos = sub.find_first_of(":");
      if (pos == string::npos) {
        continue;
      }
      val = stof(sub.substr(pos + 1, (str.length() - 1 - pos)));
      pos = stoi(sub.substr(0, pos));

      if (totalLen < bufferlen) {
        indices[totalLen] = pos;
        values[totalLen] = val;
      } else {
        std::cout << "[readSparse] Buffer is too small, data is truncated!\n";
        return;
      }
      curLen++;
      totalLen++;
    } while (iss);

    ct++;
    if (ct == (offset + n)) {
      break;
    }
  }
  markers[ct - offset] = totalLen; // Final length marker.
  std::cout << "[readSparse] Read " << totalLen << " numbers, " << ct - offset
            << " vectors. " << std::endl;
}

/* Functions for reading and parsing MNIST 60k/10k dataset.
        url: http://yann.lecun.com/exdb/mnist/
        Reference:
   https://compvisionlab.wordpress.com/2014/01/01/c-code-for-reading-mnist-data-set/
*/

/* Reversing the endianess of an integer. */
int reverseInt(int i) {
  unsigned char ch1, ch2, ch3, ch4;
  ch1 = i & 255;
  ch2 = (i >> 8) & 255;
  ch3 = (i >> 16) & 255;
  ch4 = (i >> 24) & 255;
  return ((int)ch1 << 24) + ((int)ch2 << 16) + ((int)ch3 << 8) + ch4;
}

void Read_MNIST(string fileName, int NumberOfImages, int SizeDataOfAnImage,
                float *data) {
  ifstream file(fileName, ios::binary);
  if (file.is_open()) {
    int magic_number = 0;
    int number_of_images = 0;
    int n_rows = 0;
    int n_cols = 0;
    file.read((char *)&magic_number, sizeof(magic_number));
    magic_number = reverseInt(magic_number);
    file.read((char *)&number_of_images, sizeof(number_of_images));
    number_of_images = reverseInt(number_of_images);
    file.read((char *)&n_rows, sizeof(n_rows));
    n_rows = reverseInt(n_rows);
    file.read((char *)&n_cols, sizeof(n_cols));
    n_cols = reverseInt(n_cols);
    for (int i = 0; i < number_of_images; ++i) {
      for (int r = 0; r < n_rows; ++r) {
        for (int c = 0; c < n_cols; ++c) {
          unsigned char temp = 0;
          file.read((char *)&temp, sizeof(temp));
          // arr[i][(n_rows*r) + c] = (double)temp;
          data[i * SizeDataOfAnImage + n_rows * r + c] = (float)temp;
        }
      }
    }
  }
}

// Reference: http://eric-yuan.me/cpp-read-mnist/
void Read_MNISTLabel(string fileName, int numToRead, int *labels) {
  ifstream file(fileName, ios::binary);
  if (file.is_open()) {
    int magic_number = 0;
    int number_of_images = 0;
    int n_rows = 0;
    int n_cols = 0;
    file.read((char *)&magic_number, sizeof(magic_number));
    magic_number = reverseInt(magic_number);
    file.read((char *)&number_of_images, sizeof(number_of_images));
    number_of_images = reverseInt(number_of_images);
    for (int i = 0; i < number_of_images; ++i) {
      unsigned char temp = 0;
      file.read((char *)&temp, sizeof(temp));
      labels[i] = (int)temp;
    }
  }
}

/* Functions for reading and parsing the SIFT dataset. */

void fvecs_read(const std::string &file, int offset, int readsize, float *out) {
  int dimension = 128;

  /* According to sift1b hosting website, 4+d bytes exists for each vector,
   * where d is the dimension. */

  std::ifstream myFile(file, std::ios::in | std::ios::binary);
  long pointer_offset = (long)offset * ((long)4 * dimension + 4);

  if (!myFile) {
    printf("Error opening file ... \n");
    return;
  }
  myFile.seekg(pointer_offset);

  float x[1];
  int ct = 0;
  while (!myFile.eof()) {
    // Dummy reads, 128 0 0 0 separates each vector.
    myFile.read((char *)x, 4);

    for (int d = 0; d < 128; d++) {
      if (!myFile.read((char *)x, 4)) {
        printf("Error reading file ... \n");
        return;
      }
      out[ct * dimension + d] = (float)*x;
    }

    ct++;
    if (ct == readsize) {
      break;
    }
  }
  myFile.close();
}
/*
Reads the SIFT1B dataset in bvecs format (dimension 128)
Offset and readsize is in unit of datapoints
"queries.bvecs"
*/
void bvecs_read(const std::string &file, int offset, int readsize, float *out) {

  int dimension = 128;

  /* According to sift1b hosting website, 4+d bytes exists for each vector,
   * where d is the dimension. */

  std::ifstream myFile(file, std::ios::in | std::ios::binary);
  long pointer_offset = (long)offset * ((long)dimension + 4);

  if (!myFile) {
    printf("Error opening file ... \n");
    return;
  }
  myFile.seekg(pointer_offset);

  unsigned char x[1];
  int ct = 0;
  while (!myFile.eof()) {
    // Dummy reads, 128 0 0 0 separates each vector.
    myFile.read((char *)x, 1);
    myFile.read((char *)x, 1);
    myFile.read((char *)x, 1);
    myFile.read((char *)x, 1);

    for (int d = 0; d < 128; d++) {
      if (!myFile.read((char *)x, 1)) {
        printf("Error reading file ... \n");
        return;
      }
      out[ct * dimension + d] = (float)*x;
    }

    ct++;
    if (ct == readsize) {
      break;
    }
  }
  myFile.close();
}

void compute_averages() {
  cout << "Computing averages" << endl;
  int batch = 1000;
  float* fvs = new float[DIMENSION * batch];
  long* ids  = new long[batch];        // not used

  BinaryReader reader(BASEFILE);
  size_t features_read = 0;
  double totals[DIMENSION + 1] = {};
  while (features_read < NUMBASE) {
        size_t read = reader.read(batch, fvs, DIMENSION*batch, ids, batch);
	if (read == 0) {
	  cout << features_read << endl;
	}
#pragma omp parallel for
        for (size_t d = 0; d < DIMENSION; d++) {
          for (size_t i = 0; i < read; i++) {
            totals[d] += fvs[i * DIMENSION + d];
          }
        }


#pragma omp parallel for
        for (size_t i = 0; i < read; i++) {
          double magnitude = 0;
          for (size_t d = 0; d < DIMENSION; d++) {
            float component = fvs[i * DIMENSION + d];
            magnitude += component * component;
          }
#pragma omp critical        
          totals[DIMENSION] = max(totals[DIMENSION], sqrt(magnitude));
        }
      	features_read += read;
  }

  for (size_t i = 0; i < DIMENSION; i++) {
    totals[i] /= NUMBASE;
  }

  float averages[DIMENSION + 1];
  for (uint i = 0; i <= DIMENSION; i++) {
    averages[i] = totals[i];
  }

  FILE * pFile;
  pFile = fopen (("averages" + to_string(NUMBASE) + ".bin").c_str(), "wb");
  fwrite(averages, sizeof(float), DIMENSION + 1, pFile);
  fclose(pFile);
}

void get_averages(float *buffer) {
  cout << "Getting averages" << endl;
  FILE *pFile = fopen (("averages" + to_string(NUMBASE) + ".bin").c_str(), "rb");
  if (!pFile) {
    compute_averages();
    get_averages(buffer);
    return;
  }
  fread(buffer, sizeof(float), DIMENSION + 1, pFile);
  fclose(pFile);
}

/* Functions for reading and parsing the YFCC100M dataset. */

void fvecs_yfcc_read_data(const std::string& file_prefix, int offset, int readsize, float* start) {
    int batch = 1000;
    size_t index = 0;

    float* fvs = new float[DIMENSION * batch];
    long* ids  = new long[batch];        // not used

    float averages[DIMENSION + 1];
    get_averages(averages);

    if (offset > 97 || offset < 0) {
        printf("offset needs to be a valid file number for YFCC100M... \n");
        exit(EXIT_FAILURE);
    }

    BinaryReader reader(offset, file_prefix);

    size_t features_read = 0;
    while (features_read < readsize) {
        size_t read = reader.read(batch, fvs, DIMENSION * batch, ids, batch);
#pragma omp parallel for
        for (size_t i = 0; i < read * DIMENSION; i++) {
          start[index + i] = (fvs[i] - averages[i % DIMENSION]) / averages[DIMENSION];
        }
        index += read * DIMENSION;
      	features_read += read;
    }

}

void fvecs_yfcc_read_queries(const std::string& file, int dim, int readsize, float* out) {
  ifstream in(file);
  string line;
  float averages[DIMENSION + 1];
  get_averages(averages);
  for (int line_num = 0; line_num < readsize; line_num++) {
    getline(in, line);
    stringstream ss(line);
    string buff;
    for (int d = 0; d < dim; d++){
      getline(ss, buff, ' ');
      out[line_num * dim + d] = (stof(buff) - averages[d]) / averages[DIMENSION];
    }
  }
  // partly courtesy of:
  // https://stackoverflow.com/questions/1894886/parsing-a-comma-delimited-stdstring

}


/*
For reading the indices of the groudtruths.
Each indice represent a datapoint in the same order as the base dataset.
Vector indexing:
The k_th neighbor of the q_th query is out[(q * availableTopK)]
file - filename
numQueries - the number of query data points
availableTopK - the topk groundtruth available for each vector
out - output vector
*/
void readGroundTruthInt(const std::string &file, int numQueries,
                        int availableTopK, unsigned int *out) {
  std::ifstream myFile(file, std::ios::in | std::ios::binary);

  if (!myFile) {
    printf("Error opening file ... \n");
    return;
  }

  char cNum[256];
  int ct = 0;
  while (myFile.good() && ct < availableTopK * numQueries) {
    myFile.good();
    myFile.getline(cNum, 256, ' ');
    out[ct] = atoi(cNum);
    ct++;
  }

  myFile.close();
}

/*
For reading the distances of the groudtruths.
Each distances represent the distance of the respective base vector in the
"indices" to the query. Vector indexing: The k_th neighbor's distance to the
q_th query is out[(q * availableTopK) + k] file - filename numQueries - the
number of query data points availableTopK - the topk groundtruth available for
each vector out - output vector
*/
void readGroundTruthFloat(const std::string &file, int numQueries,
                          int availableTopK, float *out) {
  std::ifstream myFile(file, std::ios::in | std::ios::binary);

  if (!myFile) {
    printf("Error opening file ... \n");
    return;
  }

  char cNum[256];
  int ct = 0;
  while (myFile.good() && ct < availableTopK * numQueries) {
    myFile.good();
    myFile.getline(cNum, 256, ' ');
    out[ct] = strtof(cNum, NULL);
    ct++;
  }

  myFile.close();
}

void readSet(std::istream &data_file, uint size, int **sparse_indice, float **sparse_dist, int **sparse_marker) {

  // Read in data
  vector<vector<size_t>> *data = new vector<vector<size_t>>(size);
  size_t array_length = 0;
	vector<size_t> row = vector<size_t>(0);
	size_t index;
	size_t i = 0;
	while (load_vector_from_stream_int(data_file, row, &index) && i < size) {
		data->at(i) = row;
		i++;
		array_length += row.size();
	}
  *sparse_indice = new int[array_length];
  *sparse_dist = new float[array_length];
  *sparse_marker = new int[size + 1];
  size_t current_position = 0;
  for (size_t i = 0; i < size; ++i) {
    (*sparse_marker)[i] = current_position;
    for (size_t j = 0; j < data->at(i).size(); ++j) {
      (*sparse_dist)[current_position] = 1;
      (*sparse_indice)[current_position] = (data->at(i).at(j) % (((size_t)1)<<31));
      ++current_position;
    }
  }
  (*sparse_marker)[size] = current_position;
  delete data;
}

bool load_vector_from_stream_int(std::istream &in, std::vector<size_t> &vec,
                                 size_t *first) {

  std::string line;
  std::getline(in, line);
  std::stringstream ss(line);
  std::string buff;
  vec.clear();

  size_t i = 0;
  bool ret = false;
  while (getline(ss, buff, ' ')) {
    ret = true;
    if (i != 0) {
      vec.push_back(stoul(buff));
    } else {
      *first = stoul(buff);
    }
    ++i;
  }
  return ret;
  // courtesy of:
  // https://stackoverflow.com/questions/1894886/parsing-a-comma-delimited-stdstring
}

bool load_vector_from_stream_float(std::istream &in, std::vector<float> &vec,
                                   size_t *first) {

  std::string line;
  std::getline(in, line);
  std::stringstream ss(line);
  std::string buff;
  vec.clear();

  size_t i = 0;
  bool ret = false;
  while (getline(ss, buff, ' ')) {
    ret = true;
    if (i != 0) {
      vec.push_back(stof(buff));
    } else {
      *first = stoul(buff);
    }
    ++i;
  }
  return ret;
  // courtesy of:
  // https://stackoverflow.com/questions/1894886/parsing-a-comma-delimited-stdstring
}
