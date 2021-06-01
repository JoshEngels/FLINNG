#include "LSHReservoirSampler.h"
#include "misc.h"
#include "indexing.h"

#include <functional>
#include <algorithm>

#define NAIVE_COUNTING
//#define SINGLETHREAD_COUNTING
//#define PRINT_TOPK

void LSHReservoirSampler::HashAddCPUTB(unsigned int *allprobsHash, unsigned int* allprobsIdx, int numProbePerTb, int numInputEntries,   std::function<size_t(size_t, size_t)> indexFunc) {

	unsigned int* storelog = new unsigned int[(size_t)(_numTables * 4) * (size_t)(numProbePerTb)]();

	reservoir_sampling_cpu_openmp(allprobsHash, allprobsIdx, storelog, numProbePerTb, indexFunc);
	add_table_cpu_openmp(storelog, numProbePerTb);
	delete[] storelog;
}

void LSHReservoirSampler::kSelect(unsigned int *tally, unsigned int *outputs, int segmentSize, int numQueryEntries, int topk) {
	// SegmentedSort.
// #pragma omp parallel for
	for (int i = 0; i < numQueryEntries; i++) {
		std::sort(tally + i * segmentSize, tally + i * segmentSize + segmentSize);
	}

	// Reduction.
	unsigned int *tallyCnt = new unsigned int[(size_t)(segmentSize) * (size_t)(numQueryEntries)]();

#if !defined SINGLETHREAD_COUNTING
// #pragma omp parallel for
#endif

	for (int i = 0; i < numQueryEntries; i++) {
		unsigned int *vec = tally + i * segmentSize;
		unsigned int *cntvec = tallyCnt + i * segmentSize;
		int prev = vec[0];
		int ct = 0;
		int counter = 0;
		for (int j = 1; j < segmentSize; j++) {
			counter++;
			if (prev != vec[j]) {
				vec[ct] = prev;
				cntvec[ct] = counter;
				prev = vec[j];
				counter = 0;
				ct++;
			}
		}
		//vec[ct] = prev;
		//cntvec[ct] = counter;
		//ct++;
		for (; ct < segmentSize; ct++) {
			vec[ct] = 0;
		}
	}
	// KV SegmentedSort.
// #pragma omp parallel for
	for (int i = 0; i < numQueryEntries; i++) {
		unsigned int *vec = tally + i * segmentSize;
		unsigned int *cntvec = tallyCnt + i * segmentSize;
		unsigned int *idx = new unsigned int[segmentSize];
		for (int j = 0; j < segmentSize; j++) {
			idx[j] = j;
		}
		std::sort(idx, idx + segmentSize,
			[&cntvec](unsigned int i1, unsigned int i2) { return cntvec[i1] > cntvec[i2]; });

		int ss;
		int ct = 0;
		if (vec[idx[0]] == 0) { // The first item is spurious.
			ss = 1;
		}
		else {
			ss = 0;
		}
		ct = 0;
		for (int k = ss; k < topk + ss; k++) {
			outputs[i * topk + ct] = vec[idx[k]];
			ct++;
		}
		delete[] idx;
	}
	delete[] tallyCnt;

}

void reverse_array(unsigned int *array, int arraylength)
{
	for (int i = 0; i < (arraylength / 2); i++) {
		float temporary = array[i];
		array[i] = array[(arraylength - 1) - i];
		array[(arraylength - 1) - i] = temporary;
	}
}

void segmentedReverse(int dir, int segmentSize, int numSegments, unsigned int *a) {
	for (int sidx = 0; sidx < numSegments; sidx++) {
		// Reverse the current segment?
		if (sidx % 2 == dir) {
			reverse_array(a + segmentSize * sidx, segmentSize);
		}
	}
}