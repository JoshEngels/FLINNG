# Synthetic sanity check for FLINNG that ensures near duplicate detection works well

import numpy as np
import random 
import flinng

data_dim = 100
dataset_size = 1000000
queries_size = 10000
dataset_std = 1
queries_std = 0.1

flinng_num_rows = 3
flinngs_cells_per_row = dataset_size // 100
flinng_hashes_per_table = 16
flinng_num_hash_tables = 20

# Generate n points using gaussian
np.random.seed(42)
random.seed(42)
dataset = np.random.normal(size=(dataset_size, data_dim), scale=dataset_std)

# Generate queries from random points
queries = []
gts = []
for i in range(queries_size):
  gt = random.randrange(dataset_size)
  query = dataset[gt] + np.random.normal(size=(data_dim), scale=queries_std)
  queries.append(query)
  gts.append(gt)
queries = np.array(queries)
 
index = flinng.dense_32_bit(num_rows=flinng_num_rows, 
                            cells_per_row=flinngs_cells_per_row, 
                            data_dimension=data_dim, 
                            num_hash_tables=flinng_num_hash_tables, 
                            hashes_per_table=flinng_hashes_per_table)
index.add_points(dataset)
index.prepare_for_queries()
results = index.query(queries, 1)
recall = sum([gt == result[0] for gt, result in zip(gts, results)]) / queries_size
print(f"R1@1 = {recall}")