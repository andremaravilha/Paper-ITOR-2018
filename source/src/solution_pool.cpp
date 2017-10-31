#include "solution_pool.h"
#include <limits>
#include <cmath>
#include <algorithm>


orcs::SolutionPool::SolutionPool(IloEnv& env, IloObjective::Sense sense, 
        std::size_t max_size, bool sorted) : 
        env_(env), sense_(sense), max_size_(max_size), sorted_(sorted), 
        next_age_(std::numeric_limits<unsigned long long>::max())
{
    entries_.reserve(max_size_);
}

orcs::SolutionPool::~SolutionPool() {
    for (auto& entry : entries_) {
        entry.solution.end();
    }
}

const std::vector<orcs::SolutionPool::Entry>& orcs::SolutionPool::get_entries() const {
    return entries_;
}

bool orcs::SolutionPool::add_entry(const IloNumArray& solution, const IloNum value) {
    
    // Used to identify the worst solution into the pool
    std::size_t worst_idx = 0;
    IloNum worst_val = (sense_ == IloObjective::Sense::Minimize ? 
        -std::numeric_limits<double>::max() : std::numeric_limits<double>::max());
    
    // Check similarity and find the worst solution into the pool
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        
        // Check similarity
        bool equal = true;
        for (std::size_t j = 0; j < solution.getSize(); ++j) {
            if (std::abs(solution[j] - entries_[i].solution[j]) > SIMILARITY_THRESHOLD) {
                equal = false;
                break;
            }
        }
        
        if (equal) {
            return false;
        }
        
        // Find the worst
        if (((sense_ == IloObjective::Sense::Minimize) ? 
                (entries_[i].value > worst_val) : 
                (entries_[i].value < worst_val))) {
            worst_idx = i;
            worst_val = entries_[i].value;
        }
    }
    
    // Try to insert the entry into the pool
    bool inserted = false;
    
    // Check whether the pool is full
    if (entries_.size() < max_size_) {
        
        entries_.emplace_back(IloNumArray(env_, solution.getSize()), value, next_age_);
        for (std::size_t i = 0; i < solution.getSize(); ++i) {
            entries_[entries_.size() - 1].solution[i] = solution[i];
        }
        
        inserted = true;
        
    // Check if the new entry is better than the worst entry into the pool
    } else if (((sense_ == IloObjective::Sense::Minimize) ? 
            (value < worst_val) : 
            (value > worst_val))) {

        entries_[worst_idx].value = value;
        entries_[worst_idx].age = next_age_;
        for (std::size_t i = 0; i < solution.getSize(); ++i) {
            entries_[worst_idx].solution[i] = solution[i];
        }
        
        inserted = true;
    }
    
    // Sort the pool
    if (inserted && sorted_) {
        std::sort(entries_.begin(), entries_.end(), 
                [this](const Entry& entry1, const Entry& entry2){
                    if (std::abs(entry1.value - entry2.value) < SIMILARITY_THRESHOLD) {
                        return entry1.age < entry2.age;
                    } else if (sense_ == IloObjective::Sense::Minimize) {
                        return entry1.value < entry2.value;
                    } else {
                        return entry1.value > entry2.value;
                    }
                });
    }
    
    // Update the next age
    if (inserted) {
        --next_age_;
    }
    
    return inserted;
}

std::size_t orcs::SolutionPool::size() const {
    return entries_.size();
}

std::size_t orcs::SolutionPool::max_size() const {
    return max_size_;
}
