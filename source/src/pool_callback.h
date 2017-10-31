#ifndef ORCS_SOLUTION_POOL_CALLBACK_H
#define ORCS_SOLUTION_POOL_CALLBACK_H


#include "solution_pool.h"
#include <ilcplex/ilocplex.h>

ILOSTLBEGIN


namespace orcs {


/**
 * Callback class used to store into the pool feasible solutions found throught
 * the optimization process.
 */
class PoolCallback : public IloCplex::IncumbentCallbackI {

public:
    
    /**
     * This static method creates a new instance of this class and returns a 
     * handle for the instance.
     * 
     * @param   env
     *          CPLEX environment.
     * @param   pool
     *          A solution pool to keep the solutions found.
     * @param   variables
     *          Variables of the optimization problem.
     */
    static IloCplex::Callback create_instance(IloEnv& env, SolutionPool* pool, 
            IloNumVarArray& variables);

protected:
    
    /**
     * Constructor is made protected. For creating an instance, you should call
     * the static method create_instance(...).
     * 
     * @param   env
     *          CPLEX environment.
     * @param   pool
     *          A solution pool to keep the solutions found.
     * @param   variables
     *          Variables of the optimization problem.
     */
    PoolCallback(IloEnv& env, SolutionPool* pool, IloNumVarArray& variables);
    
    IloCplex::CallbackI* duplicateCallback() const override;

    void main() override;
    
private:
    
    SolutionPool* pool_;
    IloNumVarArray variables_;
    
};

}


#endif
