#include "FLINNG.h"
#include "MurmurHash3.h"
#include <algorithm>
#include <bitset>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <math.h>
#include <set>
#include <sstream>
#include <string.h>
#include <string>
#include <unordered_set>
#include <vector>
#include "benchmarking.h"

using namespace std;

vector<uint> FLINNG::get_hashed_row_indices(uint index) {
  string key = to_string(index);
  uint key_length = to_string(index).size();
  vector<uint> hashvals(0);
  uint op;
  for (uint i = 0; i < row_count; i++) {
    MurmurHash3_x86_32(key.c_str(), key_length, i, &op); // seed is row number
    hashvals.push_back(op % blooms_per_row);
  }
  return hashvals;
}

FLINNG::FLINNG(uint row_count, uint blooms_per_row, uint *hashes, uint num_hashes_generated,
             LSH* hash_family, uint hash_bits, uint hash_repeats, uint num_points) {

  this->row_count = row_count;
  this->blooms_per_row = blooms_per_row;
  this->num_bins = blooms_per_row * row_count;
  this->hash_repeats = hash_repeats;
  this->num_points = num_points;
  this->hash_size = hash_size;
  this->hashes = hashes;
  this->num_hashes_generated = num_hashes_generated;
  this->internal_hash_bits = hash_bits;
  this->internal_hash_length = 1 << internal_hash_bits;
  #ifdef FLINNG32BIT
    this->rambo_array = new vector<uint32_t>[hash_repeats * internal_hash_length];
  #else
    this->rambo_array = new vector<uint16_t>[hash_repeats * internal_hash_length];
  #endif
  this->hash_function = hash_family;

  cout << "Getting row indices" << endl;
  vector<vector<uint>> *row_indices_arr = new vector<vector<uint>>(num_points);
#pragma omp parallel for
  for (uint i = 0; i < num_points; i++) {
    row_indices_arr->at(i) = get_hashed_row_indices(i);
  }

  cout << "Creating meta rambo" << endl;
  // Create meta rambo
  meta_rambo = new vector<uint>[row_count * blooms_per_row];
  for (uint point = 0; point < num_points; point++) {
    vector<uint> hashvals = row_indices_arr->at(point);
    for (uint r = 0; r < row_count; r++) {
      meta_rambo[hashvals.at(r) + blooms_per_row * r].push_back(point);
    }
  }

  cout << "Sorting meta rambo" << endl;
  // Sort array entries in meta rambo
#pragma omp parallel for  
  for (uint i = 0; i < num_bins; i++) {
    sort(meta_rambo[i].begin(), meta_rambo[i].end());
  }

  do_inserts();

  row_indices_arr->clear();
  delete row_indices_arr;
}

FLINNG::~FLINNG() {
  delete[] this->rambo_array;
  delete[] this->meta_rambo;
}

/**
 * Inserts a keys of a given index into the FLINNG array
 */
void FLINNG::do_inserts() {

  cout << "Getting row indices" << endl;
  vector<vector<uint>> *row_indices_arr = new vector<vector<uint>>(num_points);
#pragma omp parallel for
  for (uint i = 0; i < num_points; i++) {
    row_indices_arr->at(i) = get_hashed_row_indices(i);
  }

  cout << "Populating FLINNG" << endl;
#pragma omp parallel for
  for (uint rep = 0; rep < hash_repeats; rep++) {
    for (uint index = 0; index < num_points; index++) {
      vector<uint> row_indices = row_indices_arr->at(index);
      for (uint r = 0; r < row_count; r++) {
        uint b = row_indices.at(r);
        rambo_array[rep * internal_hash_length +
                    hashes[rep * num_points + index]]
            .push_back(r * blooms_per_row + b);
      }
    }
  }

  row_indices_arr->clear();
  delete row_indices_arr;
}

/**
 * Finishes FLINNG construction by sorting all buckets for fast access. All
 * points must be inserted at this point.
 */
void FLINNG::finalize_construction() {

  // Remove duplicates
  cout << "Sorting FLINNG" << endl;
#pragma omp parallel for
  for (uint i = 0; i < internal_hash_length * hash_repeats; i++) {
    sort(rambo_array[i].begin(), rambo_array[i].end());
    rambo_array[i].erase(unique(rambo_array[i].begin(), rambo_array[i].end()),
                         rambo_array[i].end());
  }
}

void FLINNG::query(int *data_ids, float *data_vals, int *data_marker,
                  uint query_goal, uint *query_output) {
  uint hashes[hash_repeats];
  uint indices[hash_repeats]; // Should be all one value

  // cout << data_vals[0] << endl;
  hash_function->getHash(hashes, indices, data_ids, data_vals, data_marker, 1,
                         1, 1, 0);

  // Get observations, ~80%!
  vector<uint> counts(num_bins, 0);
  for (uint rep = 0; rep < hash_repeats; rep++) {
    const uint index = internal_hash_length * rep + hashes[rep];
    const uint size = rambo_array[index].size();
    for (uint small_index = 0; small_index < size; small_index++) {
      // This single line takes 80% of the time, around half for the move and
      // half for the add
      ++counts[rambo_array[index][small_index]];
    }
  }

  vector<uint> sorted[hash_repeats + 1];
  uint size_guess = num_bins / (hash_repeats + 1);
  for (vector<uint> &v : sorted) {
    v.reserve(size_guess);
  }
  for (uint i = 0; i < num_bins; ++i) {
    sorted[counts[i]].push_back(i);
  }

  if (row_count > 2 || num_points < 4000000) {
    vector<uint8_t> num_counts(num_points, 0);
    uint num_found = 0;
    for (int rep = hash_repeats; rep >= 0; --rep) {
      for (uint bin : sorted[rep]) {
        for (uint point : meta_rambo[bin]) {
          if (++num_counts[point] == row_count) {
            query_output[num_found] = point;
            if (++num_found == query_goal) {
              // cout << "Using threshhold " << rep << endl;
              return;
            }
          }
        }
      }
    }
  }
  else {
    char* num_counts = (char*) calloc(num_points / 8 + 1, sizeof(char));
    uint num_found = 0;
    for (int rep = hash_repeats; rep >= 0; --rep) {
      for (uint bin : sorted[rep]) {
        for (uint point : meta_rambo[bin]) {
          if (num_counts[(point / 8)] & (1 << (point % 8))) {
            query_output[num_found] = point;
            if (++num_found == query_goal) {
              // cout << "Using threshhold " << rep << endl;
              free(num_counts);
              return;
            }
          } else {
            num_counts[(point / 8)] |= (1 << (point % 8));
          }
        }
      }
    }
  }
}
