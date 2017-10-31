#include "abort_callback.h"
#include <chrono>


IloCplex::Callback orcs::AbortCallback::create_instance(IloEnv& env, const cxxtimer::Timer* timer,
        double time_limit, unsigned long long nodes_limit, unsigned long long nodes_unsuccessful)
{
    return (IloCplex::Callback(new (env) orcs::AbortCallback(env, timer, time_limit,
            nodes_limit, nodes_unsuccessful)));
}

orcs::AbortCallback::AbortCallback(IloEnv& env, const cxxtimer::Timer* timer, double time_limit,
        unsigned long long nodes_limit, unsigned long long nodes_unsuccessful) :
    IloCplex::MIPInfoCallbackI(env), timer_(timer), time_limit_(time_limit),
    nodes_limit_(nodes_limit), initialized_(false), aborted_(false),
    nodes_unsuccessful_(nodes_unsuccessful)
{
    // It does nothing here.
}

IloCplex::CallbackI* orcs::AbortCallback::duplicateCallback() const {
    return (new (getEnv()) orcs::AbortCallback(*this));
}

void orcs::AbortCallback::main() {

    // Check if the optimization process has been aborted
    if (aborted_) {
        return;
    }

    // Abort, if time limit has been reached
    if (timer_ != nullptr) {
        if ((timer_->count<std::chrono::milliseconds>() / 1000.0) >= time_limit_) {
            aborted_ = true;
            abort();
            return;
        }
    }

    // Abort, if maximum number of MIP nodes has been explored
    if (getNnodes64() >= nodes_limit_) {
        aborted_ = true;
        abort();
        return;
    }

    // Abort, if maximum number of MIP nodes without improvement has been reached
    if (!initialized_) {
        initialized_ = true;
        obj_last_incumbent_ = getIncumbentObjValue();
        nodes_last_incumbent_ = getNnodes64();

    } else {

        IloNum obj_current_incumbent = getIncumbentObjValue();
        IloInt64 nnodes = getNnodes64();

        if (obj_last_incumbent_ - obj_current_incumbent > 1e-5) {
            obj_last_incumbent_ = obj_current_incumbent;
            nodes_last_incumbent_ = getNnodes64();

        } else if (nnodes - nodes_last_incumbent_ > nodes_unsuccessful_) {
            aborted_ = true;
            abort();
            return;
        }
    }

}
