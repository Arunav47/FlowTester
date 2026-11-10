#include "parser.hpp"

#include <yaml-cpp/yaml.h>

#include <stdexcept>
#include <string>

namespace flowtester {

namespace {

// Read a node as a string, returning a default value if it is absent.
std::string readString(const YAML::Node& node, const std::string& key, const std::string& defaultValue = "") {
    const YAML::Node value = node[key];

    if (!value || value.IsNull()) {
        return defaultValue;
    }

    if (!value.IsScalar()) {
        throw std::runtime_error(
            "Expected a scalar value for '" + key + "'."
        );
    }

    return value.as<std::string>();
}

// Parse a string or sequence of strings into a vector.
std::vector<std::string> readStringList(const YAML::Node& node) {
    std::vector<std::string> values;

    if (!node || node.IsNull()) {
        return values;
    }

    if (node.IsScalar()) {
        values.push_back(node.as<std::string>());
    } else if (node.IsSequence()) {
        for (const auto& item : node) {
            if (!item.IsScalar()) {
                throw std::runtime_error(
                    "Expected each list item to be a scalar string."
                );
            }

            values.push_back(item.as<std::string>());
        }
    } else {
        throw std::runtime_error(
            "Expected a string or a sequence of strings."
        );
    }

    return values;
}

// Read a map of scalar values into std::map<std::string, std::string>.
std::map<std::string, std::string> readStringMap(const YAML::Node& node, const std::string& fieldName) {
    std::map<std::string, std::string> values;

    if (!node || node.IsNull()) {
        return values;
    }

    if (!node.IsMap()) {
        throw std::runtime_error(
            "Expected '" + fieldName + "' to be a YAML map."
        );
    }

    for (const auto& entry : node) {
        const std::string key = entry.first.as<std::string>();

        if (!entry.second.IsScalar()) {
            throw std::runtime_error(
                "Expected a scalar value for '" +
                fieldName + "." + key + "'."
            );
        }

        values[key] = entry.second.as<std::string>();
    }

    return values;
}

// Find the GitHub Actions "on" key.
// Some YAML parsers interpret an unquoted "on" as a Boolean key.
YAML::Node findOnNode(const YAML::Node& config) {
    if (!config || !config.IsMap()) {
        return YAML::Node{};
    }

    // First try the normal string key.
    YAML::Node onNode = config["on"];

    if (onNode.IsDefined()) {
        return onNode;
    }

    // Fallback: look through the root map for a Boolean key.
    for (const auto& entry : config) {
        if (entry.first.IsScalar()) {
            const std::string key = entry.first.Scalar();

            if (key == "true" || key == "1") {
                return entry.second;
            }
        }
    }

    return YAML::Node{};
}

void parseTriggerOptions(
    const YAML::Node& options,
    Trigger& trigger
) {
    if (!options || options.IsNull()) {
        return;
    }

    // A trigger may have no options, e.g.:
    // on:
    //   push:
    if (!options.IsMap()) {
        if (options.IsScalar()) {
            return;
        }

        throw std::runtime_error(
            "Trigger options must be a YAML map."
        );
    }

    if (options["types"]) {
        trigger.types = readStringList(options["types"]);
    }

    if (options["branches"]) {
        trigger.branches = readStringList(options["branches"]);
    }

    if (options["branches-ignore"]) {
        trigger.branchIgnore =
            readStringList(options["branches-ignore"]);
    }

    if (options["paths"]) {
        trigger.paths = readStringList(options["paths"]);
    }

    if (options["paths-ignore"]) {
        trigger.pathIgnore =
            readStringList(options["paths-ignore"]);
    }

    if (options["inputs"] && options["inputs"].IsMap()) {
        for (const auto& inputNode : options["inputs"]) {
            const std::string inputName =
                inputNode.first.as<std::string>();

            if (inputNode.second.IsScalar()) {
                trigger.inputs[inputName] =
                    inputNode.second.as<std::string>();
            } else {
                // Workflow dispatch inputs can contain structured
                // metadata. Preserve a simple string representation
                // for now where possible.
                trigger.inputs[inputName] =
                    YAML::Dump(inputNode.second);
            }
        }
    }

    if (options["if"]) {
        trigger.ifCondition =
            options["if"].as<std::string>();
    }
}

void parseTriggers(
    const YAML::Node& onNode,
    Workflow& workflow
) {
    if (!onNode || onNode.IsNull()) {
        return;
    }

    // Format 1:
    // on: push
    if (onNode.IsScalar()) {
        Trigger trigger;
        trigger.event = onNode.as<std::string>();
        workflow.triggers.push_back(trigger);
        return;
    }

    // Format 2:
    // on: [push, pull_request]
    if (onNode.IsSequence()) {
        for (const auto& eventNode : onNode) {
            if (!eventNode.IsScalar()) {
                throw std::runtime_error(
                    "Each workflow trigger must be a string."
                );
            }

            Trigger trigger;
            trigger.event = eventNode.as<std::string>();
            workflow.triggers.push_back(trigger);
        }

        return;
    }

    // Format 3:
    // on:
    //   push:
    //     branches: [main]
    if (onNode.IsMap()) {
        for (const auto& triggerNode : onNode) {
            Trigger trigger;

            trigger.event =
                triggerNode.first.as<std::string>();

            parseTriggerOptions(
                triggerNode.second,
                trigger
            );

            workflow.triggers.push_back(trigger);
        }

        return;
    }

    throw std::runtime_error(
        "Invalid workflow trigger configuration."
    );
}

} // namespace

Workflow WorkflowParser::parseFile(
    const std::string& filePath
) {
    Workflow workflow;
    YAML::Node config;

    // Load the YAML file.
    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error(
            "Failed to load workflow configuration '" +
            filePath + "': " + e.what()
        );
    }

