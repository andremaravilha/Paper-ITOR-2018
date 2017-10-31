#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <string>
#include <limits>
#include <chrono>
#include <set>
#include <ilcplex/ilocplex.h>
#include <cxxopts.hpp>
#include <cxxtimer.hpp>
#include <cxxproperties.hpp>
#include "problem_data.h"
#include "solution_pool.h"
#include "pool_callback.h"
#include "heuristic_callback.h"
#include "abort_callback.h"
#include "rothberg.h"
#include "maravilha.h"


ILOSTLBEGIN


/* *****************************************************************************
 * Definitions and constant values
 */

constexpr unsigned long long CPLEX_DEFAULT_LIMIT_NODES = 9223372036800000000;
constexpr double CPLEX_DEFAULT_LIMIT_TIME = 1e75;


/* *****************************************************************************
 * Data structures.
 */

struct Result {
    IloAlgorithm::Status status;
    IloInt64 mip_nodes_explored;
    IloNum runtime;
    IloNum objective_value;
    std::size_t pool_size;
};


/* *****************************************************************************
 * Function statements.
 */

cxxopts::Options
create_command_parser(int argc, char** argv);

void
print_command_help(const cxxopts::Options&options,
                   std::ostream& output);

Result
get_result(const orcs::ProblemData& problem,
           const orcs::SolutionPool& pool,
           const cxxtimer::Timer& timer);

void
print_results(orcs::ProblemData& problem,
              const Result& before,
              const Result& after,
              const cxxopts::Options& options);

void
print_details_1(orcs::ProblemData& problem,
                const Result& before,
                const Result& after,
                const cxxopts::Options& options);

void
print_details_2(orcs::ProblemData& problem,
                const Result& before,
                const Result& after,
                const cxxopts::Options& options);

void
print_details_3(orcs::ProblemData& problem,
                const Result& before,
                const Result& after,
                const cxxopts::Options& options);

void
print_details_4(orcs::ProblemData& problem,
                const Result& before,
                const Result& after,
                const cxxopts::Options& options);


/* *****************************************************************************
 * Implementation of functions (including main function).
 */

