#include "mlfq.h"
#include <vector>
#include <algorithm>

using namespace std;

// Preemptive Shortest Remaining Time First scheduling strategy
class DynamicShortestScheduler final : public SchedulingStrategy {
    vector<int> candidateList;

    static int findOptimalTask(const vector<int>& options, int current, const vector<Task>& tasks){
        int bestChoice = current;
        for (int taskId : options){
            if (bestChoice == -1 || tasks[taskId].timeLeft < tasks[bestChoice].timeLeft) {
                bestChoice = taskId;
            } else if (bestChoice != -1 && tasks[taskId].timeLeft == tasks[bestChoice].timeLeft) {
                if (tasks[taskId].arrivalMoment < tasks[bestChoice].arrivalMoment) {
                    bestChoice = taskId;
                } else if (tasks[taskId].arrivalMoment == tasks[bestChoice].arrivalMoment && 
                           tasks[taskId].identifier < tasks[bestChoice].identifier) {
                    bestChoice = taskId;
                }
            }
        }
        return bestChoice;
    }

    static void removeElement(vector<int>& list, int taskId){
        auto position = find(list.begin(), list.end(), taskId);
        if (position != list.end()) list.erase(position);
    }

public:
    explicit DynamicShortestScheduler(vector<Task>& tasks)
        : SchedulingStrategy(tasks) {}

    void addToQueue(int taskId) override { candidateList.push_back(taskId); }
    bool hasWaitingTasks() const override { return !candidateList.empty(); }

    int selectNextTask(int currentTaskId) override {
        if (candidateList.empty() && currentTaskId == -1) return -1;
        int optimalTask = findOptimalTask(candidateList, currentTaskId, taskList);
        if (optimalTask == currentTaskId) return currentTaskId;
        if (currentTaskId != -1) candidateList.push_back(currentTaskId);
        removeElement(candidateList, optimalTask);
        return optimalTask;
    }

    void processTimeUnit(int) override {}
    void handleTaskExit(int) override {}

    void purgeTask(int taskId) override { removeElement(candidateList, taskId); }

    void updateWaitingTimes(int activeTaskId) override {
        for (int taskId : candidateList) {
            if (taskId != activeTaskId) taskList[taskId].delayAccumulated++;
        }
    }
};

std::unique_ptr<SchedulingStrategy> createPreemptiveShortestStrategy(std::vector<Task>& tasks){
    return std::make_unique<DynamicShortestScheduler>(tasks);
}