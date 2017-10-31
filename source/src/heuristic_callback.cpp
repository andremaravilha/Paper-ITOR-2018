#include "heuristic_callback.h"


IloCplex::Callback orcs::HeuristicCallback::create_instance(IloEnv& env, 
        Heuristic* heuristic, unsigned long long frequency, const cxxtimer::Timer* timer,
        double time_limit) {
    return (IloCplex::Callback(new (env) orcs::HeuristicCallback(env, heuristic, 
            frequency, timer, time_limit)));
}

orcs::HeuristicCallback::HeuristicCallback(IloEnv& env, Heuristic* heuristic, 
        unsigned long long frequency, const cxxtimer::Timer* timer,
        double time_limit) :
    IloCplex::HeuristicCallbackI(env), heuristic_(heuristic), 
    frequency_(frequency), timer_(timer), time_limit_(time_limit)
{
    // It does nothing here.
}

IloCplex::CallbackI* orcs::HeuristicCallback::duplicateCallback() const {
    return (new (getEnv()) orcs::HeuristicCallback(*this));
}

void orcs::HeuristicCallback::main() {
    if (frequency_ > 0ULL && heuristic_ != nullptr) {
        if (getNnodes64() % frequency_ == 0) {
            heuristic_->run(this, timer_, time_limit_);
        }
    }
}
