
# FLINNG

Filters to Identify Near-Neighbor Groups (FLINNG) is a near neighbor search algorithm outlined in the paper 
[Practical Near Neighbor Search via Group Testing](https://arxiv.org/pdf/2106.11565.pdf). 

This branch (the main branch) contains a moderately cleaned up version of FLINNG. To 
access the original research code, use the research branch. Only this branch will 
be actively updated.
## Features

- If using C++, header-only 
- If using Python, clean and simple bindings
- Incremental/streaming index construction
- Parallel index construction and querying

Note that some features of the research branch have yet to be ported over, and there are a few improvements
this branch might soon receive:
- Signed random projection and densified minhash performance optimization (sparsify SRP, improve DOPH)
- Index dumping to and from disk
- Addition of random seeds for reproducible experiments

## Installation

To install Python bindings, run
```
git clone --depth 1 https://github.com/JoshEngels/FLINNG
cd FLINNG
make
export PYTHONPATH=$(pwd)/build:$PYTHONPATH
```

You will need to have pybind11 installed with conda or pip. This has been tested
on an M1 Mac and on Windows 10 WSL with Ubuntu.

If you want to be able to use FLINNG without running the export command every time you
start a new terminal, add the export command to your .bashrc or another file that 
gets run on terminal startup.

To use the C++ headers, you just need to clone the repo and copy src/Flinng.h to your project. You 
can also copy src/LshFunctions.h to hash your data before passing into Flinng.h; see
pybind/pybind.cpp for a direct example of how this works.


## Usage/Examples

### Python
To use FLINNG we must first create a new index, either a dense or a sparse index. 
A dense index will use the cosine similarity for the similarity search and accept
points as dense vectors in R^n (2D numpy array), while a sparse index will use the Jaccard similarity
for the similarity search and accept points as sets of positive integers 
(2D numpy array if all sets are the same length, otherwise python list of lists ). 

Here are the steps to use the FLINNG Python API:

Create a dense or sparse index:
```python
dense_index = flinng.dense_32_bit(
                            num_rows, 
                            cells_per_row, 
                            data_dimension, 
                            num_hash_tables, 
                            hashes_per_table)
sparse_index = flinng.sparse_32_bit(
                            num_rows, 
                            cells_per_row, 
                            num_hash_tables, 
                            hashes_per_table,
                            hash_range_pow)
```

Add points to the index:
```python
index.add_points(dataset)
```

Prepare for querying:
```python
index.prepare_for_queries()
```

Query:
```python
results = index.query(queries, top_k)
```

test/dense_data.py contains a complete example for running FLINNG on a synthetic dense data,
expected result ~100% R1@1. 
test/promethion.py contains a complete example for running FLINNG on real sparse DNA 
data, expected result ~98% R10@100. Make sure to read the comment at the beginning of promethion.py to see
how to download the dataset.


### C++
Similar to the above, but example usage might look like:
```C++
auto flinng = Flinng(num_rows, cells_per_row, num_hashes, hash_range);
std::vector<uint64_t> dataHashes = getHashes(data); // You need to implement this yourself or use LshFunctions.h
flinng.addPoints(hashes);
flinng.prepareForQueries();
std::vector<uint64_t> queryHashes = getHashes(queries); // You need to implement this yourself or use LshFunctions.h
auto results = flinng.query(queryHashes, topK);
```


## Authors

Implementation by [Josh Engels](https://www.github.com/joshengels). 
FLINNG created in collaboration with [Ben Coleman](https://randorithms.com/about.html)
and [Anshumali Shrivastava](https://www.cs.rice.edu/~as143/).

Please feel free to contact josh.adam.engels@gmail.com with any questions.

## Contributing

Currently, contributions are limited to bug fixes and suggestions. 
For a bug fix, feel free to submit a PR or send an email. 

## Citations

If you found our work useful, please cite our work as follows:

```
@inproceedings{NEURIPS2021_5248e511,
 author = {Engels, Joshua and Coleman, Benjamin and Shrivastava, Anshumali},
 booktitle = {Advances in Neural Information Processing Systems},
 editor = {M. Ranzato and A. Beygelzimer and Y. Dauphin and P.S. Liang and J. Wortman Vaughan},
 pages = {9950--9962},
 publisher = {Curran Associates, Inc.},
 title = { Practical Near Neighbor Search via Group Testing},
 url = {https://proceedings.neurips.cc/paper_files/paper/2021/file/5248e5118c84beea359b6ea385393661-Paper.pdf},
 volume = {34},
 year = {2021}
}
```
