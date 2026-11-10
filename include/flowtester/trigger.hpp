#pragma once

#include <string>
#include <vector>
#include <map>

namespace flowtester {
    struct Trigger {
        std::string event;
        std::vector<std::string> types; //optional only for pull_request event
        std::vector<std::string> branches;
        std::vector<std::string> paths;
        std::vector<std::string> branchIgnore;
        std::vector<std::string> pathIgnore;
        std::map<std::string, std::string> inputs;
        std::string ifCondition;
    };
}