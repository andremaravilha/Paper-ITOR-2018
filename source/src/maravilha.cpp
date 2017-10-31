#include "maravilha.h"
#include "abort_callback.h"
#include <cmath>
#include <limits>


orcs::Maravilha::Maravilha(ProblemData* problem, SolutionPool* pool,
        const cxxproperties::Properties* params) :
        problem_(problem), submip_(problem->env, problem->filename),
        pool_(pool), differences_(problem_->variables.getSize(), 0.0)
{

    // Heuristic parameters
    iterations_ = params->get<long>("iterations", 1L);
    submip_min_ = params->get<double>("submip-min", 0.00);
    submip_max_ = params->get<double>("submip-max", 0.65);
    offset_ = params->get<double>("offset", 0.45);

    // Other parameters
    seed_ = params->get<int>("seed", 0);
    submip_nodes_limit_ = params->get<long>("submip-nodes-limit", 500);
    submip_nodes_unsuccessful_ = params->get<long>("submip-nodes-unsuccessful", std::numeric_limits<long>::max());

    // Set CPLEX instance used to solve sub-MIPs
    submip_.cplex.setOut(submip_.env.getNullStream());
    submip_.cplex.setWarning(submip_.env.getNullStream());
    submip_.cplex.setError(submip_.env.getNullStream());
    submip_.cplex.setParam(IloCplex::Param::Threads, 1);
    submip_.cplex.setParam(IloCplex::Param::RandomSeed, seed_);
    submip_.cplex.setParam(IloCplex::Param::MIP::Limits::Nodes, submip_nodes_limit_);
    submip_.cplex.setParam(IloCplex::Param::MIP::Strategy::HeuristicFreq, 0);
    submip_.cplex.setParam(IloCplex::Param::MIP::Strategy::RINSHeur, 0);
    submip_.cplex.setParam(IloCplex::Param::MIP::Strategy::LBHeur, false);

    // Identify binary variables
    for (std::size_t i = 0; i < problem_->variables.getSize(); ++i) {
        if (problem_->variables[i].getType() == IloNumVar::Type::Bool || 
                (problem_->variables[i].getType() == IloNumVar::Type::Int && 
                std::abs(problem_->variables[i].getLB()) < THRESHOLD && 
                std::abs(problem_->variables[i].getUB() - 1.0) < THRESHOLD)) {
            binary_variables_.push_back(i);
        }
    }

    // Initialize the random number generator
    random_.seed(seed_);
}

