#include "mlfq.h"
#include <vector>
#include <algorithm>

using namespace std;

// Non-preemptive Shortest Job First scheduling strategy
class MinimalJobScheduler final : public SchedulingStrategy {
    vector<int> waitingList;

    static int findTaskWithLeastWork(const vector<int>& candidates, const vector<Task>& tasks){
        int selected = -1;
        for (int taskId : candidates){
            if (selected == -1) {
                selected = taskId;
            } else {
                if (tasks[taskId].timeLeft < tasks[selected].timeLeft) {
                    selected = taskId;
                } else if (tasks[taskId].timeLeft == tasks[selected].timeLeft) {
                    if (tasks[taskId].arrivalMoment < tasks[selected].arrivalMoment) {
                        selected = taskId;
                    } else if (tasks[taskId].arrivalMoment == tasks[selected].arrivalMoment && 
                               tasks[taskId].identifier < tasks[selected].identifier) {
                        selected = taskId;
                    }
                }
            }
        }
        return selected;
    }

    static void removeFromList(vector<int>& list, int value){
        auto position = find(list.begin(), list.end(), value);
        if (position != list.end()) list.erase(position);
    }

public:
    explicit MinimalJobScheduler(vector<Task>& tasks)
        : SchedulingStrategy(tasks) {}

    void addToQueue(int taskId) override { waitingList.push_back(taskId); }
    bool hasWaitingTasks() const override { return !waitingList.empty(); }

    int selectNextTask(int currentTaskId) override {
        if (currentTaskId != -1) return currentTaskId;
        if (waitingList.empty()) return -1;
        int nextTask = findTaskWithLeastWork(waitingList, taskList);
        removeFromList(waitingList, nextTask);
        return nextTask;
    }

    void processTimeUnit(int) override {}
    void handleTaskExit(int) override {}

    void purgeTask(int taskId) override {
        auto position = find(waitingList.begin(), waitingList.end(), taskId);
        if (position != waitingList.end()) waitingList.erase(position);
    }

    void updateWaitingTimes(int runningTaskId) override {
        for (int taskId : waitingList) {
            if (taskId != runningTaskId) taskList[taskId].delayAccumulated++;
        }
    }
};

std::unique_ptr<SchedulingStrategy> createShortestJobStrategy(std::vector<Task>& tasks){
    return std::make_unique<MinimalJobScheduler>(tasks);
}
