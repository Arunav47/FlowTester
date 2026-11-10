#pragma once

#include <string>
#include <vector>
#include <map>

namespace flowtester {
    struct Step {
        std::string id;
        std::string name;
        std::string run;
        std::string uses;
        std::map<std::string, std::string> env;
        std::string ifCondition;
        std::map<std::string, std::string> with;
        std::string shell;
        std::string workingDirectory;
        int timeoutMinutes = 0;
        bool continueOnError = false;
    };
}