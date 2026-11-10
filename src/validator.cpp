#include "flowtester/validator.hpp"

#include <set>

namespace flowtester {
    std::vector<std::string> WorkflowValidator::validate(const Workflow& workflow) {
        std::vector<std::string> errors;

        if (workflow.name.empty()) {
            errors.push_back("Workflow name is missing.");
        }

        if (workflow.jobs.empty()) {
            errors.push_back("Workflow must contain at least one job.");
        }

        std::set<std::string> jobIds;
        for (const auto& job : workflow.jobs) {
            if (job.id.empty()) {
                errors.push_back("Job ID is missing.");
            } else if (!jobIds.insert(job.id).second) {
                errors.push_back("Duplicate job ID '" + job.id + "'.");
            }

            if (job.steps.empty()) {
                errors.push_back(
                    "Job '" + job.id + "' must contain at least one step."
                );
            }
        }

        for (const auto& trigger : workflow.triggers) {
            if (trigger.event.empty()) {
                errors.push_back("Trigger event is missing.");
            }
        }

        for (const auto& job : workflow.jobs) {
            for (const auto& step : job.steps) {
                if (step.id.empty()) {
                    errors.push_back(
                        "Step ID is missing in job '" + job.id + "'."
                    );
                }

                if (step.run.empty() && step.uses.empty()) {
                    errors.push_back(
                        "Step '" + step.id + "' in job '" + job.id +
                        "' must have either 'run' or 'uses' defined."
                    );
                } else if (!step.run.empty() && !step.uses.empty()) {
                    errors.push_back(
                        "Step '" + step.id + "' in job '" + job.id +
                        "' must not define both 'run' and 'uses'."
                    );
                }
            }
        }

        for (const auto& job : workflow.jobs) {
            for (const auto& dependency : job.needs) {
                if (!jobIds.contains(dependency)) {
                    errors.push_back(
                        "Job '" + job.id + "' depends on missing job '" +
                        dependency + "'."
                    );
                }
            }
        }

        return errors;
    }
}