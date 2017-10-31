#ifndef ORCS_ABORT_CALLBACK_H
#define ORCS_ABORT_CALLBACK_H


#include <limits>
#include <ilcplex/ilocplex.h>
#include <cxxtimer.hpp>


ILOSTLBEGIN


namespace orcs {

    
/**
 * Callback class used to set custom stopping criteria for the optimization 
 * method.
 */
class AbortCallback : public IloCplex::MIPInfoCallbackI {

public:
    
    /**
     * This static method creates a new instance of this class and returns a 
     * handle for the instance.
     * 
     * @param   env
     *          CPLEX environment.
     * @param   timer
     *          Pointer to a timer used for checking time limit.
     * @param   time_limit
     *          Limits the total time (in secondes). When the time limit is
     *          reached, the optimization process stops. If set as a negative 
     *          value, this stopping criterion is ignored.
     * @param   nodes_limit
     *          Abort the optimization process when nodes_limit nodes are
     *          explored. If set as a negative value, this stopping criterion is 
     *          ignored.
     * @param   nodes_unsuccessful
     *          Abort the optimization process when nodes_unsuccessful have been
     *          explored and no improved solution was found.
     */
    static IloCplex::Callback create_instance(IloEnv& env, const cxxtimer::Timer* timer = nullptr,
            double time_limit = std::numeric_limits<double>::max(),
            unsigned long long nodes_limit = std::numeric_limits<unsigned long long>::max(),
            unsigned long long nodes_unsuccessful = std::numeric_limits<unsigned long long>::max());
    
protected:
    
    /**
     * This static method creates a new instance of this class and returns a 
     * handle for the instance.
     * 
     * @param   env
     *          CPLEX environment.
     * @param   timer
     *          Pointer to a timer used for checking time limit.
     * @param   time_limit
     *          Limits the total time (in seconds). When the time limit is
     *          reached, the optimization process stops. If set as a negative 
     *          value, this stopping criterion is ignored.
     * @param   nodes_limit
     *          Abort the optimization process when nodes_limit nodes are
     *          explored. If set as a negative value, this stopping criterion is 
     *          ignored.
     * @param   nodes_unsuccessful
     *          Abort the optimization process when nodes_unsuccessful have been
     *          explored and no improved solution was found.
     */
    AbortCallback(IloEnv& env, const cxxtimer::Timer* timer,
            double time_limit, unsigned long long nodes_limit,
            unsigned long long nodes_unsuccessful);

    IloCplex::CallbackI* duplicateCallback() const override;
    void main() override;
    
private:

    /*
     * Timer.
     */
    const cxxtimer::Timer* timer_;

    /*
     * Stop criteria.
     */
    double time_limit_;
    unsigned long long nodes_limit_;
    unsigned long long nodes_unsuccessful_;

    /*
     * Status
     */
    bool initialized_;
    bool aborted_;

    /*
     * Info about the incumbent solution.
     */
    IloNum obj_last_incumbent_;
    IloInt64 nodes_last_incumbent_;
    
};

}


#endif
