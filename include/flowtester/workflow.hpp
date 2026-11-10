#pragma once

#include "flowtester/job.hpp"
#include "flowtester/trigger.hpp"

#include <string>
#include <vector>

namespace flowtester {
    class Workflow
    {
        public:
            std::string name;
            std::string description;
            std::vector<Trigger> triggers;
            std::map<std::string, std::string> permissions;
            std::map<std::string, std::string> env;
            std::vector<Job> jobs;
    };
}