void orcs::Maravilha::run(IloCplex::HeuristicCallbackI* callback,
        const cxxtimer::Timer* timer, double time_limit) {

    // Need at least one feasible solution
    if (pool_->size() > 0) {

        // Get the incumbent solution
        double incumbent_objective = callback->getIncumbentObjValue();
        IloNumArray incumbent_solution(problem_->env, problem_->variables.getSize());
        callback->getIncumbentValues(incumbent_solution, problem_->variables);

        // Get the relaxed solution from the current node
        double relaxed_objective = callback->getObjValue();
        IloNumArray relaxed_solution(problem_->env, problem_->variables.getSize());
        callback->getValues(relaxed_solution, problem_->variables);

        // Create and solve sub-MIPs
        long current_iteration = 0;
        while (current_iteration < iterations_) {

            // Check timer (stop criterion)
            if (timer != nullptr && (timer->count<std::chrono::milliseconds>() / 1000.0) >= time_limit) {
                break;
            }

            // Increment the iteration counter
            ++current_iteration;

            // Unextract previous model in CPLEX solver
            submip_.cplex.clear();

            // Select a solution from the pool
            std::size_t idx_pool = (random_() % pool_->size());
            const SolutionPool::Entry &entry = pool_->get_entries()[idx_pool];

            // Define the bias parameter (by GAP)
            double feas_bias = (entry.value - incumbent_objective) / (1e-5 + std::abs(incumbent_objective));
            feas_bias = 1.0 - std::max(0.0, std::min(1.0, feas_bias));

            double rel_bias = (incumbent_objective - relaxed_objective) / (1e-5 + std::abs(relaxed_objective));
            rel_bias = 1.0 - std::max(0.0, std::min(1.0, rel_bias));

            double bias = 1 - (feas_bias / (feas_bias + rel_bias));

            // Process information about each binary variable
            double sum_differences = 0.0;
            for (auto idx : binary_variables_) {

                // Fix binary variables with values equal to incumbent solution
                double value_to_fix = (incumbent_solution[idx] > 0.5 ? 1.0 : 0.0);
                submip_.variables[idx].setLB(value_to_fix);
                submip_.variables[idx].setUB(value_to_fix);

                // Compute the biased differences
                differences_[idx] = bias * std::abs(incumbent_solution[idx] - entry.solution[idx]) +
                        (1 - bias) * std::abs(incumbent_solution[idx] - relaxed_solution[idx]);

                sum_differences += differences_[idx];
            }

            // Index of binary variables available to construct the sub-MIP
            variables_available_.clear();
            variables_available_.insert(binary_variables_.begin(), binary_variables_.end());

            // Define the size of the sub-MIP
            std::size_t submip_size = (std::size_t) std::max(1.0,
                    (binary_variables_.size() * ((submip_min_ + submip_max_) / 2.0)));

            // Build the sub-MIP
            for (std::size_t count = 0; count < submip_size; ++count) {

                // Select a binary variable
                double rand_value = (random_() / (double) random_.max()) * sum_differences;
                double acc = 0.0;

                for (auto idx : variables_available_) {
                    acc += differences_[idx];
                    if (acc >= rand_value) {

                        // Make the binary variable free for optimization
                        submip_.variables[idx].setLB(problem_->variables[idx].getLB());
                        submip_.variables[idx].setUB(problem_->variables[idx].getUB());

                        // Remove the variable from the available ones
                        sum_differences -= differences_[idx];
                        variables_available_.erase(idx);

                        break;
                    }
                }
            }

            // Extract sub-MIP model into CPLEX solver
            submip_.cplex.extract(submip_.model);

            // Set a MIP start solution
            submip_.cplex.addMIPStart(submip_.variables, incumbent_solution);

            // Set sub-MIP abort callback
            submip_.cplex.use(orcs::AbortCallback::create_instance(submip_.env, timer,
                    time_limit, std::numeric_limits<unsigned long long>::max(),
                    submip_nodes_unsuccessful_));

            // Optimize the sub-MIP
            bool submip_found_solution = submip_.cplex.solve();
            IloAlgorithm::Status submip_status = submip_.cplex.getStatus();

            // Check if some solution was found
            bool submip_has_improved = false;
            if (submip_found_solution) {

                // Get the solution
                SolutionPool::Entry current_entry {
                        IloNumArray(submip_.env, submip_.variables.getSize()),
                        submip_.cplex.getObjValue(), 0};

                submip_.cplex.getValues(current_entry.solution, submip_.variables);

                // Update the solution pool
                pool_->add_entry(current_entry.solution, current_entry.value);

                // Check if the new solution is better than the current incumbent
                if ((submip_.objective.getSense() == IloObjective::Minimize &&
                     current_entry.value < incumbent_objective - THRESHOLD) ||
                    (submip_.objective.getSense() == IloObjective::Maximize &&
                     current_entry.value > incumbent_objective + THRESHOLD)) {

                    // Update the incumbent solution
                    incumbent_objective = current_entry.value;
                    for (std::size_t idx = 0; idx < incumbent_solution.getSize(); ++idx) {
                        incumbent_solution[idx] = current_entry.solution[idx];
                    }

                    // Set flag of improved solution found
                    submip_has_improved = true;
                }

                // Free resources
                current_entry.solution.end();
            }

            // Update the sub-MIP size (if necessary)
            if (!submip_has_improved) {
                if (submip_status == IloAlgorithm::Status::Optimal ||
                        submip_status == IloAlgorithm::Status::Infeasible) {

                    // Sub-MIP is too small to contain an improving solution
                    submip_min_ += (submip_max_ - submip_min_) * offset_;

                } else {

                    // Sub-MIP is too large to be efficiently explored
                    submip_max_ -= (submip_max_ - submip_min_) * offset_;
                }
            }
        }

        // Let CPLEX know about the best solution
        callback->setSolution(problem_->variables, incumbent_solution);

        // Free resources
        incumbent_solution.end();
        relaxed_solution.end();
    }
}
