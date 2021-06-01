#include "LSHReservoirSampler.h"
#include "indexing.h"
#include "misc.h"

void LSHReservoirSampler::checkTableMemLoad() {
	unsigned int maxx = 0;
	unsigned int minn = _numReservoirs;
	unsigned int tt = 0;
	for (unsigned int i = 0; i < _numTables; i++) {
		if (_tableMemAllocator[i] < minn) {
			minn = _tableMemAllocator[i];
		}
		if (_tableMemAllocator[i] > maxx) {
			maxx = _tableMemAllocator[i];
		}
		tt += _tableMemAllocator[i];
	}

	printf("Table Mem Usage ranges from %f to %f, average %f\n",
		((float)minn) / (float)_aggNumReservoirs,
		((float)maxx) / (float)_aggNumReservoirs,
		((float)tt) / (float)(_numTables * _aggNumReservoirs));

}

void LSHReservoirSampler::showParams() {
	printf("\n");
	printf("<<< LSHR Parameters >>>\n");
	std::cout << "_rangePow " << _rangePow << "\n";
	std::cout << "_rangePow_Rehashed " << _numSecHash << "\n";
	std::cout << "_numTables " << _numTables << "\n";
	std::cout << "_reservoirSize " << _reservoirSize << "\n";
	std::cout << "_queryProbes " << _queryProbes << "\n";
	std::cout << "_hashingProbes " << _hashingProbes << "\n";

	std::cout << "_dimension " << _dimension << "\n";
	std::cout << "_maxSamples " << _maxSamples << "\n";
	std::cout << "_tableAllocFraction " << _tableAllocFraction << "\n";
	std::cout << "_segmentSizeModulor " << _segmentSizeModulor << "\n";
	std::cout << "_segmentSizeBitShiftDivisor " << _segmentSizeBitShiftDivisor << "\n";
	std::cout << "_numReservoirs " << _numReservoirs << "\n";
	std::cout << "_numReservoirsHashed " << _numReservoirsHashed << "\n";
	std::cout << "_aggNumReservoirs " << _aggNumReservoirs << "\n";
	std::cout << "_maxReservoirRand " << _maxReservoirRand << "\n";
	printf("\n");
}

void LSHReservoirSampler::viewTables() {
	for (unsigned int which = 0; which < std::min(_numTables, (unsigned int) DEBUGTB); which++) {
		printf("\n");
		printf("<<< Table %d Content >>>\n", which);
		unsigned int maxResShow = 0;
		for (unsigned int t = 0; t < _numReservoirs; t++) {
			if (_tablePointers[tablePointersIdx(_numReservoirsHashed, t, which, _sechash_a, _sechash_b)] != _tableNull && maxResShow < DEBUGENTRIES) {
				unsigned int allocIdx = _tablePointers[tablePointersIdx(_numReservoirsHashed, t, which, _sechash_a, _sechash_b)];
				printf("Reservoir %d (%u): ", t, _tableMem[tableMemCtIdx(which, allocIdx, _aggNumReservoirs)]);
				for (unsigned int h = 0; h < std::min(_reservoirSize, (unsigned)DEBUGENTRIES); h++) {
					printf("%u ", _tableMem[tableMemResIdx(which, allocIdx, _aggNumReservoirs) + h]);
				}
				printf("\n");
				maxResShow++;
			}
		}
		printf("\n");
	}

	pause();
}

void LSHReservoirSampler::pause() {
#ifdef VISUAL_STUDIO
	system("pause");
#endif
}
