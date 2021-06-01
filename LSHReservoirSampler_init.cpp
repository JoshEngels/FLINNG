
#include "indexing.h"
#include "misc.h"
#include "LSHReservoirSampler.h"

//#define PRINT_CLINFO

LSHReservoirSampler::LSHReservoirSampler(LSH *hashFamIn, unsigned int numHashPerFamily, unsigned int numHashFamilies,
	unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples,
	unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction) {

#if !defined SECONDARY_HASHING
	if (numHashPerFamily != numSecHash) {
		std::cout << "[LSHReservoirSampler::LSHReservoirSampler] Fatal, secondary hashing disabled. " << std::endl;
	}
#endif

	initVariables(numHashPerFamily, numHashFamilies, reservoirSize, dimension, numSecHash, maxSamples, queryProbes,
		hashingProbes, tableAllocFraction);

	_hashFamily = hashFamIn;

	initHelper(_numTables, _rangePow, _reservoirSize);
}


void LSHReservoirSampler::restart(LSH *hashFamIn, unsigned int numHashPerFamily, unsigned int numHashFamilies,
	unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples,
	unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction) {
	unInit();
	initVariables(numHashPerFamily, numHashFamilies, reservoirSize, dimension, numSecHash, maxSamples, queryProbes,
		hashingProbes, tableAllocFraction);
	_hashFamily = hashFamIn;
	initHelper(_numTables, _rangePow, _reservoirSize);
}

void LSHReservoirSampler::initVariables(unsigned int numHashPerFamily, unsigned int numHashFamilies,
	unsigned int reservoirSize, unsigned int dimension, unsigned int numSecHash, unsigned int maxSamples,
	unsigned int queryProbes, unsigned int hashingProbes, float tableAllocFraction) {
	_rangePow = numHashPerFamily;
	_numTables = numHashFamilies;
	_reservoirSize = reservoirSize;
	_dimension = dimension;
	_numSecHash = numSecHash;
	_maxSamples = maxSamples;
	_queryProbes = queryProbes;
	_hashingProbes = hashingProbes;
	_tableAllocFraction = tableAllocFraction;
	_segmentSizeModulor = numHashFamilies * reservoirSize * queryProbes - 1;
	_segmentSizeBitShiftDivisor = getLog2(_segmentSizeModulor);

	_numReservoirs = (unsigned int) pow(2, _rangePow);		// Number of rows in each hashTable.
	_numReservoirsHashed = (unsigned int) pow(2, _numSecHash);		// Number of rows in each hashTable.
	_aggNumReservoirs = (unsigned int) _numReservoirsHashed * _tableAllocFraction;
	_maxReservoirRand = (unsigned int) ceil(maxSamples / 10); // TBD.

	_zero = 0;
	_zerof = 0.0;
	_tableNull = TABLENULL;
}

void LSHReservoirSampler::initHelper(int numTablesIn, int numHashPerFamilyIn, int reservoriSizeIn) {

	/* Reservoir Random Number. */
	std::cout << "[LSHReservoirSampler::initHelper] Generating random number for reservoir sampling ..." << std::endl;
	std::default_random_engine generator1;
	std::uniform_int_distribution<unsigned int> distribution_a(0, 0x7FFFFFFF);
	_sechash_a = distribution_a(generator1) * 2 + 1;
	std::uniform_int_distribution<unsigned int> distribution_b(0, 0xFFFFFFFF >> _numSecHash);
	_sechash_b = distribution_b(generator1);

	_global_rand = new unsigned int[_maxReservoirRand];
	for (unsigned int i = 0; i < _maxReservoirRand; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, i);
		_global_rand[i] = distribution(generator1);
	}
	std::cout << "Completed. " << std::endl;

	/* Hash tables. */
	_tableMemReservoirMax = (_numTables - 1) * _aggNumReservoirs + _numReservoirsHashed;
	_tableMemMax = _tableMemReservoirMax * (1 + _reservoirSize);
	_tablePointerMax = _numTables * _numReservoirsHashed;

	std::cout << "Initializing CPU tables and pointers ... " << std::endl;
	_tableMem = new unsigned int[_tableMemMax]();
	_tableMemAllocator = new unsigned int[_numTables]();
	_tablePointers = new unsigned int[_tablePointerMax];
	_tablePointersLock = new omp_lock_t[_tablePointerMax];
	std::cout << "Completed. " << std::endl;
	std::cout << "Initializing CPU tablePointers/Locks ... " << std::endl;
	for (unsigned long long i = 0; i < _tablePointerMax; i++) {
		_tablePointers[i] = TABLENULL;
		omp_init_lock(_tablePointersLock + i);
	}
	std::cout << "Completed. " << std::endl;
	std::cout << "Initializing CPU tableCountersLocks ... " << std::endl;
	_tableCountersLock = new omp_lock_t[_tableMemReservoirMax];
	for (unsigned long long i = 0; i < _tableMemReservoirMax; i++) {
		omp_init_lock(_tableCountersLock + i);
	}
	std::cout << "Completed. " << std::endl;
}

LSHReservoirSampler::~LSHReservoirSampler() {

//	free(platforms); //For GPU??

	unInit();
}

void LSHReservoirSampler::unInit() {
	delete[] _tableMem;
	delete[] _tablePointers;
	delete[] _tableMemAllocator;
	for (unsigned long long i = 0; i < _tablePointerMax; i++) {
		omp_destroy_lock(_tablePointersLock + i);
	}
	for (unsigned long long i = 0; i < _tableMemReservoirMax; i++) {
		omp_destroy_lock(_tableCountersLock + i);
	}
	delete[] _tablePointersLock;
	delete[] _tableCountersLock;
	delete[] _global_rand;
}
