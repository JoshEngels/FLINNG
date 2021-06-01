import argparse
import matplotlib
import math
from get_data import get_data_colored, get_dataset_title
import traceback

# Instantiate the parser
parser = argparse.ArgumentParser(description='Compare FLASH and FLINNG')
parser = argparse.ArgumentParser()
parser.add_argument("dataset")
parser.add_argument("directory")
parser.add_argument('-save', action='store_true')
parser.add_argument('-cutoff', action='store_true')
parser.add_argument("compare_by")

args = parser.parse_args()
directory = args.directory
save = args.save
dataset = args.dataset
compare_by = args.compare_by
cutoff = args.cutoff

if save:
	matplotlib.use('agg')
import matplotlib.pyplot as plt

def get_pareto(record):
	record.sort()
	result = [record[0]]
	for i in range(1, len(record)):
		if result[-1][2] < record[i][2]:
			result.append(record[i])
	return result
	
titlefontsize = 22
axisfontsize = 18
labelfontsize = 12
ls = "--"

all_data = get_data_colored(dataset)
for method, data, c, mark, marksize in all_data:
		try:
			filtered = get_pareto([d for d in data if d[0] == compare_by and (not cutoff or d[2] > 0.8)])
			plt.plot([d[2] for d in filtered], [math.log10(1000 / d[1]) for d in filtered if d[1] != 0], color = c, linestyle = ls, marker = mark, label = method.upper(), alpha = 0.8, markersize=marksize)
		except:
			print(method, "failed in recall time on", dataset)
			# traceback.print_exc()
			pass
		
plt.legend(fontsize=labelfontsize, loc = 'center left')
plt.xlabel(compare_by, fontsize=axisfontsize)
plt.ylabel('Queries per second (log 10)', fontsize=axisfontsize)
plt.title(get_dataset_title(dataset), fontsize=titlefontsize)
if save:
	valid = compare_by.replace("@", "at")
	plt.savefig(f"{dataset}-{valid}.png", bbox_inches='tight')
else:
	plt.show()
