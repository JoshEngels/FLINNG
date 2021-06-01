#include "LSH.h"
#ifndef UINT_MAX
#define UINT_MAX 0xffffffff
#endif
#define RANDPROJGROUPSIZE 100

/* Constructor - Optimal Densified Minhash - Type 2. */
LSH::LSH(int hashType, int _K_in, int _L_in, int _rangePow_in)	{

	_hashType = hashType;

	_K = _K_in;
	_L = _L_in;
	_numTables = _L_in; // In densified minhash, _numTables is equivalan to _L. Initialized for general usage just in case.
	_rangePow = _rangePow_in;

	printf("<<< LSH Parameters >>>\n");
	std::cout << "_K " << _K << std::endl;
	std::cout << "_L " << _L << std::endl;
	std::cout << "_rangePow " << _rangePow_in << std::endl;
	std::cout << "_hashType " << _hashType << std::endl;

	rand1 = new int[_K * _L];

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned int> dis(1, UINT_MAX);

	// Generate rand1 - odd random numbers.
	for (int i = 0; i < _K * _L; i++)
	{
		rand1[i] = dis(gen);
		if (rand1[i] % 2 == 0)
			rand1[i]++;
	}

	_numhashes = _K * _L;
	_lognumhash = log2(_numhashes);
	std::cout << "_lognumhash " << _lognumhash << std::endl;

	// _randa and _randHash* are random odd numbers.
	_randa = dis(gen);
	if (_randa % 2 == 0)
		_randa++;
	_randHash = new int[2];
	_randHash[0] = dis(gen);
	if (_randHash[0] % 2 == 0)
		_randHash[0]++;
	_randHash[1] = dis(gen);
	if (_randHash[1] % 2 == 0)
		_randHash[1]++;
	std::cout << "Optimal Densified Hashing intialized ...  \n";
}

/* Constructor - SRP - Type 1 and Type 3. */
LSH::LSH(int hashType, int numHashPerFamily, int numHashFamilies, int dimension, int samFactor) {

	_rangePow = numHashPerFamily,
	_numTables = numHashFamilies;
	_dimension = dimension;
	_samSize = (int) floor(dimension / samFactor);;
	_samFactor = samFactor;
	_hashType = hashType;
	_groupHashingSize = RANDPROJGROUPSIZE;
	_numTablesToUse = _numTables;

	printf("<<< LSH Parameters >>>\n");
	std::cout << "_rangePow " << _rangePow << std::endl;
	std::cout << "_numTables " << _numTables << std::endl;
	std::cout << "_dimension " << _dimension << std::endl;
	std::cout << "_samSize " << _samSize << std::endl;
	std::cout << "_hashType " << _hashType << std::endl;

	/* Signed random projection. */
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::cout << "Generating random number of srp hashes of dense data ...  \n";
	// Random number generation for hashing purpose - declarations.
	// Random number generation for fast random projection.
	// Reference: Anshumali Shrivastava, Rice CS
	// randBits - random bits deciding to add or subtract, contain randbits for numTable * _rangePow * samSize.
	_randBits = new short[_numTables * _rangePow * _samSize];
	// indices - selected indices to perform subtraction. Dimension same as randbits.
	_indices = new int[_numTables * _rangePow * _samSize];

	int *a = new int[_dimension];
	for (int i = 0; i < _dimension; i++) {
		a[i] = i;
	}
	for (int tb = 0; tb < _numTables; tb++) {
		srand(time(0));
		for (int i = 0; i < _rangePow; i++) {
			std::random_shuffle(&a[0], &a[_dimension]);
			for (int j = 0; j < _samSize; j++) {
				_indices[tb * _rangePow * _samSize + i * _samSize + j] = a[j];
				// For 1/2 chance, assign random bit 1, or -1 to randBits.
				if (rand() % 2 == 0)
					_randBits[tb * _rangePow * _samSize + i * _samSize + j] = 1;
				else
					_randBits[tb * _rangePow * _samSize + i * _samSize + j] = -1;
			}
		}
	}
	delete[] a;

	std::cout << "Generating random number of universal hashes of sparse data ...  \n";
	_hash_a = new unsigned int[_rangePow * _numTables];
	_hash_b = new unsigned int[_rangePow * _numTables];
	_binhash_a = new unsigned int[_rangePow * _numTables];
	_binhash_b = new unsigned int[_rangePow * _numTables];

	std::default_random_engine generator0;
	for (int i = 0; i < _rangePow * _numTables; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, 0x7FFFFFFF);
		_binhash_a[i] = (distribution(generator0)) * 2 + 1;
	}
	for (int i = 0; i < _rangePow * _numTables; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, 0x7FFFFFFF);
		_binhash_b[i] = distribution(generator0);
	}
	for (int i = 0; i < _rangePow * _numTables; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, 0x7FFFFFFF);
		_hash_a[i] = (distribution(generator0)) * 2 + 1;
	}
	for (int i = 0; i < _rangePow * _numTables; i++) {
		std::uniform_int_distribution<unsigned int> distribution(0, 0xFFFFFFFF >> _samFactor);
		_hash_b[i] = distribution(generator0);
	}

}

LSH::~LSH() {

	switch (_hashType)
	{
	case 1:
		delete[] _binhash_a;
		delete[] _binhash_b;
		delete[] _hash_a;
		delete[] _hash_b;
		delete[] _randBits;
		delete[] _indices;
		break;
	case 2:
		delete[] _randHash;
		delete[] rand1;
		break;
	default:
		break;
	}

}
