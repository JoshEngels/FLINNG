#include "LSHReservoirSampler.h"
#include "misc.h"
#include "indexing.h"
#include "FrequentItems.h"
#include <algorithm>
#include "omp.h"
#include <functional>
#include "benchmarking.h"

//#define DEBUG_TALLY

void LSHReservoirSampler::reservoir_sampling_cpu_openmp(unsigned int *allprobsHash, unsigned int *allprobsIdx,
	unsigned int *storelog, int numProbePerTb,  std::function<size_t(size_t, size_t)> indexFunc) {

	unsigned int counter, allocIdx, reservoirRandNum, TB, hashIdx, inputIdx, ct, reservoir_full, location;

#pragma omp parallel for private(TB, hashIdx, inputIdx, ct, allocIdx, counter, reservoir_full, reservoirRandNum, location)
	for (size_t probeIdx = 0; probeIdx < numProbePerTb; probeIdx++) {
		for (size_t tb = 0; tb < _numTables; tb++) {

			TB = numProbePerTb * tb;

			hashIdx = allprobsHash[indexFunc(tb, probeIdx)];
			inputIdx = indexFunc(tb, probeIdx) % NUMBASE;
			ct = 0;

			/* Allocate the reservoir if non-existent. */
			omp_set_lock(_tablePointersLock + tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b));
			allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)];
			if (allocIdx == TABLENULL) {
				allocIdx = _tableMemAllocator[tableMemAllocatorIdx(tb)];
				_tableMemAllocator[tableMemAllocatorIdx(tb)] ++;
				_tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)] = allocIdx;
			}
			omp_unset_lock(_tablePointersLock + tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b));

			// ATOMIC: Obtain the counter, and increment the counter. (Counter initialized to 0 automatically).
			// Counter counts from 0 to currentCount-1.
			omp_set_lock(_tableCountersLock + tableCountersLockIdx(tb, allocIdx, _aggNumReservoirs));

			counter = _tableMem[tableMemCtIdx(tb, allocIdx, _aggNumReservoirs)]; // Potentially overflowable.
			_tableMem[tableMemCtIdx(tb, allocIdx, _aggNumReservoirs)] ++;
			omp_unset_lock(_tableCountersLock + tableCountersLockIdx(tb, allocIdx, _aggNumReservoirs));

			// The counter here is the old counter. Current count is already counter + 1.
			// If current count is larger than _reservoirSize, current item needs to be sampled.
			//reservoir_full = (counter + 1) > _reservoirSize;

			reservoirRandNum = _global_rand[std::min((unsigned int)(_maxReservoirRand-1), counter)]; // Overflow prevention.

			if ((counter + 1) > _reservoirSize) { // Reservoir full.
				location = reservoirRandNum;
			}
			else {
				location = counter;
			}

			//location = reservoir_full * (reservoirRandNum)+(1 - reservoir_full) * counter;

			storelog[storelogIdIdx(numProbePerTb, probeIdx, tb)] = inputIdx;
			storelog[storelogCounterIdx(numProbePerTb, probeIdx, tb)] = counter;
			storelog[storelogLocationIdx(numProbePerTb, probeIdx, tb)] = location;
			storelog[storelogHashIdxIdx(numProbePerTb, probeIdx, tb)] = hashIdx;

		}
	}
}

void LSHReservoirSampler::add_table_cpu_openmp(unsigned int *storelog, int numProbePerTb) {


	unsigned int id, hashIdx, allocIdx;
	unsigned locCapped;
#pragma omp parallel for private(allocIdx, id, hashIdx, locCapped)
	for (int probeIdx = 0; probeIdx < numProbePerTb; probeIdx++) {
		for (unsigned int tb = 0; tb < _numTables; tb++) {

			id = storelog[storelogIdIdx(numProbePerTb, probeIdx, tb)];
			hashIdx = storelog[storelogHashIdxIdx(numProbePerTb, probeIdx, tb)];
			allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)];
			// If item_i spills out of the reservoir, it is capped to the dummy location at _reservoirSize.
			locCapped = storelog[storelogLocationIdx(numProbePerTb, probeIdx, tb)];

			if (locCapped < _reservoirSize) {
				_tableMem[tableMemResIdx(tb, allocIdx, _aggNumReservoirs) + locCapped] = id;
			}
		}
	}
}

