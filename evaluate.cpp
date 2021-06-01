#include "evaluate.h"
#include "misc.h"

#include <iostream>

using namespace std;

/*
* Function:  evaluate
* --------------------
* Evaluate the results of a dataset using various metrics, prints the result
*
*  returns: nothing
*/
void evaluate (
	unsigned int *queryOutputs,		// The output indices of queries.
	int numQueries,			// The number of query entries, should be the same for outputs and groundtruths.
	int topk,				// The topk per query contained in the queryOutputs.
	unsigned int *groundTruthIdx,	// The groundtruth indice vector.
	int availableTopk		// Available topk information in the groundtruth.
	) {				// The number of n(s) interested.

	rMetric(queryOutputs, numQueries, topk, groundTruthIdx, availableTopk, 1);
	rMetric(queryOutputs, numQueries, topk, groundTruthIdx, availableTopk, 10);
	if (availableTopk >= 100) {
		rMetric(queryOutputs, numQueries, topk, groundTruthIdx, availableTopk, 100);
	} else {
		rMetric(queryOutputs, numQueries, topk, groundTruthIdx, availableTopk, availableTopk);
	}
	
}

void rMetric(unsigned int *queryOutputs, int numQueries, int topk,
	unsigned int *groundTruthIdx, int availableTopk, int numerator) {

	printf("Top 10 neighbours of first query\n");
	for (int i = 0; i < 10; i++) {
		printf("%d ", queryOutputs[i]);
	}
	printf("\n");
	printf("Top 10 neighbours of second query\n");
	for (int i = 0; i < 10; i++) {
		printf("%d ", queryOutputs[topk + i]);
	}
	printf("\n");
	printf("\nR10@k Average fraction of top %d nearest neighbors returned in k first results. \n", numerator);

	int *good_counts = new int[topk]();

	for (int i = 0; i < numQueries; i++) {
		unordered_set<unsigned int> topGtruths(groundTruthIdx + i * availableTopk, groundTruthIdx + i * availableTopk + numerator);
		for (int denominator = numerator; denominator <= topk; denominator++) {
			unordered_set<unsigned int> topOutputs(queryOutputs + i * topk, queryOutputs + i * topk + denominator);
			uint local_count = 0;
			for (const auto& elem : topGtruths) {
				if (topOutputs.find(elem) != topOutputs.end()) { // If elem is found in the intersection.
					local_count++;
					good_counts[denominator - numerator]++;
				}
			}
		}
	}

	for (int denominator = numerator; denominator <= topk; denominator++) {
		printf("R%d@%d = %1.3f \n", numerator, denominator, (float)good_counts[denominator - numerator] / numQueries / numerator);
	}
	printf("\n"); printf("\n");

	delete[] good_counts;

}

