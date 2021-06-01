#include <cmath>

#include "FLINNG.h"
#include "benchmarking.h"
#include "dataset.h"
#include "evaluate.h"
#include "indexing.h"
#include "omp.h"
#include "LSHReservoirSampler.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include "FrequentItems.h"
#include "dataset.h"
#include "benchmarking.h"
#include "HybridCNNReader.h"

#include <iostream>
using namespace std;

typedef chrono::high_resolution_clock Clock;

void runTrials(size_t b, size_t r, size_t reps, size_t range, uint *hashes,
              uint max_reps, unsigned int *gtruth_indice,
              int *query_sparse_indice, float *query_sparse_val, int *query_sparse_marker, 
              LSH *hash_family) {

  unsigned int *queryOutputs = new unsigned int[NUMQUERY * TOPK]();

  auto begin = Clock::now();
  float etime_0;

  // Create index
  FLINNG *flinng = new FLINNG(r, b, hashes, max_reps, hash_family, range, reps, NUMBASE);
  flinng->finalize_construction();


  auto end = Clock::now();
  etime_0 = (end - begin).count() / 1000000;
  cout << "Indexing took " << etime_0 << "ms." << endl;

  // Do queries
  cout << "Querying..." << endl;
  omp_set_num_threads(1);
  begin = Clock::now();
  for (uint i = 0; i < NUMQUERY; i++) {
    uint32_t *recall_buffer = new uint32_t[TOPK];
#ifdef YFCC
    flinng->query(query_sparse_indice, query_sparse_val + DIMENSION * i, query_sparse_marker, TOPK, recall_buffer);
#else
    flinng->query(query_sparse_indice, query_sparse_val, query_sparse_marker + i, TOPK, recall_buffer);
#endif
    for (size_t j = 0; j < TOPK; j++) {
      queryOutputs[TOPK * i + j] = recall_buffer[j];
    }
    delete[] recall_buffer;
  }
  end = Clock::now();
  omp_set_num_threads(80);

  etime_0 = (end - begin).count() / 1000000;
  std::cout << "Queried " << NUMQUERY << " datapoints, used " << etime_0
            << "ms. \n";

  evaluate(queryOutputs, NUMQUERY, TOPK, gtruth_indice, AVAILABLE_TOPK);

  delete flinng;
  delete[] queryOutputs;
}


void do_normal(size_t reservoir, size_t reps, size_t range, uint *hashes, uint *indices, 
              uint max_reps, unsigned int *gtruth_indice,
              int *query_sparse_indice, float *query_sparse_val, int *query_sparse_marker, 
              LSH *hashFamily) {

  unsigned int *queryOutputs = new unsigned int[NUMQUERY * TOPK]();

  auto begin = Clock::now();
  float etime_0;

  // Dimension not used so just pass in -1
  // Initialize hashtables and other datastructures.
  LSHReservoirSampler *myReservoir = new LSHReservoirSampler(
      hashFamily, RANGE, reps, reservoir, -1, range, NUMBASE, 1, 1, 1); 


  for (size_t start = 0; start < NUMBASE;) {
    size_t end = min(start + NUMBASE / NUMHASHBATCH, (size_t)NUMBASE);
    auto indexFunc = [start](size_t table, size_t probe) { 
      return (table * (size_t) (NUMBASE) + start + probe); 
    };
    myReservoir->add(end - start, hashes, indices, indexFunc);
    start = end;
  }

  auto end = Clock::now();
  etime_0 = (end - begin).count() / 1000000;
  cout << "Indexing took " << etime_0 << "ms." << endl;

  std::cout << "Querying..." << endl;
  omp_set_num_threads(1);
  begin = Clock::now();
  myReservoir->ann(NUMQUERY, query_sparse_indice, query_sparse_val, query_sparse_marker,
                   queryOutputs, TOPK);
  end = Clock::now();
  omp_set_num_threads(80);
  etime_0 = (end - begin).count() / 1000000;
  std::cout << "Queried " << NUMQUERY << " datapoints, used " << etime_0
            << "ms." << endl;
 
  evaluate(queryOutputs, NUMQUERY, TOPK, gtruth_indice, AVAILABLE_TOPK);

  delete myReservoir;
  delete[] queryOutputs;
}