void LSHReservoirSampler::mock_markdiff(unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSizePow2) {

	/* For each and every element. */
	for (int gIdx = 0; gIdx < numQueryEntries * segmentSizePow2; gIdx++) {

		/* Index inside each query segment, gIdx % segmentSizePow2. */
		unsigned int localQueueIdx = gIdx & _segmentSizeModulor;

		/* Record differences, except for the first element in the queue. */
		if (localQueueIdx != 0) {
			tallyCnt[gIdx] = (tally[gIdx] != tally[gIdx - 1]) ? gIdx : -1;
		}
		else { // The first element, no spot of comparison.
			tallyCnt[gIdx] = gIdx;
		}
	}
}

void LSHReservoirSampler::mock_agg(unsigned int *g_queryCt, unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSizePow2) {

	for (int i = 0; i < numQueryEntries; i++) {

		int counter = 0;	// To record the number of valid elements.
		int gIdx;			// Temporary global index.
		for (int k = 0; k < segmentSizePow2; k++) {
			gIdx = i * segmentSizePow2 + k;

			if (tallyCnt[gIdx] != -1) { // If difference marked.
				tallyCnt[i * segmentSizePow2 + counter] = tallyCnt[gIdx];
				tally[i * segmentSizePow2 + counter] = tally[gIdx];
				counter++;
			}
		}

		// Record the number of compacted elements.
		g_queryCt[i] = counter;

		// Mark all following as zeros.
		for (; counter < segmentSizePow2; counter++) {
			tallyCnt[i * segmentSizePow2 + counter] = 0;
			tally[i * segmentSizePow2 + counter] = 0;
		}
	}

}

void LSHReservoirSampler::mock_sub(unsigned int *g_queryCt, unsigned int *tallyCnt, unsigned int* tally, int numQueryEntries, int segmentSize, int segmentSizePow2) {

	for (int i = 0; i < numQueryEntries; i++) {
		for (int k = 0; k < segmentSizePow2; k++) {

			int gIdx = i * segmentSizePow2 + k;

			if (k < (g_queryCt[i] - 1)) { // If is in the valid range.
				tallyCnt[gIdx] = tallyCnt[gIdx + 1] - tallyCnt[gIdx];
			}
			else if (k != (g_queryCt[i] - 1)) { // At k >= queryCt[i], fill with zero to prevent interfering with sorting.
				tallyCnt[gIdx] = 0;
				tally[gIdx] = 0;
			}
			else { // At k == (g_queryCt[i] - 1).
				tallyCnt[gIdx] = (i + 1) * segmentSizePow2 - tallyCnt[gIdx]; // Very important - *(i + 1)
			}
		}
	}
}

void LSHReservoirSampler::query_extractRows_cpu_openmp(int numQueryEntries, int segmentSize, unsigned int *queue,
	unsigned int *hashIndices) {

	unsigned int hashIdx, allocIdx;
// #pragma omp parallel for private(hashIdx, allocIdx)
	for (int tb = 0; tb < _numTables; tb++) {
		for (int queryIdx = 0; queryIdx < numQueryEntries; queryIdx++) {
			for (int elemIdx = 0; elemIdx < _reservoirSize; elemIdx++) {

				for (unsigned int k = 0; k < _queryProbes; k++) {
					hashIdx = hashIndices[allprobsHashIdx(_queryProbes, numQueryEntries, tb, queryIdx, k)];
					allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)];
					if (allocIdx != TABLENULL) {
						queue[queueElemIdx(segmentSize, tb, queryIdx, k, elemIdx)] =
							_tableMem[tableMemResIdx(tb, allocIdx, _aggNumReservoirs) + elemIdx];
					}
				}

			}
		}
	}
}

void LSHReservoirSampler::query_frequentitem_cpu_openmp(int numQueryEntries, unsigned int *outputs,
	unsigned int *hashIndices, int topk) {

	unsigned int hashIdx, allocIdx;
#pragma omp parallel for private(hashIdx, allocIdx)
	for (int queryIdx = 0; queryIdx < numQueryEntries; queryIdx++) {

		FrequentItems * items = new FrequentItems(topk);
		for (int tb = 0; tb < _numTables; tb++) {
			for (int elemIdx = 0; elemIdx < _reservoirSize; elemIdx++) {
				for (unsigned int k = 0; k < _queryProbes; k++) {
					hashIdx = hashIndices[allprobsHashIdx(_queryProbes, numQueryEntries, tb, queryIdx, k)];
					allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, hashIdx, tb, _sechash_a, _sechash_b)];
					if (allocIdx != TABLENULL) {
						/* Instead of inserting into the queue, insert directly to the lossy counter. */
						items->increment(_tableMem[tableMemResIdx(tb, allocIdx, _aggNumReservoirs) + elemIdx]);
					}
				}
			}
		}
		items->getTopk(outputs + queryIdx * topk);
		delete items;
	}
}