    // The workflow root must be a map.
    if (!config.IsMap()) {
        throw std::runtime_error(
            "Invalid workflow configuration: root must be a YAML map."
        );
    }

    // The workflow must have a name.
    if (!config["name"] || !config["name"].IsScalar()) {
        throw std::runtime_error(
            "Workflow name is missing or invalid."
        );
    }

    workflow.name = config["name"].as<std::string>();

    // Optional description.
    if (config["description"]) {
        workflow.description =
            config["description"].as<std::string>();
    }

    // Triggers.
    parseTriggers(findOnNode(config), workflow);

    // Workflow-level permissions.
    if (config["permissions"]) {
        workflow.permissions = readStringMap(
            config["permissions"],
            "permissions"
        );
    }

    // Workflow-level environment variables.
    if (config["env"]) {
        workflow.env = readStringMap(
            config["env"],
            "env"
        );
    }

    // Jobs.
    const YAML::Node jobsNode = config["jobs"];

    if (jobsNode && !jobsNode.IsNull()) {
        if (!jobsNode.IsMap()) {
            throw std::runtime_error(
                "Invalid jobs configuration: expected a YAML map."
            );
        }

        for (const auto& jobNode : jobsNode) {
            if (!jobNode.first.IsScalar()) {
                throw std::runtime_error(
                    "Job ID must be a string."
                );
            }

            if (!jobNode.second.IsMap()) {
                throw std::runtime_error(
                    "Job '" + jobNode.first.as<std::string>() +
                    "' must be a YAML map."
                );
            }

            const YAML::Node jobConfig = jobNode.second;

            Job job;
            job.id = jobNode.first.as<std::string>();

            if (jobConfig["name"]) {
                job.name =
                    jobConfig["name"].as<std::string>();
            }

            // Job dependencies.
            if (jobConfig["needs"]) {
                job.needs = readStringList(
                    jobConfig["needs"]
                );
            }

            // Runner labels.
            if (jobConfig["runs-on"]) {
                job.runsOn = readStringList(
                    jobConfig["runs-on"]
                );
            }

            // Job environment variables.
            if (jobConfig["env"]) {
                job.env = readStringMap(
                    jobConfig["env"],
                    "job.env"
                );
            }

            // Job permissions.
            if (jobConfig["permissions"]) {
                job.permissions = readStringMap(
                    jobConfig["permissions"],
                    "job.permissions"
                );
            }

            // Job outputs.
            if (jobConfig["outputs"]) {
                job.outputs = readStringMap(
                    jobConfig["outputs"],
                    "job.outputs"
                );
            }

            if (jobConfig["if"]) {
                job.ifCondition =
                    jobConfig["if"].as<std::string>();
            }

            if (jobConfig["working-directory"]) {
                job.workingDirectory =
                    jobConfig["working-directory"]
                        .as<std::string>();
            }

            if (jobConfig["timeout-minutes"]) {
                job.timeoutMinutes =
                    jobConfig["timeout-minutes"].as<int>();
            }

            if (jobConfig["continue-on-error"]) {
                job.continueOnError =
                    jobConfig["continue-on-error"].as<bool>();
            }

            // Steps.
            const YAML::Node stepsNode = jobConfig["steps"];

            if (stepsNode && !stepsNode.IsNull()) {
                if (!stepsNode.IsSequence()) {
                    throw std::runtime_error(
                        "Invalid steps configuration for job '" +
                        job.id + "': expected a sequence."
                    );
                }

                for (const auto& stepNode : stepsNode) {
                    if (!stepNode.IsMap()) {
                        throw std::runtime_error(
                            "Each step in job '" + job.id +
                            "' must be a YAML map."
                        );
                    }

                    Step step;

                    if (stepNode["name"]) {
                        step.name =
                            stepNode["name"].as<std::string>();
                    }

                    if (stepNode["id"]) {
                        step.id =
                            stepNode["id"].as<std::string>();
                    }

                    if (stepNode["run"]) {
                        step.run =
                            stepNode["run"].as<std::string>();
                    }

                    if (stepNode["uses"]) {
                        step.uses =
                            stepNode["uses"].as<std::string>();
                    }

                    if (stepNode["env"]) {
                        step.env = readStringMap(
                            stepNode["env"],
                            "step.env"
                        );
                    }

                    if (stepNode["if"]) {
                        step.ifCondition =
                            stepNode["if"].as<std::string>();
                    }

                    if (stepNode["shell"]) {
                        step.shell =
                            stepNode["shell"].as<std::string>();
                    }

                    if (stepNode["working-directory"]) {
                        step.workingDirectory =
                            stepNode["working-directory"]
                                .as<std::string>();
                    }

                    if (stepNode["with"]) {
                        step.with = readStringMap(
                            stepNode["with"],
                            "step.with"
                        );
                    }

                    if (stepNode["timeout-minutes"]) {
                        step.timeoutMinutes =
                            stepNode["timeout-minutes"].as<int>();
                    }

                    if (stepNode["continue-on-error"]) {
                        step.continueOnError =
                            stepNode["continue-on-error"].as<bool>();
                    }

                    job.steps.push_back(step);
                }
            }

            // Add the job even when it has no steps.
            workflow.jobs.push_back(job);
        }
    }

    return workflow;
}

} // namespace flowtester