void runBenchmark() {

  omp_set_num_threads(80);

  float etime_0, etime_1, etime_2;
  auto begin = Clock::now();
  auto end = Clock::now();

  std::cout << "Reading groundtruth and data ... " << std::endl;
  begin = Clock::now();

  // Read in data and queries
#ifdef YFCC
  int *sparse_data_indice, *sparse_data_marker, *sparse_query_indice, *sparse_query_marker;
  float *sparse_query_val = new float[(size_t)(NUMQUERY) * DIMENSION];
	fvecs_yfcc_read_queries(QUERYFILE, DIMENSION, NUMQUERY, sparse_query_val);

  LSH *hashFamily = new LSH(3, RANGE, MAXREPS, DIMENSION, sqrt(DIMENSION)); 
  unsigned int *all_hashes = new unsigned int [(size_t)NUMBASE * MAXREPS];
  unsigned int *indices_unused;

  size_t chunk_size = 1000000; // For now needs to be multiple of 1000000
  cout << NUMBASE << endl;
  for (size_t i = 0; i < (NUMBASE + chunk_size - 1) / chunk_size; i++) {
    size_t num_vectors = min(chunk_size, NUMBASE - i * chunk_size);
    cout << "Starting chunk " << i << ", contains " << num_vectors << " vectors." << endl;
    float *sparse_data_val_chunk = new float[(size_t)num_vectors * (size_t)DIMENSION];
    fvecs_yfcc_read_data(BASEFILE, i, num_vectors, sparse_data_val_chunk);
    hashFamily->getHash(all_hashes, indices_unused, 
                    sparse_data_indice, sparse_data_val_chunk, sparse_data_marker, 
                    num_vectors, 1, NUMBASE, i * chunk_size);
    delete[] sparse_data_val_chunk;
  }
#else
  int *sparse_data_indice;
  float *sparse_data_val;
  int *sparse_data_marker;
  int *sparse_query_indice;
  float *sparse_query_val;
  int *sparse_query_marker;
  readDataAndQueries(BASEFILE, NUMQUERY, NUMBASE, 
                     &sparse_data_indice, &sparse_data_val, &sparse_data_marker,
                     &sparse_query_indice, &sparse_query_val, &sparse_query_marker);
#endif

  // Read in ground truth
  unsigned int *gtruth_indice = new unsigned int[NUMQUERY * AVAILABLE_TOPK];
  readGroundTruthInt(GTRUTHINDICE, NUMQUERY, AVAILABLE_TOPK, gtruth_indice);


  end = Clock::now();
  etime_0 = (end - begin).count() / 1000000;

  // Generate hashes with maximum reps
  cout << "Starting total hash generation" << endl;

  cout << "Starting index building and query grid parameter test" << endl;
  unsigned int *queryOutputs = new unsigned int[NUMQUERY * TOPK]();
  
  for (size_t reps = STARTREPS; reps <= MAXREPS; reps *= REPRATIO) { 

    std::cout << "Initializing data hashes, array size " << reps * NUMBASE << endl;
    // Initialize LSH hash.
    unsigned int *hashes;
    unsigned int *indices;
#ifdef YFCC
    hashes = all_hashes;
    hashFamily->set_reps(reps);
#else
    LSH *hashFamily = new LSH(2, K, reps, RANGE); 
    hashes = new unsigned int[reps * NUMBASE];
    indices = new unsigned int[reps * NUMBASE];
    hashFamily->getHash(hashes, indices, 
                        sparse_data_indice, sparse_data_val, sparse_data_marker, 
                        NUMBASE, 1, NUMBASE, 0);
#endif

    for (size_t r = STARTR; r < ENDR; r++) {
      for (size_t b = STARTB; b < ENDB; b *= BRATIO) {
        #ifdef FLINNG16BIT
          if (b*r > 1 << 16) {
            continue;
          }
        #endif
        std::cout << "STATS_GROUPS: " << r << " " << b << " " << RANGE << " "
                  << reps << std::endl;
        runTrials(b, r, reps, RANGE, hashes, reps, gtruth_indice,
                  sparse_query_indice, sparse_query_val,
                  sparse_query_marker, hashFamily);
      }
    }
    
#ifndef YFCC
      delete[] hashes;
      delete[] indices;
      delete hashFamily;
#endif
    }

  delete[] sparse_data_indice;
#ifndef YFCC
  delete[] sparse_data_val;
#endif
  delete[] sparse_data_marker;
  delete[] sparse_query_indice;
  delete[] sparse_query_val;
  delete[] sparse_query_marker;
  delete[] gtruth_indice;
  delete[] queryOutputs;
}

int main() {
	runBenchmark();
}