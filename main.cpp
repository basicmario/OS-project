
#include <iostream>
#include <filesystem>
#include "include/resource.hpp"

namespace fs = std::filesystem;


int main(int argc, char** argv){
    std::string path = "data";
    int quantum = 4;
    bool verbose = false;
    int positional = 0;
    for (int i = 1; i < argc; i++){
        std::string a = argv[i];
        if (a == "-v") verbose = true;
        else if (positional == 0) { path = a; positional++; }
        else { quantum = std::stoi(a); positional++; }
    }

    // a single file, or every .txt in a folder
    std::vector<fs::path> files;
    if (fs::is_directory(path)){
        for (auto& e : fs::directory_iterator(path))
            if (e.path().extension() == ".txt") files.push_back(e.path());
        std::sort(files.begin(), files.end());
    } else if (fs::exists(path)){
        files.push_back(path);
    }
    if (files.empty()){ std::cout << "No workload files found at: " << path << "\n"; return 1; }

    std::ofstream csv("results.csv");
    csv << "workload,algorithm,avg_wait,avg_turnaround,avg_response,throughput,"
           "cpu_util,gpu_util,io_util,mem_util,total_time\n";
    csv << std::fixed << std::setprecision(2);

    for (const fs::path& f : files){
        System sys;
        sys.initiatesystem(f.string());
        if (sys.getJobs().empty()){ std::cout << "No jobs in " << f.string() << "\n"; continue; }

        FCFSScheduler fcfs;
        RoundRobinScheduler rr(quantum);
        PriorityScheduler prio;
        Resources res;   // the fake computer; each run gets its own copy

        std::vector<Result> results = {
            simulate(sys.getJobs(), fcfs, res),
            simulate(sys.getJobs(), rr,   res),
            simulate(sys.getJobs(), prio, res),
        };

        std::string title = f.filename().string();
        printComparison(title, results);
        if (verbose) for (const Result& r : results) printJobs(r);
        writeCsv(csv, title, results);
    }
    std::cout << "\nResults also saved to results.csv\n";
    return 0;
}