int main(int argc, char** argv) {

    // CPLEX environment
    IloEnv env;

    try {

        // Initialize the command line parser
        cxxopts::Options options = create_command_parser(argc, argv);

        // Print help message and exit, if requested
        if (options.count("help") > 0) {
            print_command_help(options, std::cout);
            return EXIT_SUCCESS;
        }

        // Abort, if problem file is not specified
        if (options["file"].count() == 0) {
            throw std::string("No file have been specified.");
        }

        // Abort, if heuristic is not valid
        std::set<std::string> heuristic_values = {"none", "cplex-polishing", "rothberg", "maravilha"};
        if (heuristic_values.count(options["heuristic"].as<std::string>()) == 0) {
            throw std::string("Invalid heuristic method.");
        }

        // Disable CPLEX output log
        env.setOut(env.getNullStream());
        env.setWarning(env.getNullStream());
        env.setError(env.getNullStream());

        // Load the problem file
        orcs::ProblemData problem(env, options["file"].as<std::string>().c_str());

        // Set some settings of CPLEX
        problem.cplex.setParam(IloCplex::Param::Threads, 1);
        problem.cplex.setParam(IloCplex::Param::RandomSeed, options["seed"].as<unsigned long>());

        // Verbosity
        if (options.count("verbose") > 0) {
            problem.cplex.setOut(std::cout);
            problem.cplex.setParam(IloCplex::Param::MIP::Display, 2);
        }

        // Add solution pool callback
        orcs::SolutionPool pool(problem.env, problem.objective.getSense(), options["pool-size"].as<long>(), true);
        problem.cplex.use(orcs::PoolCallback::create_instance(env, &pool, problem.variables));

        // Triggers to start heuristic
        if (options.count("heuristic-trigger-nodes") > 0) {
            problem.cplex.setParam(IloCplex::Param::MIP::Limits::Nodes, options["heuristic-trigger-nodes"].as<long>());
        }

        if (options.count("heuristic-trigger-time") > 0) {
            problem.cplex.setParam(IloCplex::Param::TimeLimit, options["heuristic-trigger-time"].as<double>());
        }

        // Timer (used to compute running time)
        cxxtimer::Timer timer;

        // Solve model (1st phase: before heuristics)
        timer.start();
        problem.cplex.solve();
        timer.stop();

        // Get result
        Result result_before_heuristic = get_result(problem, pool, timer);

        // Reset CPLEX stopping criteria
        problem.cplex.setParam(IloCplex::Param::MIP::Limits::Nodes, CPLEX_DEFAULT_LIMIT_NODES);
        problem.cplex.setParam(IloCplex::Param::TimeLimit, CPLEX_DEFAULT_LIMIT_TIME);

        double time_limit = std::numeric_limits<double>::max();
        if (options.count("heuristic-absolute-time-limit") > 0) {
            time_limit = std::min(time_limit, result_before_heuristic.runtime + options["heuristic-absolute-time-limit"].as<double>());
        }

        if (options.count("heuristic-proportional-time-limit") > 0) {
            time_limit = std::min(time_limit, (1.0 + options["heuristic-proportional-time-limit"].as<double>()) * result_before_heuristic.runtime);
        }

        if (options.count("heuristic-nodes-limit") > 0) {
            long nodes_limit = result_before_heuristic.mip_nodes_explored + options["heuristic-nodes-limit"].as<long>();
            problem.cplex.use(orcs::AbortCallback::create_instance(env, &timer, time_limit, nodes_limit));
        } else {
            problem.cplex.use(orcs::AbortCallback::create_instance(env, &timer, time_limit));
        }

        // Initialize heuristic method
        orcs::Heuristic* heuristic = nullptr;
        cxxproperties::Properties heuristic_params;

        if (options["heuristic"].as<std::string>().compare("none") != 0) {

            if (options["heuristic"].as<std::string>().compare("cplex-polishing") == 0) {

                // Enable CPLEX' implementation of Solution Polishing
                problem.cplex.setParam(IloCplex::Param::MIP::PolishAfter::Nodes, 0);
                problem.cplex.setParam(IloCplex::Param::MIP::Limits::SubMIPNodeLim,
                                       options["submip-nodes-limit"].as<long>());

                // CPLEX stop criteria
                double polishing_time_limit = CPLEX_DEFAULT_LIMIT_TIME;
                long polishing_nodes_limit = CPLEX_DEFAULT_LIMIT_NODES;

                if (options.count("heuristic-absolute-time-limit") > 0) {
                    polishing_time_limit = std::min(polishing_time_limit, result_before_heuristic.runtime + options["heuristic-absolute-time-limit"].as<double>());
                }

                if (options.count("heuristic-proportional-time-limit") > 0) {
                    polishing_time_limit = std::min(polishing_time_limit, (1.0 + options["heuristic-proportional-time-limit"].as<double>()) * result_before_heuristic.runtime);
                }

                if (options.count("heuristic-nodes-limit") > 0) {
                    polishing_nodes_limit = result_before_heuristic.mip_nodes_explored + options["heuristic-nodes-limit"].as<long>();
                }

                problem.cplex.setParam(IloCplex::Param::MIP::Limits::Nodes, polishing_nodes_limit);
                problem.cplex.setParam(IloCplex::Param::TimeLimit, polishing_time_limit);

            } else {

                // Turn off CPLEX own heuristics
                problem.cplex.setParam(IloCplex::Param::MIP::Strategy::HeuristicFreq, -1);
                problem.cplex.setParam(IloCplex::Param::MIP::Strategy::RINSHeur, -1);
                problem.cplex.setParam(IloCplex::Param::MIP::Strategy::LBHeur, false);

                // Load general heuristic parameters
                heuristic_params.add("verbose", options["verbose"].as<bool>());
                heuristic_params.add("seed", options["seed"].as<unsigned long>());
                heuristic_params.add("submip-nodes-limit", options["submip-nodes-limit"].as<long>());
                if (options.count("submip-nodes-unsuccessful") > 0) {
                    heuristic_params.add("submip-nodes-unsuccessful", options["submip-nodes-unsuccessful"].as<long>());
                }

                // Set the heuristic chosen by the user
                if (options["heuristic"].as<std::string>().compare("maravilha") == 0) {

                    // Load Maravilha's heuristic parameters
                    heuristic_params.add("iterations", options["maravilha-iterations"].as<long>());
                    heuristic_params.add("submip-min", options["maravilha-submip-min"].as<double>());
                    heuristic_params.add("submip-max", options["maravilha-submip-max"].as<double>());
                    heuristic_params.add("offset", options["maravilha-offset"].as<double>());

                    // Instantiate the heuristic
                    heuristic = new orcs::Maravilha(&problem, &pool, &heuristic_params);

                } else if (options["heuristic"].as<std::string>().compare("rothberg") == 0) {

                    // Load Rothberg's heuristic parameters
                    heuristic_params.add("recombinations", options["rothberg-recombinations"].as<long>());
                    heuristic_params.add("mutations", options["rothberg-mutations"].as<long>());
                    heuristic_params.add("fixing-fraction", options["rothberg-fixing-fraction"].as<double>());
                    heuristic_params.add("offset-init", options["rothberg-offset-init"].as<double>());
                    heuristic_params.add("offset-reduction", options["rothberg-offset-reduction"].as<double>());
                    heuristic_params.add("offset-minimum", options["rothberg-offset-minimum"].as<double>());

                    // Instantiate the heuristic
                    heuristic = new orcs::Rothberg(&problem, &pool, &heuristic_params);

                }
            }
        }

        // Heuristic
        long heuristic_frequency = options["heuristic-frequency"].as<long>();
        problem.cplex.use(orcs::HeuristicCallback::create_instance(env, heuristic,
                heuristic_frequency, &timer, time_limit));

        // Resume the optimization process (2nd phase: heuristic)
        timer.start();
        problem.cplex.solve();
        timer.stop();

        // Get result
        Result result_after_heuristic = get_result(problem, pool, timer);

        // Display results
        print_results(problem, result_before_heuristic, result_after_heuristic, options);

        // Write best solution found
        if (options.count("solution") > 0) {
            std::string output_file = options["solution"].as<std::string>() + ".sol";
            problem.cplex.writeSolution(output_file.c_str());
        }

        // Free resources
        if (heuristic != nullptr) {
            delete heuristic;
            heuristic = nullptr;
        }

    } catch (const cxxopts::OptionException& e) {
        std::cerr << "Syntax error." << std::endl;
        std::cerr << e.what() << std::endl;
        std::cerr << "Type the following command for a correct usage:" << std::endl;
        std::cerr << argv[0] << " --help" << std::endl << std::endl;
        return EXIT_FAILURE;
    } catch (const IloException& e) {
        std::cerr << "CPLEX error." << std::endl;
        std::cerr << e << std::endl;
        std::cerr << "Type the following command for a correct usage:" << std::endl;
        std::cerr << argv[0] << " --help" << std::endl << std::endl;
        return EXIT_FAILURE;
    } catch (const std::string& e) {
        std::cerr << e << std::endl;
        std::cerr << "Type the following command for a correct usage." << std::endl;
        std::cerr << argv[0] << " --help" << std::endl << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unexpected error." << std::endl;
        std::cerr << "Type the following command for a correct usage." << std::endl;
        std::cerr << argv[0] << " --help" << std::endl << std::endl;
        return EXIT_FAILURE;
    }

    // Deallocate resources
    env.end();

    return EXIT_SUCCESS;
}

