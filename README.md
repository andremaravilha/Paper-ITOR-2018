# A Recombination-based Matheuristic for MIP Problems with Binary Variables

> **Contributors:** André L. Maravilha<sup>1,3</sup>, Eduardo G. Carrano<sup>2,3</sup>, Felipe Campelo<sup>2,3</sup>  
> <sup>1</sup> *Graduate Program in Electrical Engineering, Universidade Federal de Minas Gerais ([PPGEE](https://www.ppgee.ufmg.br/), [UFMG](https://www.ufmg.br/))*  
> <sup>2</sup> *Dept. Electrical Engineering, Universidade Federal de Minas Gerais ([DEE](http://www.dee.ufmg.br/), [UFMG](https://www.ufmg.br/))*  
> <sup>3</sup> *Operations Research and Complex Systems Lab., Universidade Federal de Minas Gerais ([ORCS Lab](http://orcslab.ppgee.ufmg.br/), [UFMG](https://www.ufmg.br/))*

## 1. About

This repository contains the code of the MIP heuristics evaluated in the manuscript entitled "A Recombination-based Matheuristic for Mixed Integer Programming Problems with Binary Variables", written by André L. Maravilha, Eduardo G. Carrano and Felipe Campelo and published in the Iternational Transactions in Operational Research ([ITOR](https://onlinelibrary.wiley.com/doi/pdf/10.1111/itor.12526)).

The MIP heuristics were coded in C++ (version C++17) over [CPLEX](http://www.ibm.com/software/commerce/optimization/cplex-optimizer/) (version 12.7.1), using callback methods. The project was developed using [CLion](https://www.jetbrains.com/clion/) (version 2017.2.3) with [CMake](https://cmake.org/) (version 3.8) and GNU Compiller Collection ([GCC](https://gcc.gnu.org/), version 6.3.0) on a Linux machine. The content of this repository is available under the MIT license.


## 2. How to build the project

#### 2.1. Important comments before building the project

To compile this project you need to have the CPLEX (version 12.7.1 or later), GCC (version 6.3.0 or later) and CMake (version 3.8 or later) installed on you computer.

#### 2.2. Building the project

Inside the root directory of the project, run the following commands:
```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../source
make
```

The project is configured to search for CPLEX at `/opt/ibm/ILOG/CPLEX_Studio1271/` directory. If CPLEX is installed in another directory, you have to set the correct directory. For example, if CPLEX is installed under the directory `/opt/cplex/`, you can run the CMake as follows:
```
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release -DCPLEX_PATH=/opt/cplex ../source
make
```

## 3. Running the project

Inside the `experiments` directory, you can find a Python script `run.py` that performs the same experiment described in the manuscript submitted to ITOR. To run it, after compiling the project (as described in the previous section) and inside the `experiments` directory, run the following command:
```
python3 run.py
```  
After its completion, you will find a CSV file `results.csv` with the results of the experiments.

Note that to run this script you need the MIPLIB 2010 instances (the .mps.gz files) inside a directory `instances` that must be in the same directory the `run.py` script is. The MIPLIB 2010 instances can be downloaded at [MIPLIB web site](http://miplib.zib.de/).

However, if you want to solve other MIP instances or to use other parameters for the heuristics, you can run the executable created after the building the project. The subsection below shows some examples of how to use the project and the section 4 (*Parameters description*) shows and describes all parameters of the software.


### 3.1. Usage examples

After build the project, you can run the executable created to solve MIP problems. You can solve models written in MPS or LP files. Below we present some examples on how to run the executable. For the examples, consider `itor` as the name of the executable file after building and `problem.mps.gz` as the name of the file containing the model to be solved. A complete list of parameters is given in the next sections.

###### Show the help message:
```
./itor --help
```

###### Using Maravilha's MIP heuristic:
```
./itor -v -d 4 --heuristic maravilha --pool-size 40 --heuristic-trigger-nodes 50000 --heuristic-proportional-time-limit 0.5 --submip-nodes-limit 500 --file problem.mps.gz
```

In the example above, after exploring 50,000 MIP nodes with default CPLEX, the optimization process continues for more half of the time spent, but now running Maravilha's heuristic on each subsequent MIP node. Besides, the number of MIP nodes explored at each sub-MIP is limited to 500 and the maximum size of the pool of solutions is limited to 40.

###### Using Rothberg's MIP heuristic (Solution Polishing):
```
./itor -v -d 4 --heuristic rothberg --pool-size 40 --heuristic-trigger-nodes 50000 --heuristic-proportional-time-limit 0.5 --submip-nodes-limit 500 --file problem.mps.gz
```

In the example above, after exploring 50,000 MIP nodes with default CPLEX, the optimization process continues for more half of the time spent, but now running Rothberg's heuristic (i.e., our implementation of Solution Polishing heuristic) on each subsequent MIP node. Besides, the number of MIP nodes explored at each sub-MIP is limited to 500 and the maximum size of the pool of solutions is limited to 40.


## 4. Parameters description

#### 4.1. General parameters:

`-h`, `--help`  
Show a help message and exit.

`-f <VALUE>`, `--file <VALUE>`  
Name of the file containing the model. Valid suffixes are .MPS and .LP. Files can be compressed, so the additional suffix .GZ is accepted.

`--seed <VALUE>`  
(Default: `0`)  
Set the seed used to initialize the random number generator used by CPLEX solver and MIP heuristics.

`--heuristic <VALUE>`  
(Default: `none`)  
The improvement MIP heuristic to use after some trigger (time of number of MIP nodes) is activated. Valid values are:
* `none`: continues the optimization with CPLEX default options.
* `cplex-polishing`: allows CPLEX' own implementation of Solution Polishing MIP heuristic.
* `rothberg`: allows the use of the Solution Polishing MIP heuristic proposed by Rothberg.
* `maravilha`: allows the use of the proposed Recombination-based Matheuristic. 

`--heuristic-trigger-nodes <VALUE>`  
Number of MIP nodes explored before start using the MIP heuristic (if any is set). If not set, this trigger is disabled.

`--heuristic-trigger-time <VALUE>`  
Time spent with default CPLEX before start using the MIP heuristic (if any is set). If not set, this trigger is disabled.

`--heuristic-frequency <VALUE>`  
Frequency the heuristic is called. For example: if set to 100, and it is called the first time at node 1000, then it will be called at nodes 1100, 1200 and so on. If set to zero, the heuristic will not be called.

`--heuristic-nodes-limit <VALUE>`  
Additional MIP nodes to continue the optimization process using the MIP heuristic. If not set, this stopping criterion is ignored.

`--heuristic-proportional-time-limit <VALUE>`  
Additional time to continue the optimization process using the MIP heuristic. It the optimization process spends 100 seconds solving the first initial MIP nodes before start using the MIP heuristic and this parameter is set to 0.5, then the optimization process will continue for another 0.5 x 100 = 50 seconds performing the heuristic search. If not set, this stopping criterion is ignored.

`--heuristic-absolute-time-limit <VALUE>`  
Additional time to continue the optimization process using the MIP heuristic. It this parameter is set to 100, then the optimization process will continue for another 100 seconds performing the heuristic search. If not set, this stopping criterion is ignored.

`--submip-nodes-limit <VALUE>`  
Maximum number of MIP nodes explored by each sub-MIP problem solved by a MIP heuristic.

`--submip-nodes-unsuccessful <VALUE>`  
Maximum number of MIP nodes explored without improvement in the sub-MIP incumbent solution. If not set, this stopping criteria is ignored.

`--pool-size <VALUE>`  
The maximum number of solutions kept in the pool of solutions.

#### 4.2. Printing parameters:

`-v`, `--verbose`  
Display the progress of the optimization process throughout its running.

`-d`, `--details`  
(Default: `1`)  
Set the level of details to show at the end of the the optimization process. Valid values are:
* `0`: show nothing.
* `1`: show the value of objective function if a feasible solution is found. Otherwise, it shows a question mark.
* `2`: show the status of the optimization process (Unknown, Feasible, Optimal, Infeasible, Unbounded, Infeasible or Unbounded, Error) and the value of objective function for the best solution found if the status is Feasible or Optimal.
* `3`: show the status, the value of objective function for the best solution found (if any), number of solutions in the pool, number of MIP nodes explored, and the runtime in seconds, (each information is separated by a single blank space and, if some information is not available, a question mark is printed in its place).
* `4`: show a report containing more detailed information.

`-s <value>`, `--solution <VALUE>`  
Name of the file to save with best solution found.

#### 4.3. Maravilha's MIP heuristic parameters:

`--maravilha-iterations <VALUE>`  
(Default: `1`)  
Number of sub-MIPs to solve each time Maravilha's MIP heuristic is performed.

`--maravilha-submip-min <VALUE>`  
(Default: `0.00`)  
The minimum proportion of binary variables not fixed on sub-MIP problems.

`--maravilha-submip-max <VALUE>`  
(Default: `0.65`)  
The maximum proportion of binary variables not fixed on sub-MIP problems.

`--maravilha-offset <VALUE>`  
(Default: `0.45`)  
Value used to auto-adjust (increase/decrease) the size limits of sub-MIPs. It must be a value between 0 and 1.

#### 4.4. Rothberg's MIP heuristic parameters:

`--rothgberg-recombinations <VALUE>`  
Number of recombination sub-MIP problems solved at each time Rothberg's MIP heuristic is performed.

`--rothgberg-mutations <VALUE>`  
Number of mutation sub-MIP problems solved at each time Rothberg's MIP heuristic is performed.

`--rothgberg-fixing-fraction <VALUE>`  
(Default: `0.5`)  
Initial value of the fixing fraction. It must be a value between 0 and 1.

`--rothgberg-offset-init <VALUE>`  
(Default: `0.2`)  
Value used to auto-adjust (increase/decrease) fixing fraction of mutation sub-MIPs. It must be a value between 0 and 1.

`--rothgberg-offset-reduction <VALUE>`  
(Default: `0.25`)  
The offset value is reduced by (100 x `<VALUE>`)% after all mutation phase. It must be a value between 0 and 1.

`--rothgberg-offset-minimum <VALUE>`  
(Default: `0.01`)  
The lowest value the offset can take. It must be a value between 0 and 1.

