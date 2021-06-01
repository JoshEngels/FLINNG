from get_data import get_data
import traceback

dataset = "genomes"
goal = 0.7
precisions = []
topk = "R1"
max_time = 2
for method, data in get_data(dataset):
	max_recall = 0
	try:
		best_precision = 0		
		for measure, time, recall, precision in data:
			if measure.split("@")[0] == topk and recall > goal and time < max_time:
				max_recall = max(recall, max_recall)
				best_precision = max(best_precision, precision)

		if best_precision != 0:
			precisions.append(best_precision)
			print(method, best_precision)	
			print(max_recall)
	except Exception as e:
		traceback.print_exc() 
		pass

precisions.sort()
print(f"At {goal} recall with max time of {max_time}ms, the best method on {dataset} is {precisions[-1] / precisions[-2]} better on precision")