cxxopts::Options create_command_parser(int argc, char** argv) {

    cxxopts::Options options(argv[0], "MIP Polishing Heuristics\n");

    options.add_options()
            ("h,help", "Show this help message and exit.")
            ("f,file", "Name of the file containing the model. Valid suffixes "
                     "are .MPS and .LP. Files can be compressed, so the additional "
                     "suffix .GZ is accepted.",
             cxxopts::value<std::string>(), "FILE");

    options.add_options("Printing")
            ("v,verbose", "Display the progress of the optimization process "
                    "throughout its running.")
            ("d,details", "Set the level of details to show at the end of the "
                    "the optimization process. Valid values are: 0, 1, 2, 3 and 4.",
                    cxxopts::value<int>()->default_value("1"), "VALUE")
            ("s,solution", "Name of the file to save with best solution found.",
             cxxopts::value<std::string>(), "FILE");

    options.add_options("General")
            ("seed", "Set the seed used to initialize the random number generator "
                     "used by CPLEX solver and MIP heuristics.",
             cxxopts::value<unsigned long>()->default_value("0"), "VALUE")
            ("heuristic", "The improvement MIP heuristic to use after the a given "
                     "number of MIP nodes (defined by the parameter --nodes) be "
                     "explored. Valid values are: none, cplex-polishing, rothberg "
                     "and maravilha.",
             cxxopts::value<std::string>()->default_value("none"), "VALUE")
            ("heuristic-trigger-nodes", "Number of MIP nodes explored before start using the MIP "
                     "heuristic (if any is set). If not set, this trigger is disabled.",
             cxxopts::value<long>(), "VALUE")
            ("heuristic-trigger-time", "Time spent with default CPLEX before start using the MIP "
                     "heuristic (if any is set). If not set, this trigger is disabled.",
             cxxopts::value<double>(), "VALUE")
            ("heuristic-frequency", "Frequency the heuristic is called. For example: if set to 100, "
                     "and it is called the first time at node 1000, then it will be called at nodes "
                     "1100, 1200 and so on. If set to zero, the heuristic will not be called.",
             cxxopts::value<long>()->default_value("1"), "VALUE")
            ("heuristic-nodes-limit", "Additional MIP nodes to continue the optimization "
                     "process using the MIP heuristic. If not set, this stopping criterion is ignored.",
             cxxopts::value<long>(), "VALUE")
            ("heuristic-proportional-time-limit", "Additional time to continue the optimization process "
                     "using the MIP heuristic. It the optimization process spends 100 seconds solving "
                     "the first initial MIP nodes before start using the MIP heuristic and this parameter "
                     "is set to 0.5, then the optimization process will continue for another 0.5 x 100 = 50 "
                     "seconds performing the heuristic search. If not set, this stopping criterion is ignored.",
             cxxopts::value<double>(), "VALUE")
            ("heuristic-absolute-time-limit", "Additional time to continue the optimization process "
                     "using the MIP heuristic. It this parameter is set to 100, then the optimization process "
                     "will continue for another 100 seconds performing the heuristic search. If not set, this "
                     "stopping criterion is ignored.",
             cxxopts::value<double>(), "VALUE")
            ("submip-nodes-limit", "Maximum number of MIP nodes explored by each sub-MIP "
                     "problem solved by a MIP heuristic.",
             cxxopts::value<long>()->default_value("500"), "VALUE")
            ("submip-nodes-unsuccessful", "Maximum number of MIP nodes explored "
                     "without improvement in the sub-MIP incumbent solution. If not set, "
                     "this stopping criteria is ignored.",
             cxxopts::value<long>(), "VALUE")
            ("pool-size", "The maximum number of solutions kept in the pool of solutions.",
             cxxopts::value<long>()->default_value("40"), "VALUE");

    options.add_options("Maravilha's heuristic")
            ("maravilha-iterations", "Number of sub-MIPs to solve each time Maravilha's MIP "
                     "heuristic is performed.",
             cxxopts::value<long>()->default_value("1"), "VALUE")
            ("maravilha-submip-min", "The minimum proportion of binary variables not fixed "
                     "on sub-MIP problems.",
             cxxopts::value<double>()->default_value("0.00"), "VALUE")
            ("maravilha-submip-max", "The maximum proportion of binary variables not fixed "
                     "on sub-MIP problems.",
             cxxopts::value<double>()->default_value("0.65"), "VALUE")
            ("maravilha-offset", "Value used to auto-adjust (increase/decrease) the size limits of sub-MIPs. "
                     "It must be a value between 0 and 1.",
             cxxopts::value<double>()->default_value("0.45"), "VALUE");

    options.add_options("Rothberg's heuristic")
            ("rothberg-recombinations", "Number of recombination sub-MIP problems solved "
                     "at each time Rothberg's MIP heuristic is performed.",
             cxxopts::value<long>()->default_value("40"), "VALUE")
            ("rothberg-mutations", "Number of mutation sub-MIP problems solved at each "
                     "time Rothberg's MIP heuristic is performed.",
             cxxopts::value<long>()->default_value("20"), "VALUE")
            ("rothberg-fixing-fraction", "Initial value of the fixing fraction. It must "
                     "be a value between 0 and 1.",
             cxxopts::value<double>()->default_value("0.5"), "VALUE")
            ("rothberg-offset-init", "Value used to auto-adjust (increase/decrease) fixing fraction of "
                     "mutation sub-MIPs. It must be a value between 0 and 1.",
             cxxopts::value<double>()->default_value("0.20"), "VALUE")
            ("rothberg-offset-reduction", "The offset value is reduced by (100 x <VALUE>)% "
                     "after all mutation phase. It must be a value between 0 and 1.",
             cxxopts::value<double>()->default_value("0.25"), "VALUE")
            ("rothberg-offset-minimum", "The lowest value the offset can take. It must be "
                     "a value between 0 and 1.",
             cxxopts::value<double>()->default_value("0.01"), "VALUE");

    options.parse(argc, argv);
    return options;
}

