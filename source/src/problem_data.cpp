#include "problem_data.h"


orcs::ProblemData::ProblemData(IloEnv& env_, const std::string& filename_) : 
    env(env_), filename(filename_), cplex(env), model(env), objective(), 
    variables(env), constraints(env)
{
    
    // Import model from file
    cplex.importModel(model, filename.c_str(), objective, variables, constraints);
    cplex.extract(model);
}

orcs::ProblemData::~ProblemData() {
    cplex.end();
}
