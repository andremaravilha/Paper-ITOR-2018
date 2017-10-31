#ifndef ORCS_ROTHBERG_H
#define ORCS_ROTHBERG_H

#include "problem_data.h"
#include "solution_pool.h"
#include "heuristic.h"
#include <cstdlib>
#include <vector>
#include <random>
#include <ilcplex/ilocplex.h>
#include <cxxproperties.hpp>

ILOSTLBEGIN


namespace orcs {

/**
 * This class implements the Solution Polishing algorithm [1]. 
 * 
 * [1] Rothberg, E. An evolutionary algorithm for polishing mixed integer 
 * programming solutions. INFORMS Journal on Computing, v. 19, n. 4, 2007.
 */
class Rothberg : public Heuristic {
    
public:
    
    /**
     * Constructor.
     *
     * @param   problem
     *          Pointer to problem data.
     * @param   pool
     *          Pointer to solution pool.
     * @param   params
     *          Pointer to a properties structure with the heuristic parameters.
     */
    Rothberg(ProblemData* problem, SolutionPool* pool,
            const cxxproperties::Properties* params = nullptr);
    
    /**
     * Perform the heuristic search.
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
    void run(IloCplex::HeuristicCallbackI* callback, const cxxtimer::Timer* timer,
            double time_limit) override;
    
private:

    /*
     * Internal data structures.
     */
    std::mt19937 random_;
    SolutionPool* pool_;
    ProblemData* problem_;
    ProblemData submip_;
    std::vector<std::size_t> binary_variables_;

    /**
     * Heuristic parameters.
     */
    unsigned long long num_recombinations_;
    unsigned long long num_mutations_;
    double fixing_fraction_;
    double offset_;
    double offset_reduction_;
    double offset_minimum_;

    /*
     * Other parameters.
     */
    int seed_;
    long submip_nodes_limit_;
    long submip_nodes_unsuccessful_;

};

}

#endif