void print_command_help(const cxxopts::Options& options, std::ostream& output) {
    output << options.help({"","Printing","General","Maravilha's heuristic","Rothberg's heuristic"})
           << std::endl;
}

Result get_result(const orcs::ProblemData& problem, const orcs::SolutionPool& pool, const cxxtimer::Timer& timer) {
    Result result;
    result.status = problem.cplex.getStatus();
    result.mip_nodes_explored = problem.cplex.getNnodes64();
    result.runtime = timer.count<std::chrono::milliseconds>() / 1000.0;

    result.objective_value = (problem.objective.getSense() == IloObjective::Minimize ?
                              std::numeric_limits<double>::max() : -std::numeric_limits<double>::max());

    if (result.status == IloAlgorithm::Status::Feasible ||
        result.status == IloAlgorithm::Status::Optimal) {
        result.objective_value = problem.cplex.getObjValue();
    }

    result.pool_size = pool.size();

    return result;
}

void print_results(orcs::ProblemData& problem, const Result& before, const Result& after, const cxxopts::Options& options) {
    if (options.count("details") > 0) {
        switch (options["details"].as<int>()) {
            case 0:
                // Do nothing.
                break;

            case 1:
                print_details_1(problem, before, after, options);
                break;

            case 2:
                print_details_2(problem, before, after, options);
                break;

            case 3:
                print_details_3(problem, before, after, options);
                break;

            case 4:
                print_details_4(problem, before, after, options);
                break;

            default:
                print_details_1(problem, before, after, options);
                break;
        }
    }
}

