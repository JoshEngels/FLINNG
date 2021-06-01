from get_data import get_all_size_data

compares = {"genomes": ("R1@100", 0.8), "proteomes": ("R1@100", 0.8), "promethion": ("R1@100", 0.8),\
						"url": ("R1@100", 0.5), "webspam": ("R1@100", 0.5), "yfcc": ("R1@1", 0.9)}
ns = {"genomes": 117219, "proteomes": 116373, "promethion": 3696341, "url": 2386130, "webspam": 340000, "yfcc": 96970001}

print("FLINNG:")
for dataset, results in get_all_size_data("flinng").items():
				measure, minimum = compares[dataset]
				min_size = float('inf')
				min_time = float('inf')
				for result in results:
								if result[0] == measure and result[2] > minimum:
												# Calculate Size
												print(result)
												pieces = [int(i) for i in result[-1][0].strip().split()[1:]]
												print(pieces[0], ns[dataset], pieces[-1])
												size = pieces[0] * ns[dataset] * 2 * pieces[-1]
												print(size)
												if pieces[0] * pieces[1] > 2 ** 16:
																size *= 2
												size += 4 * ns[dataset]
												print(size)
												min_size = min(min_size, size)

												# Calculate Time
												time = float(result[-1][1].strip().split()[-1][:-3])
												min_time = min(min_time, time)


				print(dataset, min_size / 10 ** 9, min_time / 1000)



print()
print("FLASH:")
for dataset, results in get_all_size_data("flash").items():
				measure, minimum = compares[dataset]
				min_size = float('inf')
				min_time = float('inf')
				for result in results:
								if result[0] == measure and result[2] > minimum:
												# Calculate Size
												pieces = [int(i) for i in result[-1][0].strip().split()[1:]]
												size = (2 ** pieces[1]) * pieces[0] * pieces[2] * 4
												min_size = min(min_size, size)

												# Calculate Time
												time = float(result[-1][1].strip().split()[-1][:-3])
												min_time = min(min_time, time)

				print(dataset, min_size / 10 ** 9, min_time / 1000)
