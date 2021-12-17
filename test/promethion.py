# Real world example on a dataset FLINNG is good at.
# To use this script, download the promethion dataset form
# https://drive.google.com/file/d/1EIN8uUuy98oIqYfHadtc2KzOzRH_E1Cs/view?usp=sharing
# Then copy it to this folder and run this script. When running it again, you
# can changed saved_as_npy to true to use the created .npy file. 

import numpy as np
import flinng
import time

saved_as_npy = False
if not saved_as_npy:
  promethion = np.loadtxt("promethion-data")
  np.save("promethion-data", promethion)
else:
  promethion = np.load("promethion-data.npy")
gts = np.load("promethion-gt.npy")

index = flinng.sparse_32_bit(num_rows=2, cells_per_row=50000, num_hash_tables=100, hashes_per_table=2, hash_range_pow=18)
index.add_points(promethion[10001:]) # This 100001 isn't a bug, for some reason the generated gt skips point 10000
index.prepare_for_queries()
start = time.time()
results = index.query(promethion[:10000], 100)
r10at100 = sum([sum([gt[i] in res for i in range(10)]) for gt, res in zip(gts, results)]) / 10 / 10000
print(f"R10@100 = {r10at100}")