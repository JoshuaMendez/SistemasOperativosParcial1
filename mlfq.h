#pragma once
#include <string>
#include <vector>
#include <memory>
#include <iosfwd>

// =============================================================
// Task models and scheduler interface definitions
// =============================================================

class Task {
public:
    // Task identification and service parameters
    std::string identifier;   // task name (A, B, ...)
    int serviceDuration = 0;  // total execution time needed
    int arrivalMoment = 0;    // when task enters system

    // Scheduling metadata
    int tier  = 1;           // queue level 1..4
    int priority = 1;        // priority value (5 > 1)

    // Execution state and metrics
    int timeLeft = 0;        // remaining execution time
    int startMoment  = -1;   // first execution timestamp
    int finishMoment = -1;   // completion timestamp
    int delayAccumulated = 0; // total waiting time

    Task() = default;
};

// ========= Scheduling strategy types and queue setup =========
enum class SchedulingMode { ROUND_ROBIN, SHORTEST_FIRST, SHORTEST_REMAINING };

struct LevelConfiguration {
    SchedulingMode strategy = SchedulingMode::ROUND_ROBIN;
    int timeSlice = 1;
};

// Abstract base for scheduling strategies
class SchedulingStrategy {
protected:
    std::vector<Task>& taskList; // shared reference to all tasks

public:
    explicit SchedulingStrategy(std::vector<Task>& t) : taskList(t) {}
    virtual ~SchedulingStrategy() = default;

    // Core scheduling lifecycle events
    virtual void addToQueue(int taskId) = 0;                 // task arrives at this level
    virtual bool hasWaitingTasks() const = 0;                // any ready tasks at this level?
    virtual int  selectNextTask(int currentTaskId) = 0;      // choose next task ID (or keep current)
    virtual void processTimeUnit(int taskId) = 0;            // execute one time unit for task
    virtual void handleTaskExit(int taskId) = 0;             // task finished or preempted
    virtual void purgeTask(int taskId) = 0;                  // remove from internal structures
    virtual void updateWaitingTimes(int runningTaskId) = 0;  // increment wait time for others
};

// Factory functions for scheduling strategies (maintain public signatures)
std::unique_ptr<SchedulingStrategy> createRoundRobinStrategy(std::vector<Task>& tasks, int timeQuantum);
std::unique_ptr<SchedulingStrategy> createShortestJobStrategy(std::vector<Task>& tasks);
std::unique_ptr<SchedulingStrategy> createPreemptiveShortestStrategy(std::vector<Task>& tasks);

// Input/Output and reporting utilities
std::vector<Task> parseInputFile(const std::string& filepath);
std::vector<Task> parseInputStream(std::istream& input);
void generateReport(std::ostream& stream, const std::vector<Task>& items);
std::vector<Task> executeMLFQ(const std::vector<Task>& input, char algorithm);

// Performance metrics
struct PerformanceMetrics { double WT = 0.0, CT = 0.0, RT = 0.0, TAT = 0.0; };
PerformanceMetrics calculateMetrics(const std::vector<Task>& items);
std::string algorithmDescription(char algorithm);
void writeConsolidatedReport(std::ostream& os, const std::vector<std::pair<char, std::vector<Task>>>& results);
