#ifndef ORCS_HEURISTIC_H
#define ORCS_HEURISTIC_H


#include <limits>
#include <ilcplex/ilocplex.h>
#include <cxxtimer.hpp>

ILOSTLBEGIN


namespace orcs {
    
/**
 * Interface implemented by heuristics.
 */
class Heuristic {

protected:

    /**
     * Tolerance when comparing equality between values.
     */
    static constexpr double THRESHOLD = 1e-5;

public:
    
    /**
     * This method must implement the heuristic method, which is called by
     * the heuristic callback throughout the CPLEX's branch-and-cut.
     * 
     * @param   callback
     *          A pointer to the heuristic callback object from which this 
     *          method is called.
     * @param   timer
     *          The timer to get the elapsed time spent on the entire
     *          optimization process.
     * @param   time_limit
     *          The time limit of the optimization process (in seconds).
     */
    virtual void run(IloCplex::HeuristicCallbackI* callback,
                     const cxxtimer::Timer* timer = nullptr,
                     double time_limit = std::numeric_limits<double>::max()) = 0;
};

}


#endif
