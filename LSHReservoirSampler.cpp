#include "LSHReservoirSampler.h"
#include "misc.h"
#include <functional>

void LSHReservoirSampler::add(int numInputEntries, unsigned int* allprobsHash, unsigned int* allprobsIdx,  std::function<size_t(size_t, size_t)> indexFunc) {

	const int numProbePerTb = numInputEntries * _hashingProbes;

	HashAddCPUTB(allprobsHash, allprobsIdx, numProbePerTb, numInputEntries, indexFunc);
}

void LSHReservoirSampler::lossy_ann(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, unsigned int* outputs, int k) {

	unsigned int* allprobsHash = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numQueryEntries * _queryProbes];

	_hashFamily->getHash(allprobsHash, allprobsIdx,
		dataIdx, dataVal, dataMarker, numQueryEntries, _queryProbes, numQueryEntries, 0);

	query_frequentitem_cpu_openmp(numQueryEntries, outputs, allprobsHash, k);

	delete[] allprobsHash;
	delete[] allprobsIdx;
}

void LSHReservoirSampler::ann(int numQueryEntries, int* dataIdx, float* dataVal, int* dataMarker, unsigned int* outputs, int topk) {

	if ((unsigned) topk > _reservoirSize * _numTables) {
		printf("Error: Maximum k exceeded! %d\n", topk);
		pause();
		return;
	}

	unsigned int* allprobsHash = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	unsigned int* allprobsIdx = new unsigned int[_numTables * numQueryEntries * _queryProbes];
	size_t segmentSize = (size_t)_numTables * (size_t)_queryProbes * (size_t)_reservoirSize;

	_hashFamily->getHash(allprobsHash, allprobsIdx,
		dataIdx, dataVal, dataMarker, numQueryEntries, _queryProbes, numQueryEntries, 0);

	unsigned int* tally = new unsigned int[(size_t)(numQueryEntries) * segmentSize];

	query_extractRows_cpu_openmp(numQueryEntries, segmentSize, tally, allprobsHash);

	kSelect(tally, outputs, segmentSize, numQueryEntries, topk);

	delete[] allprobsHash;
	delete[] allprobsIdx;
	delete[] tally;
}