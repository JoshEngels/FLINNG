# Practical Near Neighbor Search via Group Testing

This repository is the official implementation of the FLINNG algorithm from the paper Practical Near Neighbor Search via Group Testing. It was originally cloned from the FLASH repository and uses that repository's hashing functions. The FLINNG algorithm itself is implemented in [FLINNG.cpp](FLINNG.cpp).

## Setup
This repository requires you to specify a dataset to run FLINNG on, as well as the hyperparameters to run FLINNG with, in the file [benchmarking.h](benchmarking.h). You can run with the default parameter values for a given dataset by uncommenting the corresponding dataset name at the top of the file. Once you have done so, to build FLINNG, simply run 
```setup
make
```
You will need to have a copy of the g++ compiler.
****
Dataset ground truth is already in the datasets folder. To get the datasets themselves, besides yfcc, you will need to run
```setup
get-data
```
NOTE: While the url and webspam datasets are available on lib-svm, have hosted the promethion, genome, and proteome datasets on google drive since getting them can be a process. To get these files, first follow the links in the comments in get-data and download them into the base directory before running get-data.

Getting yfcc is more complicated, especially since it is so large. Like the other datasets, the groundtruth is already in the data folder.

## Run FLINNG
To run FLINNG on the given hyperparameters and dataset, simply run
```setup
runme > flinng_<dataset_name>.txt
```
which will run the trials and save the result in a textfile with the name flinng_<dataset_name>.txt.

## Baseline methods

See the paper for more details on baseline methods we compared against. 
These baseline method implementations are not contained here, but we saved the output of those methods in similar format text files to the above for FLINNG, or in simple "RA@B time, recall, precision" format, in text files with the same naming convention: [method_name]_[algo_name].txt.

## Generate Results
 All results for our method and the baselines we compared against are contained in the results folder, and you can ready that for comparison by running
```saved
cp results/* .
```
To generate all graphs used in the paper, run
```analysis
cd analysis
./generate-graphs
```