#pragma once

#include <string>
#include <vector>

#include "flowtester/workflow.hpp"


namespace flowtester {
    class WorkflowValidator {
        public:
        std::vector<std::string> validate(const Workflow& workflow);
    };
}