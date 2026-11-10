#pragma once

#include <string>
#include <vector>
#include <map>
#include "flowtester/step.hpp"

namespace flowtester {
    struct Job {
        std::string id;
        std::string name;
        std::vector<std::string> runsOn;
        std::vector<std::string> needs;
        std::map<std::string, std::string> permissions;
        std::map<std::string, std::string> env;
        std::vector<Step> steps;
        std::map<std::string, std::string> outputs;
        std::string ifCondition;
        std::string workingDirectory;
        int timeoutMinutes = 360;
        bool continueOnError = false;

        enum class State {
            Pending,
            Queued,
            Running,
            Success,
            Failure,
            Skipped,
            Cancelled
        };

        State state = State::Pending;

    };
}