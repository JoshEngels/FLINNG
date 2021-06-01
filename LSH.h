#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <iostream>
#include <random>
#include <algorithm>
#include "omp.h"

//#define DEBUG
#define MAGIC_NUMBER 100 // For debugging purpose, ignore.

#define UNIVERSAL_HASH(x,M,a,b) ((unsigned) (a * x + b) >> (32 - M))
#define BINARY_HASH(x,a,b) ((unsigned) (a * x + b) >> 31)

// hashIndicesOutputIdx HAS to be the same as that of the LSHReservoirSampler class !!!
#define hashIndicesOutputIdx(numHashFamilies, numProbes, numInputs, dataIdx, probeIdx, tb) (unsigned long long)(numInputs * numProbes * tb + dataIdx * numProbes + probeIdx)
#define hashesOutputIdx(numHashPerFamily, numInputs, dataIdx, tb, hashInFamIdx) (unsigned long long)(tb * (numInputs * numHashPerFamily) + dataIdx * numHashPerFamily + hashInFamIdx)

class LSH {
private:

	/* Core parameters. */
	int _rangePow;

	// Here _rangePow always means the power2 range of the hash table.
	int _hashType;

	/* Signed random projection. */
	int _numTables;
	int _dimension, _samSize, _samFactor, _groupHashingSize;
	unsigned int *_binhash_a;
	unsigned int *_binhash_b;
	unsigned int *_hash_a;
	unsigned int *_hash_b;
	short *_randBits;
	int *_indices;
	unsigned int _numTablesToUse;

	/* Optimal Densified Minhash. */
	int *_randHash, _randa, _numhashes, _lognumhash, _K;
	int _L;
	int *rand1;

	/* Function definitions. */

	/* Signed random projection implementations. */
	void srp_openmp_sparse(unsigned int *hashes, int *dataIdx, float *dataVal, int *dataMarker, int numInputEntries);
	void srp_openmp_dense_data(unsigned int *hashesToFill, float *dataVal, int numInputEntries, size_t maxNumEntries, size_t offset, uint numTablesToUse);

	/* Optimal Densified Minhash */
	unsigned int getRandDoubleHash(int binid, int count);
	void optimalMinHash(unsigned int *hashArray, unsigned int *nonZeros, int sizenonzeros);
	void getOptimalMinhash(unsigned int *hashIndices, unsigned int *probeDataIdx, int *dataIdx, int *dataMarker, int numInputEntries, int numProbes);

	void getHashIdx(unsigned int *hashIndices, unsigned int *probeDataIdx, unsigned int *hashes, int numInputEntries, int numProbes);
	void getHashIdx(unsigned int *hashIndices, unsigned int *hashes, int numInputEntries, int numProbes);

public:

	void set_reps(uint reps);


	/** Obtain hash indice given the (sparse) input vector, using CPU.
	Hash indice refer to the corresponding "row number" in a hash table, in the form of unsigned integer.
	This function will only be valid when an CPU implementation exists for that type of hashing.
	The outputs indexing is defined as hashIndicesOutputIdx(numHashFamilies, numProbes, numInputs, inputIdx, probeIdx, tb) (unsigned)(numInputs * numProbes * tb + inputIdx * numProbes + probeIdx).
	@param hashIndices Hash indice for the batch of input vectors.
	@param identity Hash indice's corresponding identifications (sequential number starting 0) for this batch of inputs.
	@param dataIdx Non-zero indice of the sparse format.
	@param dataVal Non-zero values of the sparse format.
	@param dataMarker Marks the start index of each vector in dataIdx and dataVal.
	@param numInputEntries Number of input vectors.
	@param numProbes Number of probes per input.
	*/
	void getHash(unsigned int *hashIndices, unsigned int *identity,
		int *dataIdx, float *dataVal, int *dataMarker, int numInputEntries, int numProbes, size_t maxNumEntries, size_t offset);

	/** Obtain hash indice given the (dense) input vector, using OpenCL.
	Hash indice refer to the corresponding "row number" in a hash table, in the form of unsigned integer.
	This function will only be valid when an OpenCL implementation exists for that type of hashing,
	and when OpenCL is initialized (clLSH) for that hashing.
	The outputs indexing is defined as hashIndicesOutputIdx(numHashFamilies, numProbes, numInputs, inputIdx, probeIdx, tb) (unsigned)(numInputs * numProbes * tb + inputIdx * numProbes + probeIdx).
	@param hashIndices Hash indice for the batch of input vectors.
	@param identity Hash indice's corresponding identifications (sequential number starting 0) for this batch of inputs.
	@param input Input vectors concatenated.
	@param numInputEntries Number of input vectors.
	@param numProbes Number of probes per input.
	*/
	void getHash(unsigned int *hashIndices, unsigned int *identity, float *input, int numInputEntries, int numProbes);

	/** Constructor.
	Construct an LSH class for signed random projection.
	@param hashType For SRP, use 1.
	@param numHashPerFamily Number of hash (bits) per hashfamily (hash table).
	@param numHashFamilies Number of hash families (hash tables).
	@param dimension Dimensionality of input data.
	@param samFactor samFactor = dimension / samSize, have to be an integer.
	*/
	LSH(int hashType, int numHashPerFamily, int numHashFamilies, int dimension, int samFactor);

	/** Constructor.

	Construct an LSH class for optimal densified min-hash (for more details refer to Anshumali Shrivastava, anshu@rice.edu).
	This hashing scheme is for very sparse and high dimensional data stored in sparse format.

	@param hashType For optimal densified min-hash, use 2.
	*/
	LSH(int hashType, int _K_in, int _L_in, int _rangePow_in);

	~LSH();
};
