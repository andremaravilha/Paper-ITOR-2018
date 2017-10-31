#ifndef ORCS_SOLUTION_POOL_H
#define ORCS_SOLUTION_POOL_H


#include <cstdlib>
#include <vector>
#include <ilcplex/ilocplex.h>

ILOSTLBEGIN


namespace orcs {
    
/**
 * This class keeps a set of entries, where each entry consists of a solution 
 * and its objective function evaluation. It is noteworthy that the pool has a 
 * limited size, then it keeps the best solutions without repetitions only.
 */
class SolutionPool {
    
public:
    
    /**
     * An entry of the pool. Each entry consists of a solution encoded by an 
     * IloNumArray (a CPLEX object) of values assigned to each variable of the 
     * optimization problem and the value of the objective function evaluation 
     * encoded as a IloNum (a CPLEX object).
     */
    struct Entry {
        IloNumArray solution;
        IloNum value;
        unsigned long long age;
        
        Entry(IloNumArray solution_, IloNum value_, unsigned long long age_) 
                : solution(solution_), value(value_), age(age_) {};
    };
    
    
    /**
     * Constructor.
     * 
     * @param   env
     *          CPLEX environment.
     * @param   sense
     *          It must be set to IloObjective::Sense::Minimize for minimization 
     *          problems or IloObjective::Sense::Maximize for maximization 
     *          problems.
     * @param   max_size
     *          The maximum number of entries kept in the pool.
     * @param   sorted
     *          If true, the solutions are sorted from the best solution
     *          (regarding to the value of the objective function) to the worst 
     *          one (with increasing age as a secondary criterion). Otherwise, 
     *          the order of the entries is undefined.
     */
    SolutionPool(IloEnv& env, IloObjective::Sense sense, std::size_t max_size, 
            bool sorted);
    
    /**
     * Destructor.
     */
    virtual ~SolutionPool();
    
    /**
     * Return a vector with all entries that make up this solution pool.
     * 
     * @return  A vector with all entries.
     */
    const std::vector<Entry>& get_entries() const;
    
    /**
     * Try to add a new entry into this pool. An entry is added if and only if 
     * the pool does not contain any entry with a similar solution (disregarding 
     * the value of the objective function). If the pool is full, a new entry is 
     * added if and only if the pool does not contain any entry with a similar 
     * solution and the value of the objective function of the new entry is 
     * better than at least one of the entries into the pool. In this case, the 
     * new entry replaces the entry of the pool with the worst value of 
     * objective function.
     * 
     * @param   solution
     *          A solution
     * @param   value
     *          The value of the objective function evaluation for the solution.
     * 
     * @return  True if the new entry was added into the pool, false otherwise.
     */
    bool add_entry(const IloNumArray& solution, const IloNum value);
    
    /**
     * Return the number of entries in this pool.
     * 
     * @return  The number of entries in this pool.
     */
    std::size_t size() const;
    
    /**
     * Return the maximum number of entries this pool is able to keep.
     * 
     * @return  The maximum number of entries this pool is able to keep.
     */
    std::size_t max_size() const;
    
    SolutionPool(const SolutionPool& other) = delete;
    SolutionPool(SolutionPool&& other) = delete;
    SolutionPool& operator=(const SolutionPool& other) = delete;
    SolutionPool& operator=(SolutionPool&& other) = delete;
    
private:
    
    IloEnv& env_;
    IloObjective::Sense sense_;
    bool sorted_;
    unsigned long long next_age_;
    std::size_t max_size_;
    std::vector<Entry> entries_;
    
    static constexpr double SIMILARITY_THRESHOLD = 1e-5;
    
};

}

#endif