void print_details_1(orcs::ProblemData& problem, const Result& before, const Result& after, const cxxopts::Options& options) {
    switch (after.status) {
        case IloAlgorithm::Status::Unknown:
            std::printf("?\n");
            break;
        case IloAlgorithm::Status::Feasible:
            std::printf("%.5lf\n", after.objective_value);
            break;
        case IloAlgorithm::Status::Optimal:
            std::printf("%.5lf\n", after.objective_value);
            break;
        case IloAlgorithm::Status::Infeasible:
            std::printf("?\n");
            break;
        case IloAlgorithm::Status::Unbounded:
            std::printf("?\n");
            break;
        case IloAlgorithm::Status::InfeasibleOrUnbounded:
            std::printf("?\n");
            break;
        case IloAlgorithm::Status::Error:
            std::printf("?\n");
            break;
        default:
            std::printf("?\n");
    }
}

void print_details_2(orcs::ProblemData& problem, const Result& before, const Result& after, const cxxopts::Options& options) {
    switch (after.status) {
        case IloAlgorithm::Status::Unknown:
            std::printf("Unknown\n");
            break;
        case IloAlgorithm::Status::Feasible:
            std::printf("Feasible %.5lf\n", after.objective_value);
            break;
        case IloAlgorithm::Status::Optimal:
            std::printf("Optimal %.5lf\n", after.objective_value);
            break;
        case IloAlgorithm::Status::Infeasible:
            std::printf("Infeasible\n");
            break;
        case IloAlgorithm::Status::Unbounded:
            std::printf("Unbounded\n");
            break;
        case IloAlgorithm::Status::InfeasibleOrUnbounded:
            std::printf("Infeasible_or_Unbounded\n");
            break;
        case IloAlgorithm::Status::Error:
            std::printf("Error\n");
            break;
        default:
            std::printf("Unknown\n");
    }
}

