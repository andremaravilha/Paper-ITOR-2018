#ifndef ORCS_HEURISTIC_CALLBACK_H
#define ORCS_HEURISTIC_CALLBACK_H


#include <limits>
#include <ilcplex/ilocplex.h>
#include "heuristic.h"
#include <cxxtimer.hpp>


ILOSTLBEGIN


namespace orcs {

    
/**
 * Callback class used to perform custom heuristic methods throughout 
 * the CPLEX' branch-and-cut.
 */
class HeuristicCallback : public IloCplex::HeuristicCallbackI {

public:
    
    /**
     * This static method creates a new instance of this class and returns a 
     * handle for the instance.
     * 
     * @param   env
     *          CPLEX environment.
     * @param   heuristic
     *          A pointer to the heuristic object.
     * @param   frequency
     *          Frequency the heuristic search is performed. If it is set to 100,
     *          the heuristic search is performed at nodes 100, 200, 300 and so 
     *          on. If set to 0, the heuristic search will not be performed.
     * @param   timer
     *          The timer to get the elapsed time spent on the entire
     *          optimization process.
     * @param   time_limit
     *          The time limit (in seconds) of the optimization process.
     */
    static IloCplex::Callback create_instance(IloEnv& env, Heuristic* heuristic, 
            unsigned long long frequency, const cxxtimer::Timer* timer = nullptr,
            double time_limit = std::numeric_limits<double>::max());
    
protected:
    
    /**
     * Constructor is made protected. For creating an instance, you should call
     * the static method create_instance(...).
     * 
     * @param   env
     *          CPLEX environment.
     * @param   heuristic
     *          A pointer to the heuristic object.
     * @param   frequency
     *          Frequency the heuristic search is performed. If it is set to 100,
     *          the heuristic search is performed at nodes 100, 200, 300 and so 
     *          on. If set to 0, the heuristic search will not be performed.
     */
    HeuristicCallback(IloEnv& env, Heuristic* heuristic, 
            unsigned long long frequency, const cxxtimer::Timer* timer,
            double time_limit);
    
    IloCplex::CallbackI* duplicateCallback() const override;
    void main() override;
    
private:
    
    Heuristic* heuristic_;
    const cxxtimer::Timer* timer_;
    double time_limit_;
    unsigned long long frequency_;
};

}


#endif
