#pragma once

#include "flowtester/workflow.hpp"

#include <string>


namespace flowtester {
    class WorkflowParser {
        public:
            Workflow parseFile(const std::string& filePath);
    };
}