#include "LSH.h"
#include <assert.h>     /* assert */

#ifndef INT_MAX
#define INT_MAX 0xffffffff
#endif

void LSH::getOptimalMinhash(unsigned int *hashIndices, unsigned int *probeDataIdx, int *dataIdx, int *dataMarker, int numInputEntries, int numProbes) {
#if defined DEBUG
	std::cout << "[LSH::getOptimalMinhash]" << std::endl;
#endif

#ifndef DEBUG
#pragma omp parallel for
#endif
	for (int inputIdx = 0; inputIdx < numInputEntries; inputIdx++) {
#if defined DEBUG
		std::cout << "inputIdx " << inputIdx << std::endl;
#endif
		unsigned int *hashes = new unsigned int[_numhashes];
		int sizenonzeros = dataMarker[inputIdx + 1] - dataMarker[inputIdx];
#if defined DEBUG
		if (sizenonzeros > MAGIC_NUMBER || sizenonzeros < 0) {
			std::cout << "[LSH::getOptimalMinhash] sizenonzeros = " << sizenonzeros << " (" <<
				dataMarker[inputIdx] << "," << dataMarker[inputIdx + 1] << ")" << std::endl;
		}
		std::cout << "Entering optimalMinHash()" << std::endl;
#endif
		optimalMinHash(hashes, (unsigned int*)(dataIdx + dataMarker[inputIdx]), sizenonzeros);
#if defined DEBUG
		std::cout << "Exit optimalMinHash()" << std::endl;
#endif
		for (int tb = 0; tb < _L; tb++)
		{
			unsigned int index = 0;
			for (int k = 0; k < _K; k++)
			{
				unsigned int h = hashes[_K*tb + k];
				h *= rand1[_K * tb + k];
				h ^= h >> 13;
				h ^= rand1[_K * tb + k];
				index += h*hashes[_K * tb + k];
			}
			index = (index << 2) >> (32 - _rangePow);
#if defined DEBUG
			unsigned long long allHashesIdxVerify =
				hashIndicesOutputIdx(L, numProbes, numInputEntries, inputIdx, 0, tb);
			if (allHashesIdxVerify < 0 || allHashesIdxVerify > (numInputEntries * numProbes * L)) {
				std::cout << "AllHErr!" << allHashesIdxVerify << std::endl;
			}
#endif
			hashIndices[hashIndicesOutputIdx(L, numProbes, numInputEntries, inputIdx, 0, tb)] = index;
			probeDataIdx[hashIndicesOutputIdx(L, numProbes, numInputEntries, inputIdx, 0, tb)] = inputIdx;
			for (int k = 1; k < numProbes; k++) {
				hashIndices[hashIndicesOutputIdx(L, numProbes, numInputEntries, inputIdx, k, tb)] = index ^ (1 << (k - 1));
				probeDataIdx[hashIndicesOutputIdx(L, numProbes, numInputEntries, inputIdx, k, tb)] = inputIdx;
			}
		}
		delete[] hashes;
	}
#if defined DEBUG
	std::cout << "[LSH::getOptimalMinhash] Exit. " << std::endl;
#endif
}

// The range of hashes returned by getRandDoubleHash is 2^_lognumhash = numhash.
unsigned int LSH::getRandDoubleHash(int binid, int count) {
	unsigned int tohash = ((binid + 1) << 10) + count;
	return ((unsigned int)_randHash[0] * tohash << 3) >> (32 - _lognumhash); // _lognumhash needs to be ceiled.
}

