#include "mlfq.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

using namespace std;

// Utility functions
static inline string trimWhitespace(const string& text){
    size_t start = text.find_first_not_of(" \t\r\n");
    if(start == string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

static inline vector<string> splitString(const string& text, char delimiter){
    vector<string> segments; string fragment; stringstream stream(text);
    while(getline(stream, fragment, delimiter)) segments.push_back(trimWhitespace(fragment));
    return segments;
}

// Internal: parse input stream into task list
static vector<Task> extractTasks(istream& source){
    vector<Task> items; string line;
    while(getline(source, line)){
        line = trimWhitespace(line);
        if(line.empty() || line[0] == '#') continue; // 🚫 Skip comments
        auto components = splitString(line, ';');
        if(components.size() < 5) continue;
        Task job;
        job.identifier     = components[0];
        job.serviceDuration = stoi(components[1]);
        job.arrivalMoment  = stoi(components[2]);
        job.tier         = stoi(components[3]);
        job.priority        = stoi(components[4]);
        job.timeLeft = job.serviceDuration;
        items.push_back(job);
    }
    return items;
}

// Public API functions
vector<Task> parseInputFile(const string& filepath){
    ifstream inputFile(filepath);
    if(!inputFile) throw runtime_error("❌ Unable to open file: " + filepath);
    return extractTasks(inputFile);
}

vector<Task> parseInputStream(istream& inputStream){
    return extractTasks(inputStream);
}

void generateReport(ostream& output, const vector<Task>& taskList){
    long long totalWT = 0, totalCT = 0, totalRT = 0, totalTAT = 0;
    for (const auto& task : taskList){
        int responseTime  = (task.startMoment < 0 ? 0 : (task.startMoment));
        int turnaroundTime = (task.finishMoment < 0 ? 0 : (task.finishMoment - task.arrivalMoment));
        totalWT += task.delayAccumulated; totalCT += task.finishMoment; totalRT += responseTime; totalTAT += turnaroundTime;
        output << task.identifier << ';' << task.serviceDuration << ';' << task.arrivalMoment << ';' << task.tier << ';' << task.priority << ';'
            << ' ' << task.delayAccumulated << ';' << ' ' << task.finishMoment << ';' << ' ' << responseTime << ';' << ' ' << turnaroundTime << '\n';
    }
    double taskCount = static_cast<double>(taskList.size());
    output << fixed << setprecision(1);
    output << "WT=" << (totalWT / taskCount) << "; CT=" << (totalCT / taskCount) << "; RT=" << (totalRT / taskCount) << "; TAT=" << (totalTAT / taskCount) << ";\n";
}

PerformanceMetrics calculateMetrics(const vector<Task>& taskList){
    long long totalWT = 0, totalCT = 0, totalRT = 0, totalTAT = 0;
    for (const auto& task : taskList){
        int responseTime  = (task.startMoment < 0 ? 0 : (task.startMoment - task.arrivalMoment));
        int turnaroundTime = (task.finishMoment < 0 ? 0 : (task.finishMoment - task.arrivalMoment));
        totalWT += task.delayAccumulated; totalCT += task.finishMoment; totalRT += responseTime; totalTAT += turnaroundTime;
    }
    double taskCount = static_cast<double>(taskList.size());
    PerformanceMetrics metrics{};
    if(taskCount > 0){
        metrics.WT  = totalWT / taskCount;
        metrics.CT  = totalCT / taskCount;
        metrics.RT  = totalRT / taskCount;
        metrics.TAT = totalTAT / taskCount;
    }
    return metrics;
}

std::string algorithmDescription(char algorithmType){
    algorithmType = toupper(algorithmType);
    if (algorithmType == 'A') return "A → RR(1), RR(3), RR(4), SJF";
    if (algorithmType == 'B') return "B → RR(2), RR(3), RR(4), STCF";
    if (algorithmType == 'C') return "C → RR(3), RR(5), RR(6), RR(20)";
    return " ";
}

void writeConsolidatedReport(ostream& output, const vector<pair<char, vector<Task>>>& results) {
    for (const auto& result : results){
        output << "📈 Algorithm " << algorithmDescription(result.first) << endl;
        output << "identifier; BT; AT; Q; Pr; WT; CT; RT; TAT\n";
        generateReport(output, result.second);
        output << "\n";
    }

    output << "🏆 Algorithm Comparison\n";
    output << "algorithm; WT; CT; RT; TAT\n";
    for (const auto& result : results) {
        auto metrics = calculateMetrics(result.second);
        output << result.first << "; " << setprecision(1) << metrics.WT << "; " << metrics.CT << "; " << metrics.RT << "; " << metrics.TAT << "\n";
    }
}
