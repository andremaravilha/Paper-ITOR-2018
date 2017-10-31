#include "pool_callback.h"


IloCplex::Callback orcs::PoolCallback::create_instance(IloEnv& env, 
        orcs::SolutionPool* pool, IloNumVarArray& variables) {
    return (IloCplex::Callback(new (env) orcs::PoolCallback(env, pool, variables)));
}

orcs::PoolCallback::PoolCallback(IloEnv& env, orcs::SolutionPool* pool, 
        IloNumVarArray& variables) :
    IloCplex::IncumbentCallbackI(env), pool_(pool), variables_(variables)
{
    // It does nothing here.
}

IloCplex::CallbackI* orcs::PoolCallback::duplicateCallback() const {
    return (new (getEnv()) orcs::PoolCallback(*this));
}

void orcs::PoolCallback::main() {
    
    // Get the new incumbent solution
    IloNum value = getObjValue();
    IloNumArray solution(getEnv(), variables_.getSize());
    getValues(solution, variables_);
    
    // Try to add the new entry into the solution pool
    pool_->add_entry(solution, value);
    
    // Free resources
    solution.end();
}
