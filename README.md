# Practical Near Neighbor Search via Group Testing

This repository is the official implementation of the FLINNG algorithm from the paper Practical Near Neighbor Search via Group Testing. It was originally cloned from the FLASH repository and uses that repository's hashing functions. The FLINNG algorithm itself is implemented in [FLINNG.cpp](FLINNG.cpp).

## Setup
This repository requires you to specify a dataset to run FLINNG on, as well as the hyperparameters to run FLINNG with, in the file [benchmarking.h](benchmarking.h). You can run with the default parameter values for a given dataset by uncommenting the corresponding dataset name at the top of the file. Once you have done so, to build FLINNG, simply run 
```setup
make
```
You will need to have a copy of the g++ compiler.
****
Dataset ground truth is already in the datasets folder. To get the datasets themselves, besides yfcc, simply run
```setup
get-data
```
Getting yfcc is more complicated, especially since it is so large. Like the other datasets, the groundtruth is already in the data folder.

## Training

To train the model(s) in the paper, run this command:

```train
python train.py --input-data <path_to_data> --alpha 10 --beta 20
```

>📋  Describe how to train the models, with example commands on how to train the models in your paper, including the full training procedure and appropriate hyperparameters.

## Evaluation

To evaluate my model on ImageNet, run:

```eval
python eval.py --model-file mymodel.pth --benchmark imagenet
```

>📋  Describe how to evaluate the trained models on benchmarks reported in the paper, give commands that produce the results (section below).

## Pre-trained Models

You can download pretrained models here:

- [My awesome model](https://drive.google.com/mymodel.pth) trained on ImageNet using parameters x,y,z. 

>📋  Give a link to where/how the pretrained models can be downloaded and how they were trained (if applicable).  Alternatively you can have an additional column in your results table with a link to the models.

## Results

Our model achieves the following performance on :

### [Image Classification on ImageNet](https://paperswithcode.com/sota/image-classification-on-imagenet)

| Model name         | Top 1 Accuracy  | Top 5 Accuracy |
| ------------------ |---------------- | -------------- |
| My awesome model   |     85%         |      95%       |

>📋  Include a table of results from your paper, and link back to the leaderboard for clarity and context. If your main result is a figure, include that figure and link to the command or notebook to reproduce it. 


## Contributing

>📋  Pick a licence and describe how to contribute to your code repository. ****