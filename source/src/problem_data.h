#ifndef ORCS_PROBLEM_DATA_H
#define ORCS_PROBLEM_DATA_H

#include <string>
#include <ilcplex/ilocplex.h>

ILOSTLBEGIN


namespace orcs {

/**
 * This class groups data (model, variables, objective and constraints) of an 
 * optimization problem.
 */
class ProblemData {
    
public:
    
    std::string filename;
    IloEnv env;
    IloCplex cplex;
    IloModel model;
    IloObjective objective;
    IloNumVarArray variables;
    IloRangeArray constraints;
    

    /**
     * Constructor. This constructor loads an optimization problem from a file.
     * 
     * @param   env
     *          CPLEX environment.
     * @param   filename
     *          Path to file containing the optimization problem to be loaded.
     * @param   countdown
     *          A countdown.
     */
    ProblemData(IloEnv& env, const std::string& filename);
    
    /**
     * Destructor.
     */
    virtual ~ProblemData();
    
    ProblemData(const ProblemData& other) = delete;
    ProblemData(ProblemData&& other) = delete;
    ProblemData& operator=(const ProblemData& other) = delete;
    ProblemData& operator=(ProblemData&& other) = delete;

};

}

#endif
