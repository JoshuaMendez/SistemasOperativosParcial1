#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include "mlfq.h"

// Main entry point: handles argument parsing, input loading,
// algorithm execution, and consolidated report generation.

using namespace std;

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);

    string inputFilePath, outputFilePath;

    // Helper function to detect prefixes like --in= and --out=
    auto hasPrefix = [](const string& text, const char* prefix){
        size_t prefixLength = strlen(prefix);
        return text.size() >= prefixLength && text.compare(0, prefixLength, prefix) == 0;
    };

    // Basic argument parsing
    for (int argIndex = 1; argIndex < argc; ++argIndex) {
        string argument = argv[argIndex];
        if (hasPrefix(argument, "--in=")) {
            inputFilePath = argument.substr(5);
        } else if (hasPrefix(argument, "--out=")) {
            outputFilePath = argument.substr(6);
        }
    }

    // Exit with error code if required paths are missing
    if (inputFilePath.empty() || outputFilePath.empty()) return 1;

    try {
        // Load task list from input file
        auto taskCollection = parseInputFile(inputFilePath);

        // Execute different algorithm schemes and collect results
        vector<pair<char, vector<Task>>> algorithmResults;
        algorithmResults.emplace_back('A', executeMLFQ(taskCollection, 'A'));
        algorithmResults.emplace_back('B', executeMLFQ(taskCollection, 'B'));
        algorithmResults.emplace_back('C', executeMLFQ(taskCollection, 'C'));

        // Write consolidated report
        ofstream outputStream(outputFilePath, ios::binary);
        if (!outputStream) return 2;
        writeConsolidatedReport(outputStream, algorithmResults);
    } catch (const exception&) {
        return 3;
    } catch (...) {
        return 3;
    }

    return 0;
}

