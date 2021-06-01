import traceback
from get_data import get_data

num_queries = 10000
	
dataset = "yfcc"
goal = 0.99
compare_by = "R1@100"
times = []
for method, data in get_data(dataset):
	try:
		best_time = float("inf")
		
		for measure, time, recall, precision in data:
			if measure == compare_by and recall > goal:
				best_time = min(best_time, time)
		if best_time != float("inf"):
			times.append(best_time)
			print(method, best_time)
	except Exception as e:
		# traceback.print_exc() 
		pass

times.sort()
print(f"{goal} recall of {compare_by} on {dataset.title()} is {times[1] / times[0]} better")