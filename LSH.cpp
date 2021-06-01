#include "LSH.h"

/* OpenMP - Sparse. */
void LSH::getHash(unsigned int *hashIndices, unsigned int *probeDataIdx, int *dataIdx, float *dataVal, int *dataMarker, int numInputEntries, int numProbes, size_t maxNumEntries, size_t offset) {
#if defined DEBUG
	std::cout << "[LSH::getHash]" << std::endl;
#endif

	if (_hashType == 1) {
		unsigned int *hashes = new unsigned int[_numTables * numInputEntries * _rangePow];
		srp_openmp_sparse(hashes, dataIdx, dataVal, dataMarker, numInputEntries);
		getHashIdx(hashIndices, probeDataIdx, hashes, numInputEntries, numProbes);
		delete[] hashes;
	}
	else if (_hashType == 2) {
		// Doph on 
		getOptimalMinhash(hashIndices, probeDataIdx, dataIdx, dataMarker, numInputEntries, numProbes);
	}
	else if (_hashType == 3) {
		// Sparse srp hashes on dense data
		// Note indices are not generated
		srp_openmp_dense_data(hashIndices, dataVal, numInputEntries, maxNumEntries, offset, _numTablesToUse);
	}

#if defined DEBUG
	std::cout << "[LSH::getHash] Exit. " << std::endl;
#endif
}

void LSH::set_reps(uint reps) {
	if (_hashType != 3) {
		std::cout << "Set reps is not implemented for this hash type!" << std::endl;
	}
	if (reps > _numTables) {
		std::cout << "Too many reps, more than the max stored" << std::endl;
	}
	_numTablesToUse = reps;
}

/* OpenMP - Dense. */
void LSH::getHash(unsigned int *hashIndices, unsigned int *probeDataIdx, float *input, int numInputEntries, int numProbes) {

	switch (_hashType)
	{
	case 1:
		std::cout << "[LSH::getHash] No OpenMP implementation: Dense Data Signed Random Projection. " << std::endl;
		exit(1);
		break;
	case 2:
		break;
	default:
		break;
	}
}



/* Aux functions. */

void LSH::getHashIdx(unsigned int *hashIndices, unsigned int *hashes, int numInputEntries, int numProbes) {
	if (numProbes > _rangePow) {
		std::cout << "[LSH::getHashIdx] More probes than hashes. " << std::endl;
		exit(1);
	}
	unsigned int hashIdx;
// #pragma omp parallel private(hashIdx)
// #pragma omp parallel for
	for (int inputIdx = 0; inputIdx < numInputEntries; inputIdx ++) {
		for (int tb = 0; tb < _numTables; tb++) {
			hashIdx = 0;
			for (int k = 0; k < _rangePow; k++) {
				hashIdx |= (unsigned)hashes[hashesOutputIdx(_rangePow, numInputEntries, inputIdx, tb, k)] << k;
			}
			for (int k = 0; k < numProbes; k++) {
				hashIndices[hashIndicesOutputIdx(_numTables, numProbes, numInputEntries, inputIdx, k, tb)] =
					hashIdx ^ (1 << (k - 1));
			}
		}
	}
}

void LSH::getHashIdx(unsigned int *hashIndices, unsigned int *dataIdx, unsigned int *hashes, int numInputEntries, int numProbes) {
	if (numProbes > _rangePow) {
		std::cout << "[LSH::getHashIdx] More probes than hashes. " << std::endl;
		exit(1);
	}
	unsigned int hashIdx;
// #pragma omp parallel private(hashIdx)
// #pragma omp parallel for
	for (int inputIdx = 0; inputIdx < numInputEntries; inputIdx++) {
		for (int tb = 0; tb < _numTables; tb++) {
			hashIdx = 0;
			for (int k = 0; k < _rangePow; k++) {
				hashIdx |= (unsigned)hashes[hashesOutputIdx(_rangePow, numInputEntries, inputIdx, tb, k)] << k;
			}
			for (int k = 0; k < numProbes; k++) {
				hashIndices[hashIndicesOutputIdx(_numTables, numProbes, numInputEntries, inputIdx, k, tb)] =
					hashIdx ^ (1 << (k - 1));
				dataIdx[hashIndicesOutputIdx(_numTables, numProbes, numInputEntries, inputIdx, k, tb)] =
					inputIdx;
			}
		}
	}
}