void print_details_3(orcs::ProblemData& problem, const Result& before, const Result& after, const cxxopts::Options& options) {

    std::string status = "Unknown";
    switch (after.status) {
        case IloAlgorithm::Status::Unknown:
            status = "Unknown";
            break;
        case IloAlgorithm::Status::Feasible:
            status = "Feasible";
            break;
        case IloAlgorithm::Status::Optimal:
            status = "Optimal";
            break;
        case IloAlgorithm::Status::Infeasible:
            status = "Infeasible";
            break;
        case IloAlgorithm::Status::Unbounded:
            status = "Unbounded";
            break;
        case IloAlgorithm::Status::InfeasibleOrUnbounded:
            status = "Infeasible_or_Unbounded";
            break;
        case IloAlgorithm::Status::Error:
            status = "Error";
            break;
    }

    // Status
    std::printf("%s ", status.c_str());

    // Objective
    if (after.status == IloAlgorithm::Status::Feasible || after.status == IloAlgorithm::Status::Optimal) {
        std::printf("%.5lf ", after.objective_value);
    } else {
        std::printf("? ");
    }

    // Pool size
    std::printf("%zu ", after.pool_size);

    // MIP nodes
    std::printf("%lld ", after.mip_nodes_explored);

    // Time
    std::printf("%.3lf\n", after.runtime);
}

void print_details_4(orcs::ProblemData& problem, const Result& before, const Result& after, const cxxopts::Options& options) {
    std::string status = "Unknown";
    switch (after.status) {
        case IloAlgorithm::Status::Unknown:
            status = "Unknown";
            break;
        case IloAlgorithm::Status::Feasible:
            status = "Feasible";
            break;
        case IloAlgorithm::Status::Optimal:
            status = "Optimal";
            break;
        case IloAlgorithm::Status::Infeasible:
            status = "Infeasible";
            break;
        case IloAlgorithm::Status::Unbounded:
            status = "Unbounded";
            break;
        case IloAlgorithm::Status::InfeasibleOrUnbounded:
            status = "Infeasible_or_Unbounded";
            break;
        case IloAlgorithm::Status::Error:
            status = "Error";
            break;
    }

    std::printf("======================================================================\n");
    std::printf("SUMMARY\n");
    std::printf("======================================================================\n");
    std::printf("Status:                           %s\n", status.c_str());

    if (before.status == IloAlgorithm::Status::Feasible || before.status == IloAlgorithm::Status::Optimal) {
        std::printf("Best solution (before heuristic): %.5lf\n", before.objective_value);
    } else {
        std::printf("Best solution (before heuristic): ---\n");
    }

    if (after.status == IloAlgorithm::Status::Feasible || after.status == IloAlgorithm::Status::Optimal) {
        std::printf("Best solution (after heuristic):  %.5lf\n", after.objective_value);
    } else {
        std::printf("Best solution (after heuristic):  ---\n");
    }

    std::printf("Pool size (before heuristic):     %zu\n", before.pool_size);
    std::printf("Pool size (after heuristic):      %zu\n", after.pool_size);

    std::printf("MIP nodes (before heuristic):     %lld\n", before.mip_nodes_explored);
    std::printf("MIP nodes (using heuristic):      %lld\n", after.mip_nodes_explored - before.mip_nodes_explored);
    std::printf("MIP nodes (total):                %lld\n", after.mip_nodes_explored);

    std::printf("Time in sec. (before heuristic):  %.3lf\n", before.runtime);
    std::printf("Time in sec. (using heuristic):   %.3lf\n", (after.runtime - before.runtime));
    std::printf("Time in sec. (total):             %.3lf\n", after.runtime);
    std::printf("======================================================================\n\n");
}
