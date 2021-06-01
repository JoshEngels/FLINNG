from get_data import get_data
import traceback

dataset = "yfcc"
goal = 0.4
recalls = []
topk = "R1"
max_time = 20
for method, data in get_data(dataset):
	try:
		file_name = "../" + method + "_" + dataset + ".txt"
		best_recall = 0
		
		for measure, time, recall, precision in data:
			if measure.split("@")[0] == topk and precision > goal and time < max_time:
				best_recall = max(best_recall, recall)

		if best_recall != 0:
			recalls.append(best_recall)
			print(method, best_recall)	
	except Exception as e:
		traceback.print_exc() 
		pass

recalls.sort()
print(f"At {goal} precision with max time of {max_time}ms, the best method on {dataset} is {recalls[-1] / recalls[-2]} better on recall")