#include "mlfq.h"
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <array>

using namespace std;

// Core MLFQ algorithm implementation

struct SchedulingLevel {
    LevelConfiguration config;
    std::unique_ptr<SchedulingStrategy> scheduler;

    SchedulingLevel() = default;
    explicit SchedulingLevel(const LevelConfiguration& c, std::vector<Task>& tasks) : config(c) {
        switch (config.strategy) {
            case SchedulingMode::ROUND_ROBIN:   scheduler = createRoundRobinStrategy(tasks, config.timeSlice); break;
            case SchedulingMode::SHORTEST_FIRST:  scheduler = createShortestJobStrategy(tasks); break;
            case SchedulingMode::SHORTEST_REMAINING: scheduler = createPreemptiveShortestStrategy(tasks); break;
        }
    }
    bool hasWork() const { return scheduler->hasWaitingTasks(); }
};

static array<LevelConfiguration,4> defineAlgorithmScheme(char algorithmType){
    array<LevelConfiguration,4> configuration{};
    algorithmType = static_cast<char>(toupper(algorithmType));
    if (algorithmType == 'A'){
        configuration[0] = {SchedulingMode::ROUND_ROBIN, 1}; configuration[1] = {SchedulingMode::ROUND_ROBIN, 3};
        configuration[2] = {SchedulingMode::ROUND_ROBIN, 4}; configuration[3] = {SchedulingMode::SHORTEST_FIRST, 0};
    } else if (algorithmType == 'B'){
        configuration[0] = {SchedulingMode::ROUND_ROBIN, 2}; configuration[1] = {SchedulingMode::ROUND_ROBIN, 3};
        configuration[2] = {SchedulingMode::ROUND_ROBIN, 4}; configuration[3] = {SchedulingMode::SHORTEST_REMAINING, 0};
    } else if (algorithmType == 'C'){
        configuration[0] = {SchedulingMode::ROUND_ROBIN, 3}; configuration[1] = {SchedulingMode::ROUND_ROBIN, 5};
        configuration[2] = {SchedulingMode::ROUND_ROBIN, 6}; configuration[3] = {SchedulingMode::ROUND_ROBIN, 20};
    } else {
        throw runtime_error("Unknown algorithm scheme (A/B/C).");
    }
    return configuration;
}

static int locateHighestPriorityLevel(const array<SchedulingLevel,4>& levels){
    for (int i = 0; i < 4; ++i) if (levels[i].hasWork()) return i;
    return -1;
}

std::vector<Task> executeMLFQ(const std::vector<Task>& input, char algorithmType){
    vector<Task> taskList = input;

    // === Initialization: all tasks start at level 1 ===
    for (auto& task : taskList){
        task.timeLeft = task.serviceDuration;
        task.startMoment  = -1;
        task.finishMoment = -1;
        task.delayAccumulated = 0;
        task.tier = 1;  // top level
    }

    auto algorithmConfig = defineAlgorithmScheme(algorithmType);
    array<SchedulingLevel,4> schedulingLevels = {
        SchedulingLevel(algorithmConfig[0], taskList),
        SchedulingLevel(algorithmConfig[1], taskList),
        SchedulingLevel(algorithmConfig[2], taskList),
        SchedulingLevel(algorithmConfig[3], taskList),
    };

    // Stable arrival ordering by (arrivalMoment, identifier)
    vector<int> arrivalSequence(taskList.size());
    iota(arrivalSequence.begin(), arrivalSequence.end(), 0);
    sort(arrivalSequence.begin(), arrivalSequence.end(), [&](int x, int y){
        if (taskList[x].arrivalMoment != taskList[y].arrivalMoment) return taskList[x].arrivalMoment < taskList[y].arrivalMoment;
        return taskList[x].identifier < taskList[y].identifier;
    });
    size_t nextArrivalIndex = 0;

    int currentTime = 0, completedTasks = 0;
    int activeTaskId = -1, activeLevel = -1;

    auto assignToLevel = [&](int taskId){
        // Send to level indicated by task's current tier (1..4)
        int levelIndex = std::max(1, std::min(4, taskList[taskId].tier)) - 1;
        schedulingLevels[levelIndex].scheduler->addToQueue(taskId);
    };
    auto updateWaitingTasks = [&](int runningTask){
        for (int i = 0; i < 4; ++i) schedulingLevels[i].scheduler->updateWaitingTimes(runningTask);
    };

    while (completedTasks < static_cast<int>(taskList.size())){
        // Process arrivals at current time
        while (nextArrivalIndex < arrivalSequence.size() && taskList[arrivalSequence[nextArrivalIndex]].arrivalMoment == currentTime){
            assignToLevel(arrivalSequence[nextArrivalIndex]);
            ++nextArrivalIndex;
        }

        // Find highest priority level with ready tasks
        int topLevel = locateHighestPriorityLevel(schedulingLevels);
        if (topLevel == -1 && activeTaskId != -1) topLevel = activeLevel; // maintain level if already executing

        if (topLevel == -1){
            // CPU idle
            updateWaitingTasks(-1);
            ++currentTime;
            continue;
        }

        // Preemption due to higher priority level appearance
        if (activeTaskId != -1 && topLevel < activeLevel){
            if (taskList[activeTaskId].timeLeft > 0){
                // Re-queue at same level
                schedulingLevels[activeLevel].scheduler->addToQueue(activeTaskId);
            }
            activeTaskId = -1; activeLevel = -1;
        }
        if (activeTaskId == -1) activeLevel = topLevel;

        int selectedTask = schedulingLevels[activeLevel].scheduler->selectNextTask(activeTaskId);
        if (selectedTask != activeTaskId){
            activeTaskId = selectedTask;
            if (activeTaskId != -1 && taskList[activeTaskId].startMoment < 0) taskList[activeTaskId].startMoment = currentTime;
        }

        if (activeTaskId == -1){
            updateWaitingTasks(-1); ++currentTime; continue;
        }

        // Execute 1 time unit
        taskList[activeTaskId].timeLeft--;
        updateWaitingTasks(activeTaskId);

        if (taskList[activeTaskId].timeLeft == 0){
            // Task completed
            taskList[activeTaskId].finishMoment = currentTime + 1;
            schedulingLevels[activeLevel].scheduler->handleTaskExit(activeTaskId);
            schedulingLevels[activeLevel].scheduler->purgeTask(activeTaskId);
            activeTaskId = -1; activeLevel = -1; ++completedTasks;
        } else {
            // Task not finished: strategy might force context switch (e.g., RR quantum expiry)
            schedulingLevels[activeLevel].scheduler->processTimeUnit(activeTaskId);
            int nextSelected = schedulingLevels[activeLevel].scheduler->selectNextTask(activeTaskId);

            if (nextSelected != activeTaskId){
                // *** PREEMPTION (e.g., quantum expired in RR) ***
                schedulingLevels[activeLevel].scheduler->handleTaskExit(activeTaskId);

                // *** Level movement if applicable ***
                // RR already incremented taskList[activeTaskId].tier when quantum expired.
                schedulingLevels[activeLevel].scheduler->purgeTask(activeTaskId); // remove from current level
                assignToLevel(activeTaskId);                    // re-queue globally according to 'tier'

                activeTaskId = nextSelected;                      // might be -1 if no more here
                if (activeTaskId != -1 && taskList[activeTaskId].startMoment < 0)
                    taskList[activeTaskId].startMoment = currentTime + 1;
            }
        }

        ++currentTime;
    }

    return taskList;
}
