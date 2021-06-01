#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <random>
#include <algorithm>
#include <inttypes.h>
#include <math.h>
#include <functional>

#include "omp.h"
#include "LSH.h"

/* Outputs and verbose amount. */
//#define PROFILE_READ
#define DEBUGTB 3
#define DEBUGENTRIES 20

//#define DEBUG

#define NUM_FILES 2

/** LSHReservoirSampler Class.
	Providing hashtable data-structure and k-select algorithm.
	An LSH class instantiation is pre-required.
*/
class LSHReservoirSampler {
private:

	LSH *_hashFamily;
	unsigned int _rangePow, _numTables, _reservoirSize, _dimension, _numSecHash, _maxSamples,
		_maxReservoirRand, _queryProbes, _hashingProbes, _segmentSizeModulor, _segmentSizeBitShiftDivisor;
	float _tableAllocFraction;

	/* For OpenMP only, but declare anyways. */
	unsigned int* _tableMem;
	unsigned int* _tableMemAllocator; // Special value MAX - 1.
	unsigned int* _tablePointers;
	omp_lock_t* _tablePointersLock;
	omp_lock_t* _tableCountersLock;

	unsigned int *_global_rand;
	unsigned int _numReservoirs, _numReservoirsHashed, _aggNumReservoirs;
	unsigned long long _tableMemMax, _tableMemReservoirMax, _tablePointerMax;
	float _zerof;
	unsigned int _sechash_a, _sechash_b, _tableNull, _zero;

	/* Init. */
	void initVariables(unsigned int numHashPerFamily, unsigned int numHashFamilies, unsigned int reservoirSize,
		unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples, unsigned int queryProbes,
		unsigned int hashingProbes, float tableAllocFraction);
	void initHelper(int numTablesIn, int numHashPerFamilyIn, int reservoriSizeIn);
	void unInit();

	/* Buildingblocks. */
	void reservoir_sampling_cpu_openmp(unsigned int *allprobsHash, unsigned int *allprobsIdx, unsigned int *storelog, int numProbePerTb,   std::function<size_t(size_t, size_t)> indexFunc);
	void add_table_cpu_openmp(unsigned int *storelog, int numProbePerTb);
	void query_extractRows_cpu_openmp(int numQueryEntries, int segmentSizePow2, unsigned int *queue, unsigned int *hashIndices);
	void query_frequentitem_cpu_openmp(int numQueryEntries, unsigned int *outputs, unsigned int *hashIndices, int topk);

	/* Routines. */
	void HashAddCPUTB(unsigned int *allprobsHash, unsigned int* allprobsIdx, int numProbePerTb, int numInputEntries,   std::function<size_t(size_t, size_t)> indexFunc);
	void kSelect(unsigned int *tally, unsigned int *outputs, int segmentSize, int numQueryEntries, int topk);

	/* Aux. */
	void pause();

	/* Debug. */
	void mock_markdiff(unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSizePow2);
	void mock_agg(unsigned int *g_queryCt, unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSizePow2);
	void mock_sub(unsigned int *g_queryCt, unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSize, int segmentSizePow2);
	void viewTables();
	int benchCounting(int segmentSize, int* dataIdx, float* dataVal, int* dataMarker, float *timings);

	/* Experimental. */
	void lossy_ann(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, unsigned int* outputs, int k);

public:
	void restart(LSH *hashFamIn, unsigned int numHashPerFamily, unsigned int numHashFamilies,
		unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples,
		unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction);

	/** Constructor.
	Creates an instance of LSHReservoirSampler.
	@param hashFam An LSH class, a family of hash functions.
	@param numHashPerFamily Number of hashes (bits) per hash table, have to be the same as that of the hashFam.
	@param numHashFamilies Number of hash families (tables), have to be the same as that of the hashFam.
	@param reservoirSize Size of each hash rows (reservoir).
	@param dimension For dense vectors, this is the dimensionality of each vector.
		For sparse format data, this number is not used. (TBD)
	@param numSecHash The number of secondary hash bits. A secondary (universal) hashing is used to shrink the
		original range of the LSH for better table occupancy. Only a number <= numHashPerFamily makes sense.
	@param maxSamples The maximum number incoming data points to be hashed and added.
	@param queryProbes Number of probes per query per table.
	@param hashingProbes Number of probes per data point per table.
	@param tableAllocFraction Fraction of reservoirs to allocate for each table, will share with other table if overflows.
	*/
	LSHReservoirSampler(LSH *hashFam, unsigned int numHashPerFamily, unsigned int numHashFamilies,
		unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples,
		unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction);

	/** Adds input vectors (in sparse format) to the hash table.
	Each vector is assigned ascending identification starting 0.
	For numInputEntries > 1, simply concatenate data vectors.

	@param numInputEntries Number of input vectors.
	@param dataIdx Non-zero indice of the sparse format.
	@param dataVal Non-zero values of the sparse format.
	@param dataMarker Marks the start index of each vector in dataIdx and dataVal.
		Have an additional marker at the end to mark the (end+1) index.
	*/
	void add(int numInputEntries, unsigned int* allprobsHash, unsigned int* allprobsIdx,  std::function<size_t(size_t, size_t)> indexFunc);

	/** Query vectors (in sparse format) and return top k neighbors for each.
	Near-neighbors for each query will be returned in descending similarity.
	For numQueryEntries > 1, simply concatenate data vectors.

	@param numQueryEntries Number of query vectors.
	@param dataIdx Non-zero indice of the sparse format.
	@param dataVal Non-zero values of the sparse format.
	@param dataMarker Marks the start index of each vector in dataIdx and dataVal.
		Have an additional marker at the end to mark the (end+1) index.
	@param outputs Near-neighbor identifications. The i_th neighbor of the q_th query is outputs[q * k + i]
	@param k number of near-neighbors to query for each query vector.
	*/
	void ann(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, unsigned int* outputs, int k);

	/** Print current parameter settings to the console.
	*/
	void showParams();

	/** Check the memory load of the hash table.
	*/
	void checkTableMemLoad();

	/** Destructor. Frees memory allocations and OpenCL environments.
	*/
	~LSHReservoirSampler();
};
