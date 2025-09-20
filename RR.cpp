#include "mlfq.h"
#include <queue>
#include <unordered_map>

using namespace std;

// Round-Robin scheduling strategy implementation per level
class CircularScheduler final : public SchedulingStrategy {
    // Ready queue for THIS level using queue instead of list
    queue<int> readyQueue;
    // Time quota remaining per task ID at THIS level
    unordered_map<int,int> timeQuota;
    // Base time slice for THIS level
    int baseTimeSlice;
    // Preemption flag: task ID that must yield CPU on next selectNextTask
    int mustYield = -1;

public:
    CircularScheduler(vector<Task>& allTasks, int timeSlice)
        : SchedulingStrategy(allTasks), baseTimeSlice(timeSlice) {}

    void addToQueue(int taskId) override {
        readyQueue.push(taskId);
        if (!timeQuota.count(taskId)) timeQuota[taskId] = baseTimeSlice;
    }

    bool hasWaitingTasks() const override { return !readyQueue.empty(); }

    int selectNextTask(int currentTaskId) override {
        // No current task running, pick first from queue
        if (currentTaskId == -1) {
            if (readyQueue.empty()) return -1;
            int taskId = readyQueue.front(); 
            readyQueue.pop();
            return taskId;
        }

        // Current task marked for yielding, force context switch
        if (mustYield == currentTaskId) {
            mustYield = -1;
            if (!readyQueue.empty()) { 
                int nextTask = readyQueue.front(); 
                readyQueue.pop(); 
                return nextTask; 
            }
            return -1; // Switch even if queue empty
        }

        // Continue with current task
        return currentTaskId;
    }

    void processTimeUnit(int taskId) override {
        if (taskId == -1) return;

        // Consume one time unit from current level's quota
        if (--timeQuota[taskId] == 0 && taskList[taskId].timeLeft > 0) {
            // Time slice expired and task not finished
            timeQuota[taskId] = baseTimeSlice;                              // reset quota
            taskList[taskId].tier = std::min(4, taskList[taskId].tier + 1); // level degradation
            // Don't re-queue here; global MLFQ decides new level
            mustYield = taskId; // mark for yielding CPU on next selectNextTask
        }
    }

    void handleTaskExit(int taskId) override {
        if (taskId == -1) return;
        // Reset task's time quota
        timeQuota[taskId] = baseTimeSlice;
        // Don't re-queue in this strategy; MLFQ decides level
    }

    void purgeTask(int taskId) override {
        // Normally running task isn't in queue, but remove for robustness
        queue<int> tempQueue;
        while (!readyQueue.empty()) {
            int current = readyQueue.front();
            readyQueue.pop();
            if (current != taskId) {
                tempQueue.push(current);
            }
        }
        readyQueue = tempQueue;
        timeQuota.erase(taskId);
    }

    void updateWaitingTimes(int runningTaskId) override {
        queue<int> tempQueue = readyQueue;
        while (!tempQueue.empty()) {
            int taskId = tempQueue.front();
            tempQueue.pop();
            if (taskId != runningTaskId) taskList[taskId].delayAccumulated++;
        }
    }
};

std::unique_ptr<SchedulingStrategy> createRoundRobinStrategy(std::vector<Task>& tasks, int timeQuantum){
    return std::make_unique<CircularScheduler>(tasks, timeQuantum);
}
