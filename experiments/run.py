# MIT License
# 
# Copyright (c) 2017 André L. Maravilha
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

'''
This script runs the same experiments for performance comparison of the MIP 
heuristics described in the paper submitted to the Intenational Transactions 
in Operational Research (ITOR).
'''

import argparse
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from multiprocessing import RLock


###################################################################################################
# Re-entrant lock object used to control the access to the output file.
#
lock = RLock()


###################################################################################################
# Initializes the CSV output file and writes the header.
#
def initialize_results_file(filename):
    with open(filename, "w") as f:
        f.write("INSTANCE,ALGORITHM,SEED,STATUS,OBJECTIVE,NODES\n")


###################################################################################################
# Writes the results of an entry in the output file.
#
def write_results(filename, results):
    content = ",".join([results["instance"], results["algorithm"], results["seed"], 
                        results["status"], results["objective"], results["nodes"]])

    f = open(filename, "a")
    f.write(content + "\n")
    f.close()


###################################################################################################
# Runs an entry.
#
def run_entry(instance, algorithm, data, seed):

    # Keep the result
    results = dict()
    results["instance"] = instance
    results["algorithm"] = algorithm
    results["seed"] = str(seed)
    results["status"] = "Error"
    results["objective"] = ""
    results["nodes"] = ""
    results["time"] = ""

    try:

        # Create the command to run
        command = ([data["command"], "--seed", str(seed)] + data["algorithms"][algorithm] +
                   ["--file", data["instances"][instance]])

        # Run the command
        output = subprocess.run(command, stdout=subprocess.PIPE, universal_newlines=True)

        # Check if the command run without errors
        if output.returncode == 0:

            # Get the output as a string
            str_output = str(output.stdout).strip('\n\t ').split(" ")

            # Process the output
            status = str_output[0]
            results["status"] = status

            if status != "Error":
                results["nodes"] = str_output[3]
                results["time"] = str_output[4]

            if status == "Feasible" or status == "Optimal":
                results["objective"] = str_output[1]

    except:
        results["status"] = "Error"
        results["objective"] = ""
        results["nodes"] = ""
        results["time"] = ""

    # Write results in the output file and log the progress
    with lock:

        # Write results
        write_results(data["output-file"], results)

        # Progress log
        data["progress"] += 1
        total_entries = len(data["instances"]) * len(data["algorithms"]) * len(data["seeds"])
        progress = (data["progress"] / total_entries) * 100

        print("[{:3} of {:3} ({:6.2f}%) completed] {:15} -> {:16} -> {:4} -> {:8}"
                .format(data["progress"], total_entries, progress, algorithm, instance, seed, results["status"]))

        sys.stdout.flush()


###################################################################################################
# Main function
#
def main():

    # Configure argument parser
    parser = argparse.ArgumentParser(description="Perform the computational experiments.")
    parser.add_argument("-c", "--continue", dest="continue_previous", action="store_true",
                        help="Resume the experiment from a previous state.")

    # Parse input arguments
    args = parser.parse_args()

    # Map used to store data necessary to run the experiments
    data = dict()
    data["command"] = "../build/itor"       # program
    data["threads"] = 18                    # entries to solve simultaneouly
    data["output-file"] = "results.csv"     # file to write the results
    data["instances-path"] = "./instances"  # base path to instances
    data["instances"] = dict()              # path to each instance
    data["algorithms"] = dict()             # settings of each algorithm
    data["seeds"] = None                    # seeds
    data["progress"] = 0                    # num. of entries finished

    # Set the seeds used at each repetition of (instance, algorithm)
    data["seeds"] = [29, 173, 281, 409, 541]

    # Load the list of instances
    instances = ["probportfolio", "liu", "p100x588b", "d10200", "bg512142", "nag", "dg012142", 
                 "g200x740i", "gmut-77-40", "bab1", "mkc", "r80x800", "shipsched", "usAbbrv-8-25_70", 
                 "wnq-n100-mw99-14", "reblock354", "d20200", "gmut-75-50", "rvb-sub", "set3-20",
                 "set3-10", "rocII-9-11", "set3-15", "queens-30", "sct32", "n370a", "ns2124243", 
                 "n3705", "dc1c", "n3700", "nu120-pr3", "rococoC12-111000", "sct1", "dolom1", 
                 "seymour-disj-10", "b2c1s1", "ns2137859", "rmine10", "germanrr", "ex1010-pi",
                 "sts405", "uc-case3", "sing2", "siena1", "dc1l"]

    for instance in instances:
        data["instances"][instance] = data["instances-path"]+ "/" + instance + ".mps.gz"

    # Algorithms' settings
    params_common = "--details 3 --heuristic-trigger-nodes 50000 --heuristic-proportional-time-limit 0.5".split(" ")
    params_stop   = "--submip-nodes-limit 500".split(" ")

    params_cplex_default   = (params_common +
                              params_stop +
                              "--heuristic none".split(" "))

    params_cplex_polishing = (params_common +
                              params_stop +
                              "--heuristic cplex-polishing".split(" "))

    params_rothberg        = (params_common +
                              params_stop +
                              "--heuristic rothberg".split(" ") +
                              "--pool-size 40".split(" ") +
                              "--rothberg-recombinations 40".split(" ") +
                              "--rothberg-mutations 20".split(" ") +
                              "--rothberg-fixing-fraction 0.5".split(" ") +
                              "--rothberg-offset-init 0.2".split(" ") +
                              "--rothberg-offset-reduction 0.25".split(" ") +
                              "--rothberg-offset-minimum 0.01".split(" "))

    params_maravilha       = (params_common +
                              params_stop +
                              "--heuristic maravilha".split() +
                              "--pool-size 40".split(" ") +
                              "--maravilha-iterations 1".split() +
                              "--maravilha-submip-min 0.00".split() +
                              "--maravilha-submip-max 0.65".split() +
                              "--maravilha-offset 0.45".split())

    data["algorithms"]["cplex-default"] = params_cplex_default
    data["algorithms"]["cplex-polishing"] = params_cplex_polishing
    data["algorithms"]["rothberg"] = params_rothberg
    data["algorithms"]["maravilha"] = params_maravilha
    
    # Create and initialize the output file
    if not os.path.exists(data["output-file"]) or not args.continue_previous:
        initialize_results_file(data["output-file"])

    # Check entries already solved
    entries_completed = []
    if args.continue_previous:
        if os.path.exists(data["output-file"]):
            with open(data["output-file"], "r") as f:
                lines = f.readlines()[1:]
                for line in lines:
                    values = line.strip().split(",")
                    print(values)
                    if len(values) > 0:
                        entries_completed.append((values[0], values[1], int(values[2])))

    data["progress"] = len(entries_completed)

    # Run each entry (instance, algorithm, seed)
    with ThreadPoolExecutor(max_workers=data["threads"]) as executor:
        for seed in data["seeds"]:
            for instance in list(data["instances"].keys()):
                for algorithm in list(data["algorithms"].keys()):
                    if entries_completed.count((instance, algorithm, seed)) == 0:
                        executor.submit(run_entry, instance, algorithm, data, seed)


###################################################################################################
# Main statements
#
if __name__ == "__main__":
    main()

