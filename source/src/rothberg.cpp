#include "rothberg.h"
#include "abort_callback.h"
#include <cmath>
#include <limits>
#include <algorithm>


orcs::Rothberg::Rothberg(ProblemData* problem, SolutionPool* pool,
        const cxxproperties::Properties* params) :
        problem_(problem), submip_(problem->env, problem->filename), pool_(pool)
{

    // Heuristic parameters
    num_recombinations_ = params->get<long>("recombinations", 40);
    num_mutations_ = params->get<long>("mutations", 20);
    fixing_fraction_ = params->get<double>("fixing-fraction", 0.5);
    offset_ = params->get<double>("offset-init", 0.2);
    offset_reduction_ = params->get<double>("offset-reduction", 0.25);
    offset_minimum_ = params->get<double>("offset-minimum", 0.01);

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

void orcs::Rothberg::run(IloCplex::HeuristicCallbackI* callback, 
        const cxxtimer::Timer* timer, double time_limit) {

    // Get the incumbent solution
    double incumbent_objective = callback->getIncumbentObjValue();
    IloNumArray incumbent_solution(problem_->env, problem_->variables.getSize());
    problem_->cplex.getValues(incumbent_solution, problem_->variables);
    
    // Mutations (need at least one feasible solution)
    if (pool_->size() >= 1) {
        
        for (long i = 0; i < num_mutations_; ++i) {
            
            // Check timer (stop criterion)
            if (timer != nullptr && (timer->count<std::chrono::milliseconds>() / 1000.0) >= time_limit) {
                break;
            }
            
            // Unextract previous model in CPLEX solver
            submip_.cplex.clear();
            
            // Randomly select a seed solution
            std::size_t idx = random_() % pool_->size();
            if (idx != 0) {
                std::size_t idx_aux = random_() % idx;
                idx = idx_aux;
            }
            const SolutionPool::Entry& entry = pool_->get_entries()[idx];
            
            // Define the size of the sub-MIP
            std::size_t count_fixed_variables = (std::size_t) std::round(binary_variables_.size() * fixing_fraction_);
            
            // Build the sub-MIP
            std::shuffle(binary_variables_.begin(), binary_variables_.end(), random_);
            for (std::size_t j = 0; j < binary_variables_.size(); ++j) {
                std::size_t index = binary_variables_[j];
                if (j < count_fixed_variables) {
                    IloNum value_to_fix = (entry.solution[index] > 0.5 ? 1.0 : 0.0);
                    submip_.variables[index].setLB(value_to_fix);
                    submip_.variables[index].setUB(value_to_fix);
                } else {
                    submip_.variables[index].setLB(problem_->variables[index].getLB());
                    submip_.variables[index].setUB(problem_->variables[index].getUB());
                }
            }
            
            // Extract sub-MIP model into CPLEX solver
            submip_.cplex.extract(submip_.model);

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

            // Update the fixing fraction
            if (!submip_has_improved) {
                if (submip_status == IloAlgorithm::Status::Optimal ||
                    submip_status == IloAlgorithm::Status::Infeasible) {

                    // Sub-MIP is too small to contain an improving solution
                    fixing_fraction_ = std::max(0.0, fixing_fraction_ - offset_);

                } else {

                    // Sub-MIP is too large to be efficiently explored
                    fixing_fraction_ = std::min(1.0, fixing_fraction_ + offset_);
                }
            }
        }
        
        // Update the fixing offset value
        offset_ = (1.0 - offset_reduction_) * offset_;
        offset_ = std::max(offset_minimum_, offset_);
    }
    
    // Recombinations (need at least two feasible solutions)
    if (pool_->size() >= 2) {
        
        // Define which iteration of Recombination will consider all solutions
        long consider_all = random_() % (num_recombinations_);
        
        for (long i = 0; i < num_recombinations_; ++i) {
            
            // Check timer (stop criterion)
            if (timer != nullptr && (timer->count<std::chrono::milliseconds>() / 1000.0) >= time_limit) {
                break;
            }
            
            // Unextract previous model in CPLEX solver
            submip_.cplex.clear();
            
            // Start solution (cutoff)
            const IloNumArray* start_sol = nullptr;
            IloNum start_obj;

            // Build the sub-MIP
            if (i == consider_all) {
            // Consider all solutions into the pool
                
                for (auto idx : binary_variables_) {
                    bool fix = true;
                    IloNum value = (pool_->get_entries()[0].solution[idx] > 0.5 ? 1.0 : 0.0);
                    for (std::size_t i = 1; i < pool_->size(); ++i) {
                        if (std::abs(value - pool_->get_entries()[i].solution[idx]) >= THRESHOLD) {
                            fix = false;
                            break;
                        }
                    }

                    if (fix) {
                        submip_.variables[idx].setLB(value);
                        submip_.variables[idx].setUB(value);
                    } else {
                        submip_.variables[idx].setLB(problem_->variables[idx].getLB());
                        submip_.variables[idx].setUB(problem_->variables[idx].getUB());
                    }
                }
                
                // Get the start solution
                start_sol = &(pool_->get_entries()[0].solution);
                start_obj = pool_->get_entries()[0].value;
                
            } else {
            // Consider only a pair of solutions
            
                // Randomly select two solutions
                std::size_t idx2 = (random_() % (pool_->size() - 1)) + 1;
                std::size_t idx1 = random_() % idx2;
                
                // Get the solutions selected
                const SolutionPool::Entry& entry1 = pool_->get_entries()[idx1];
                const SolutionPool::Entry& entry2 = pool_->get_entries()[idx2];
                
                for (auto idx : binary_variables_) {
                    if (std::abs(entry1.solution[idx] - entry2.solution[idx]) < THRESHOLD) {
                        IloNum value_to_fix = (entry1.solution[idx] > 0.5 ? 1.0 : 0.0);
                        submip_.variables[idx].setLB(value_to_fix);
                        submip_.variables[idx].setUB(value_to_fix);
                    } else {
                        submip_.variables[idx].setLB(problem_->variables[idx].getLB());
                        submip_.variables[idx].setUB(problem_->variables[idx].getUB());
                    }
                }
                
                // Get the start solution
                start_sol = &(entry1.solution);
                start_obj = entry1.value;
            }
            
            // Extract sub-MIP model into CPLEX solver
            submip_.cplex.extract(submip_.model);
            
            // Set cutoff
            //if (submip_.objective.getSense() == IloObjective::Minimize) {
            //    submip_.cplex.setParam(IloCplex::Param::MIP::Tolerances::UpperCutoff, start_obj);
            //} else {
            //    submip_.cplex.setParam(IloCplex::Param::MIP::Tolerances::LowerCutoff, start_obj);
            //}

            // Set a MIP start solution
            submip_.cplex.addMIPStart(submip_.variables, *start_sol);

            // Set sub-MIP abort callback
            submip_.cplex.use(orcs::AbortCallback::create_instance(submip_.env, timer,
                    time_limit, std::numeric_limits<unsigned long long>::max(),
                    submip_nodes_unsuccessful_));

            // Solve the sub-MIP
            if (submip_.cplex.solve()) {
                
                // Get the solution found
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
                    for (std::size_t i = 0; i < incumbent_solution.getSize(); ++i) {
                        incumbent_solution[i] = current_entry.solution[i];
                    }
                }
                
                // Free resources
                current_entry.solution.end();
            }
        }
    }
    
    // Let CPLEX know about a possible new incumbent solution
    callback->setSolution(problem_->variables, incumbent_solution);
    
    // Free resources
    incumbent_solution.end();
}
