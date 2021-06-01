
#ifndef EVALUATE_H
#define EVALUATE_H

#include <stdio.h>
#include <stdlib.h>
#include <unordered_set>
#include <map>
#include <algorithm>


void evaluate(
	unsigned int *queryOutputs,		// The output indices of queries.
	int numQueries,			// The number of query entries, should be the same for outputs and groundtruths.
	int topk,				// The topk per query contained in the queryOutputs.
	unsigned int *groundTruthIdx,	// The groundtruth indice vector.
	int availableTopk		// Available topk information in the groundtruth.
	);				

void rMetric(unsigned int *queryOutputs, int numQueries, int topk,
	unsigned int *groundTruthIdx, int availableTopk, int numerator);

#endif /* EVALUATE_H */
