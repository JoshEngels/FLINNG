num_queries = 10000
import traceback

def _read_hnsw(file_name):
	record = []
	with open(file_name, "r") as f:
		while True:
			line = f.readline()
			if not line:
				break
			values = line.split()[1].split(",")
			record.append((line.split()[0], float(values[0]), float(values[1]), float(values[2])))
	return record

def _read_flash(file_name):
	record = []
	with open(file_name, "r") as f:
		time = -1
		saved = ("", "")
		while True:
			line = f.readline()
			if line.startswith("STAT"):
				saved = (line, "")
			if line.startswith("Queried"):
				time = float(line.strip().split(" ")[-1][:-3])
			if line.startswith("Indexing"):
				saved = (saved[0], line)
			if line.startswith("R") and "k" not in line and "@" in line:
				record.append((line.split()[0], time / num_queries, float(line.split()[2]),  float(line.split()[2])/int(line.split()[0].split("@")[1])*int(line.split()[0].split("@")[0][1:]), saved))
			if line == "":
				break
	return record

def _get_raw_data(file_name, cut):   
	if "hnsw" in file_name or "falconn" in file_name or "inverted" in file_name or "faiss" in file_name or "groups" in file_name:
		return _read_hnsw(file_name)
	else:
		if cut:
			return [(a,b,c,d) for (a,b,c,d,e) in _read_flash(file_name)]
		else:
			return _read_flash(file_name)

methods = ["flinng", "falconn","hnsw", "faiss", "flash", "inverted", "groups"]
raw_colors = \
	"""
	#1f55ab
	#9955a7
	#d75f8e
	#f48073
	#f5ad65
	#111111
  #808080
	"""
raw_markers = [("s", 6.5), ("*", 7.5), ("^", 7.5), ("d", 8), ("o", 6.5), ("h", 12), ("+", 8)] 
colors = {methods[i]: raw_colors.split()[i] for i in range(len(methods))}
markers = {methods[i]:raw_markers[i][0] for i in range(len(methods))}
markersizes = {methods[i]:raw_markers[i][1] for i in range(len(methods))}

def get_data(dataset):
	data = []
	for method in methods:
		file_name = "../" + method + "_" + dataset + ".txt"
		try:
			data.append((method, _get_raw_data(file_name, True)))
		except:
			# print(method, "failed in get data on", dataset)
			# traceback.print_exc()
			pass
	return data

def get_data_colored(dataset):
	return [(method, data, colors[method], markers[method], markersizes[method]) for method, data in get_data(dataset)]


size_datasets = ["yfcc"]
def get_all_size_data(method):
	return {dataset: _get_raw_data("../" + method + "_" + dataset + "_index_info.txt", False) for dataset in size_datasets}

title_map = {"genomes": "RefSeqG", "proteomes": "RefSeqP", "url": "URL", "yfcc": "YFCC100M", "promethion": "PromethION"}
def get_dataset_title(dataset):
	if dataset in title_map:
		return title_map[dataset]
	return dataset.title()