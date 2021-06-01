
#ifndef _FLING_
#define _FLING_
#include "LSH.h"
#include <bitset>
#include <set>
#include <string>
#include <vector>
#include "benchmarking.h"

class FLINNG {

private:
  std::vector<uint> get_hashed_row_indices(uint index);

  uint row_count;
  uint blooms_per_row;
  uint num_bins;
  uint hash_repeats;
  uint num_points;
  uint internal_hash_length;
  uint internal_hash_bits;
  uint hash_size;
#ifdef FLINNG32BIT
  std::vector<uint32_t> *rambo_array;
#else
  std::vector<uint16_t> *rambo_array;
#endif
  std::vector<uint> *meta_rambo;
  uint *hashes;
  uint num_hashes_generated;
  uint *sorted;
  LSH* hash_function;

public:
  FLINNG(uint row_count, uint blooms_per_row, uint *hashes, uint num_hashes_generated, LSH* hash_function, uint hash_bits,
        uint hash_repeats, uint num_points);
  ~FLINNG();

  void do_inserts();
  void query(int *data_ids, float *data_vals, int *data_marker, uint query_goal, uint *query_output);  
  void finalize_construction();
  uint get_hash_index(uint i);
};

#endif
