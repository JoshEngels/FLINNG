#include "../src/Flinng.h"
#include "../src/LshFunctions.h"
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

template <typename T>
uint64_t checkValidAndGetNumPoints(pybind11::array_t<T> points,
                                   uint64_t data_dimension) {
  auto points_buf = points.request();

  if (points_buf.ndim != 2) {
    throw std::invalid_argument(
        "The input points must be a 2 dimensional Numpy array where each "
        "row is a single point.");
  }
  uint64_t num_points = (uint64_t)points_buf.shape[0];
  uint64_t point_dimension = (uint64_t)points_buf.shape[1];
  if ((data_dimension != 0 && point_dimension != data_dimension) ||
      num_points == 0) {
    throw std::invalid_argument("The rows (each point) must be of dimension " +
                                std::to_string(data_dimension) +
                                ", and there must be at least 1 row.");
  }

  return num_points;
}

class DenseFlinng32 {

public:
  DenseFlinng32(uint64_t num_rows, uint64_t cells_per_row,
                uint64_t data_dimension, uint64_t num_hash_tables,
                uint64_t hashes_per_table)
      : internal_flinng(num_rows, cells_per_row, num_hash_tables,
                        1 << hashes_per_table),
        num_hash_tables(num_hash_tables), hashes_per_table(hashes_per_table),
        data_dimension(data_dimension),
        rand_bits(num_hash_tables * hashes_per_table * data_dimension) {

    for (uint64_t i = 0; i < rand_bits.size(); i++) {
      rand_bits[i] = (rand() % 2) * 2 - 1; // 50% chance either 1 or -1
    }
  }

  // See
  // https://pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html?highlight=numpy#arrays
  // for explanation of why we do py::array::c_style and py::array::forcecase
  // Basically ensures array is in dense row major order
  void addPoints(pybind11::array_t<float, pybind11::array::c_style |
                                              pybind11::array::forcecast>
                     points) {

    checkValidAndGetNumPoints<float>(points, data_dimension);
    std::vector<uint64_t> hashes = getHashes(points);
    internal_flinng.addPoints(hashes);
  }

  void prepareForQueries() { internal_flinng.prepareForQueries(); }

  pybind11::array_t<uint64_t>
  query(pybind11::array_t<float,
                          pybind11::array::c_style | pybind11::array::forcecast>
            queries,
        uint32_t top_k) {

    uint64_t num_queries =
        checkValidAndGetNumPoints<float>(queries, data_dimension);
    std::vector<uint64_t> hashes = getHashes(queries);
    std::vector<uint64_t> results = internal_flinng.query(hashes, top_k);

    return pybind11::array_t<uint64_t>(
        std::vector<ptrdiff_t>{(int64_t)num_queries, top_k}, &results[0]);
  }

private:
  Flinng internal_flinng;
  const uint64_t num_hash_tables, hashes_per_table, data_dimension;
  std::vector<int8_t> rand_bits;

  std::vector<uint64_t>
  getHashes(pybind11::array_t<float, pybind11::array::c_style |
                                         pybind11::array::forcecast>
                points) {
    auto points_buf = points.request();
    uint64_t num_points = (uint64_t)points_buf.shape[0];
    float *points_ptr = (float *)points_buf.ptr;
    return parallel_srp(points_ptr, num_points, data_dimension,
                        rand_bits.data(), num_hash_tables, hashes_per_table);
  }
};

class SparseFlinng32 {

public:
  SparseFlinng32(uint64_t num_rows, uint64_t cells_per_row,
                 uint64_t num_hash_tables, uint64_t hashes_per_table,
                 uint64_t hash_range_pow)
      : internal_flinng(num_rows, cells_per_row, num_hash_tables,
                        1 << hash_range_pow),
        num_hash_tables(num_hash_tables), hashes_per_table(hashes_per_table),
        hash_range_pow(hash_range_pow), seed(rand()) {}

  // See
  // https://pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html?highlight=numpy#arrays
  // for explanation of why we do py::array::c_style and py::array::forcecase
  // Basically ensures array is in dense row major order
  void
  addPointsSameDim(pybind11::array_t<uint64_t, pybind11::array::c_style |
                                                   pybind11::array::forcecast>
                       points) {
    checkValidAndGetNumPoints<uint64_t>(points, 0);
    std::vector<uint64_t> hashes = getHashes(points);
    internal_flinng.addPoints(hashes);
  }

