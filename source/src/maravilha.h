#ifndef ORCS_MARAVILHA_H
#define ORCS_MARAVILHA_H

#include "problem_data.h"
#include "solution_pool.h"
#include "heuristic.h"
#include <cstdlib>
#include <random>
#include <vector>
#include <set>
#include <ilcplex/ilocplex.h>
#include <cxxproperties.hpp>

ILOSTLBEGIN


namespace orcs {

/**
 * This class implements a recombination-based matheuristic for mixed integer 
 * programming problem with binary variables, proposed by Maravilha, A. L.;
 * Campelo, F.; and Carrano, E. G.
 */
class Maravilha : public Heuristic {
    
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
    Maravilha(ProblemData* problem, SolutionPool* pool,
              const cxxproperties::Properties* params = nullptr);
    
    /**
     * Perform the heuristic search.
     * 
     * @param   callback
     *          A pointer to the heuristic callback class.
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
    std::set<std::size_t> variables_available_;
    std::vector<double> differences_;

    /*
     * Heuristic parameters.
     */
    long iterations_;
    double submip_min_;
    double submip_max_;
    double offset_;

    /*
     * Other parameters.
     */
    int seed_;
    long submip_nodes_limit_;
    long submip_nodes_unsuccessful_;
};

}

#endif
