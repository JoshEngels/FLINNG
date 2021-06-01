#include "HybridCNNReader.h"

int main(int argc, char** argv) {
  	HybridCNNReader reader = HybridCNNReader();
	  reader.convert_to_bin(argv[1], argv[2]);
}