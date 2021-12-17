#ifndef _LSH_FUNCTIONS
#define _LSH_FUNCTIONS

#include <climits>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

uint64_t combine(uint64_t item1, uint64_t item2) {
  return item1 * 0xC4DD05BF + item2 * 0x6C8702C9;
}

void single_densified_minhash(uint64_t *result, uint64_t *point,
                              uint64_t point_len, uint64_t num_tables,
                              uint64_t hashes_per_table, uint8_t hash_range_pow,
                              uint32_t random_seed) {

  uint64_t num_hashes_to_generate = num_tables * hashes_per_table;
  std::vector<uint64_t> prelim_result(num_hashes_to_generate);
  uint64_t binsize = std::ceil(UINT64_MAX / prelim_result.size());

  for (uint64_t i = 0; i < num_hashes_to_generate; i++) {
    prelim_result[i] = UINT64_MAX;
  }

  for (uint64_t i = 0; i < point_len; i++) {
    uint64_t val = point[i];
    val *= random_seed;
    val ^= val >> 13;
    val *= 0x192AF017AAFFF017;
    val *= val;
    uint64_t hash = val;
    uint64_t binid =
        std::min((uint64_t)floor(val / binsize), num_hashes_to_generate - 1);
    if (prelim_result[binid] > hash) {
      prelim_result[binid] = hash;
    }
  }

  // Densify
  for (size_t i = 0; i < num_hashes_to_generate; i++) {
    uint64_t next = prelim_result[i];
    if (next != UINT64_MAX) {
      continue;
    }
    uint64_t count = 0;
    while (next == UINT64_MAX) {
      count++;
      uint64_t index = combine(i, count) % num_hashes_to_generate;
      next = prelim_result[index]; // Kills GPU.
      if (count > 100) {           // Densification failure.
        next = 0;
        break;
      }
    }
    prelim_result[i] = next;
  }

  // Combine each K
  for (uint64_t table = 0; table < num_tables; table++) {
    result[table] = prelim_result[hashes_per_table * table];
    for (uint64_t hash = 1; hash < hashes_per_table; hash++) {
      result[table] =
          combine(prelim_result[hashes_per_table * table], result[table]);
    }
    result[table] >>= (64 - hash_range_pow);
  }
}

std::vector<uint64_t>
parallel_densified_minhash(uint64_t *points, uint64_t num_points,
                           uint64_t point_dimension, uint64_t num_tables,
                           uint64_t hashes_per_table, uint8_t hash_range_pow,
                           uint32_t random_seed) {

  std::vector<uint64_t> result(num_tables * num_points);

#pragma omp parallel for
  for (uint64_t point_id = 0; point_id < num_points; point_id += 1) {
    single_densified_minhash((&result[0]) + point_id * num_tables,
                             points + point_id * point_dimension,
                             point_dimension, num_tables, hashes_per_table,
                             hash_range_pow, random_seed);
  }

  return result;
}

std::vector<uint64_t>
parallel_densified_minhash(std::vector<std::vector<uint64_t>> points,
                           uint64_t num_tables, uint64_t hashes_per_table,
                           uint8_t hash_range_pow, uint32_t random_seed) {

  std::vector<uint64_t> result(num_tables * points.size());

#pragma omp parallel for
  for (uint64_t point_id = 0; point_id < points.size(); point_id += 1) {
    single_densified_minhash((&result[0]) + point_id * num_tables,
                             (&points[point_id][0]), points[point_id].size(),
                             num_tables, hashes_per_table, hash_range_pow,
                             random_seed);
  }

  return result;
}

std::vector<uint64_t> parallel_srp(float *dense_data, uint64_t num_points,
                                   uint64_t data_dimension, int8_t *random_bits,
                                   uint64_t num_tables,
                                   uint64_t hashes_per_table) {
  std::vector<uint64_t> result(num_tables * num_points);

#pragma omp parallel for
  for (uint64_t data_id = 0; data_id < num_points; data_id++) {
    for (uint64_t rep = 0; rep < num_tables; rep++) {
      uint64_t hash = 0;
      for (uint64_t bit = 0; bit < hashes_per_table; bit++) {
        double sum = 0;
        for (uint64_t j = 0; j < data_dimension; j++) {
          double val = dense_data[data_dimension * data_id + j];
          if (random_bits[rep * hashes_per_table * data_dimension +
                          bit * data_dimension + j] > 0) {
            sum += val;
          } else {
            sum -= val;
          }
        }
        hash += (sum > 0) << bit;
      }
      result[data_id * num_tables + rep] = hash;
    }
  }

  return result;
}

#endif