  void addPoints(std::vector<std::vector<uint64_t>> data) {
    std::vector<uint64_t> hashes = getHashes(data);
    internal_flinng.addPoints(hashes);
  }

  std::vector<uint64_t> hashPoints(std::vector<std::vector<uint64_t>> data) {
    return getHashes(data);
    // internal_flinng.addPoints(hashes);
  }

  void prepareForQueries() { internal_flinng.prepareForQueries(); }

  pybind11::array_t<uint64_t> query(std::vector<std::vector<uint64_t>> queries,
                                    uint64_t top_k) {
    std::vector<uint64_t> hashes = getHashes(queries);
    std::vector<uint64_t> results = internal_flinng.query(hashes, top_k);

    return pybind11::array_t<uint64_t>(
        std::vector<ptrdiff_t>{(int64_t)queries.size(), (int64_t)top_k},
        &results[0]);
  }

  pybind11::array_t<uint64_t>
  querySameDim(pybind11::array_t<uint64_t, pybind11::array::c_style |
                                               pybind11::array::forcecast>
                   queries,
               uint32_t top_k) {

    uint64_t num_queries = checkValidAndGetNumPoints<uint64_t>(queries, 0);
    std::vector<uint64_t> hashes = getHashes(queries);
    std::vector<uint64_t> results = internal_flinng.query(hashes, top_k);

    return pybind11::array_t<uint64_t>(
        std::vector<ptrdiff_t>{(int64_t)num_queries, top_k}, &results[0]);
  }

private:
  Flinng internal_flinng;
  const uint64_t num_hash_tables, hashes_per_table, hash_range_pow;
  const uint32_t seed;

  std::vector<uint64_t>
  getHashes(pybind11::array_t<uint64_t, pybind11::array::c_style |
                                            pybind11::array::forcecast>
                data) {
    auto points_buf = data.request();
    uint64_t num_points = (uint64_t)points_buf.shape[0];
    uint64_t *points_ptr = (uint64_t *)points_buf.ptr;
    uint64_t point_dimension = (uint64_t)points_buf.shape[1];
    return parallel_densified_minhash(points_ptr, num_points, point_dimension,
                                      num_hash_tables, hashes_per_table,
                                      hash_range_pow, seed);
  }

  std::vector<uint64_t> getHashes(std::vector<std::vector<uint64_t>> data) {
    return parallel_densified_minhash(data, num_hash_tables, hashes_per_table,
                                      hash_range_pow, seed);
  }
};

PYBIND11_MODULE(flinng, m) {
  pybind11::class_<DenseFlinng32>(m, "dense_32_bit")
      .def(pybind11::init<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>(),
           pybind11::arg("num_rows"), pybind11::arg("cells_per_row"),
           pybind11::arg("data_dimension"), pybind11::arg("num_hash_tables"),
           pybind11::arg("hashes_per_table"))
      .def("add_points", &DenseFlinng32::addPoints,
           pybind11::arg("data_points"))
      .def("prepare_for_queries", &DenseFlinng32::prepareForQueries)
      .def("query", &DenseFlinng32::query, pybind11::arg("query_points"),
           pybind11::arg("top_k"));

  pybind11::class_<SparseFlinng32>(m, "sparse_32_bit")
      .def(pybind11::init<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>(),
           pybind11::arg("num_rows"), pybind11::arg("cells_per_row"),
           pybind11::arg("num_hash_tables"), pybind11::arg("hashes_per_table"),
           pybind11::arg("hash_range_pow"))
      .def("add_points", &SparseFlinng32::addPoints,
           pybind11::arg("data_points"))
      .def("add_points", &SparseFlinng32::addPointsSameDim,
           pybind11::arg("data_points"))
      .def("prepare_for_queries", &SparseFlinng32::prepareForQueries)
      .def("query", &SparseFlinng32::query, pybind11::arg("query_points"),
           pybind11::arg("top_k"))
      .def("query", &SparseFlinng32::querySameDim,
           pybind11::arg("query_points"), pybind11::arg("top_k"));
}