void LSH::optimalMinHash(unsigned int *hashArray, unsigned int *nonZeros, int sizenonzeros) {
	/* This function computes the minhash and perform densification. */
	unsigned int *hashes = new unsigned int[_numhashes];

	unsigned int range = 1 << _rangePow;
	// binsize is the number of times the range is larger than the total number of hashes we need.
	unsigned int binsize = ceil(range / _numhashes);

	for (size_t i = 0; i < _numhashes; i++)
	{
		hashes[i] = INT_MAX;
	}

#if defined DEBUG
	std::cout << "[optimalMinHash] Minhashing. " << sizenonzeros << " nonzeros" <<
		". First " << nonZeros[0] << std::endl;
	if (sizenonzeros > MAGIC_NUMBER || sizenonzeros < 0) return;
#endif
	for (size_t i = 0; i < sizenonzeros; i++)
	{
		unsigned int h = nonZeros[i];
		h *= _randa;
		h ^= h >> 13;
		h *= 0x85ebca6b;
		unsigned int curhash = ((unsigned int)(((unsigned int)h*nonZeros[i]) << 5) >> (32 - _rangePow));
		unsigned int binid = std::min((unsigned int) floor(curhash / binsize), (unsigned int)(_numhashes - 1));
		if (hashes[binid] > curhash)
			hashes[binid] = curhash;
	}
#if defined DEBUG
	std::cout << "[optimalMinHash] Densification. " << std::endl;
#endif
	/* Densification of the hash. */
	for (size_t i = 0; i < _numhashes; i++)
	{
		unsigned int next = hashes[i];
		if (next != INT_MAX)
		{
			hashArray[i] = hashes[i];
			continue;
		}
		unsigned int count = 0;
		while (next == INT_MAX)
		{
			count++;
			unsigned int index = std::min(
				(unsigned)getRandDoubleHash((unsigned int)i, count),
				(unsigned)_numhashes);
			next = hashes[index]; // Kills GPU.
			if (count > 100) // Densification failure.
				break;
		}
		hashArray[i] = next;
	}

	delete[] hashes;
#if defined DEBUG
	std::cout << "[optimalMinHash] Exit. " << std::endl;
#endif
}


void LSH::srp_openmp_sparse(unsigned int *hashes, int *dataIdx, float *dataVal, int *dataMarker, int numInputEntries) {
	unsigned int a1, a2, b1, b2, sparseLen, indice, start, ok;
	float value;

// #pragma omp parallel private(sparseLen, indice, start, ok, a1, a2, b1, b2, value)
// #pragma	omp for
	for (int i = 0; i < numInputEntries; i++) {
		start = dataMarker[i];
		sparseLen = dataMarker[i + 1] - start;
		for (int tb = 0; tb < _numTables; tb++) {
			for (int hashIdx = 0; hashIdx < _rangePow; hashIdx++) {
				value = 0;
				a1 = _hash_a[tb * _rangePow + hashIdx];
				b1 = _hash_b[tb * _rangePow + hashIdx];
				a2 = _binhash_a[tb * _rangePow + hashIdx];
				b2 = _binhash_b[tb * _rangePow + hashIdx];

				for (unsigned int k = 0; k < sparseLen; k++) {
					indice = dataIdx[start + k]; // This step is TOO slow, especially when sparseLen is large.
					ok = BINARY_HASH(indice, a2, b2);
					value += (UNIVERSAL_HASH(indice, _samFactor, a1, b1) == 1) ? (ok ? dataVal[start + indice] : (-dataVal[start + indice])) : 0;
				}
				hashes[hashesOutputIdx(_rangePow, numInputEntries, i, tb, hashIdx)] = value > 0;
			}
		}
	}
}

void LSH::srp_openmp_dense_data(unsigned int *hashesToFill, float *dataVal, int numInputEntries, size_t maxNumEntries, size_t offset, uint numTablesToUse) {

#pragma omp parallel for
	for (size_t inputIdx = 0; inputIdx < numInputEntries; inputIdx++) {
   		for (size_t rep = 0; rep < numTablesToUse; rep++) {
			size_t hash = 0;
			for (size_t bit = 0; bit < _rangePow; bit++) {
				double s = 0;
				for (size_t j = 0; j < _samSize; j++) {
					size_t index = rep * _rangePow * _samSize + bit * _samSize + j;
					size_t location = _indices[index];
					double v = dataVal[_dimension * inputIdx + location];
					if (_randBits[index] >= 0) {
						s += v;
					} else {
						s -= v;
					}
				}
				hash += (s >= 0 ? 0 : 1) << bit;
			}
			hashesToFill[hashIndicesOutputIdx(L, 1, maxNumEntries, offset + inputIdx, 0, rep)] = hash;
		}
    	}
}
