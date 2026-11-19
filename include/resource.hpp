#include <iostream>
#include <filesystem>
#include <sstream>
#include <queue>  
#include <deque>          // std::deque
#include <list>
#include <vector>
#include <fstream>      // std::ifstream
#include <algorithm>
#include <functional>

class Resources{
private:
    int totalMemoryMB = 8192, freeMemoryMB = 8192;
    int totalCores = 2,       coresFree    = 2;
    int totalGpus  = 1,       gpusFree     = 1;
    int totalIo    = 1,       ioFree       = 1;

public:
    bool allocateMemory(int mb){
        if (mb > freeMemoryMB) return false;      // not enough RAM, job must wait
        freeMemoryMB -= mb;
        return true;
    }
    void releaseMemory(int mb){ freeMemoryMB += mb; }

    bool takeCore(){ if (coresFree == 0) return false; coresFree--; return true; }
    void releaseCore(){ coresFree++; }

    bool takeGpu(){ if (gpusFree == 0) return false; gpusFree--; return true; }
    void releaseGpu(){ gpusFree++; }

    bool takeIo(){ if (ioFree == 0) return false; ioFree--; return true; }
    void releaseIo(){ ioFree++; }

    int cores() const { return totalCores; }
    int gpus() const { return totalGpus; }
    int ioChannels() const { return totalIo; }
    int memoryTotal() const { return totalMemoryMB; }
    int memoryUsed() const { return totalMemoryMB - freeMemoryMB; }
};




struct Job{
    int id;
    std::string name;
    std::string type;
    int arrival, io, cpu, gpu, memory, priority;

    // changes during the simulation
    int remIo = 0, remCpu = 0, remGpu = 0;
    int startTime = -1, finishTime = -1;
};



struct Stats{
    int totalTime = 0;
    long cpuBusy = 0, gpuBusy = 0, ioBusy = 0, memTicks = 0;
};



class System{

    protected:
    std::string file_path;
    std::vector<Job> jobs;

    public:
    System(){};
    System(std::string filePath){
        file_path = filePath;
    };

    ~System(){};

    void initiatesystem(std::string filepath){

        if(filepath.empty()){
            std::cout << "system needs a file to start" << std::endl;
            return;
        }

        std::filesystem::path p1 = filepath; 

        if (!std::filesystem::exists(p1) ) {
            std::cout << "Invalid directory no file path.\n";
            return;
        }

        std::ifstream ifs;
        std::vector<std::string> holder;


        
        ifs.open(filepath, std::ifstream::in);
        if (!ifs.is_open()) {
            std::cout << "error opening file" << std::endl;
            return; 
        }

        std::string buffer;
        int lineNo = 0;
        while (std::getline(ifs, buffer)) {
            lineNo++;

            if (buffer.empty() || buffer[0] == '#') {
                continue;
            }

            std::istringstream iss(buffer);
            Job j;
            if (iss >> j.id >> j.name >> j.type >> j.arrival >> j.io
                    >> j.cpu >> j.gpu >> j.memory >> j.priority) {
                j.remIo  = j.io;
                j.remCpu = j.cpu;
                j.remGpu = j.gpu;
                jobs.push_back(j);
            } else {
                std::cout << "Line " << lineNo << " rejected: [" << buffer << "]\n";
            }
        }

        std::cout << "Loaded " << jobs.size() << " jobs\n";
        
        std::cout << "System initiated successfully" << std::endl;
        

    }

};  



class FCFS : public System {
public:
    std::queue<Job> jobsholder;   // jobs that have not arrived yet, in arrival order

    void initiateFCFS(){
        std::vector<Job> sorted = jobs;
        std::stable_sort(sorted.begin(), sorted.end(),
            [](const Job& a, const Job& b){ return a.arrival < b.arrival; });

        jobsholder = std::queue<Job>();
        for (const Job& j : sorted) jobsholder.push(j);
    }

    void printResults(std::vector<Job> done, const Stats& s, const Resources& res){
        std::sort(done.begin(), done.end(),
            [](const Job& a, const Job& b){ return a.arrival < b.arrival; });

        double wait = 0, turn = 0, resp = 0;
        std::cout << "\nJob\t\tArrive\tStart\tFinish\tWait\tTurn\tResp\n";
        for (const Job& j : done){
            int t = j.finishTime - j.arrival;                 // turnaround
            int w = t - (j.io + j.cpu + j.gpu);               // time NOT being served
            int r = j.startTime - j.arrival;                  // time until first service
            wait += w; turn += t; resp += r;
            std::cout << j.name << "\t\t" << j.arrival << "\t" << j.startTime << "\t"
                      << j.finishTime << "\t" << w << "\t" << t << "\t" << r << "\n";
        }
        int n = done.size();
        if (n == 0 || s.totalTime == 0) { std::cout << "No jobs finished.\n"; return; }

        std::cout << "\nAvg wait: " << wait / n << "  Avg turnaround: " << turn / n
                  << "  Avg response: " << resp / n << "\n";
        std::cout << "Throughput: " << (double)n / s.totalTime << " jobs/tick\n";
        std::cout << "Total time: " << s.totalTime << " ticks\n";
        std::cout << "CPU utilization: " << 100.0 * s.cpuBusy / ((double)res.cores() * s.totalTime) << "%\n";
        std::cout << "GPU utilization: " << 100.0 * s.gpuBusy / ((double)res.gpus() * s.totalTime) << "%\n";
        std::cout << "I/O utilization: " << 100.0 * s.ioBusy / ((double)res.ioChannels() * s.totalTime) << "%\n";
        std::cout << "Memory utilization: " << 100.0 * s.memTicks / ((double)res.memoryTotal() * s.totalTime) << "%\n";
    }

    void runFCFS(Resources& res){
        if (jobsholder.empty()){
            std::cout << "No jobs loaded. Check the file path and that initiatesystem() ran first.\n";
            return;
        }

        // One FIFO waiting line per resource
        std::queue<Job> admitQ;    // arrived, waiting for RAM
        std::queue<Job> ioQ, cpuQ, gpuQ;
        // Jobs currently being served
        std::vector<Job> ioRun, cpuRun, gpuRun;
        std::vector<Job> finished;

        int total = jobsholder.size();
        Stats stats;
        int clock = 0;

        // Send a job to the line for its next stage (I/O -> CPU -> GPU), or finish it
        auto route = [&](Job j, int now){
            if      (j.remIo  > 0) ioQ.push(j);
            else if (j.remCpu > 0) cpuQ.push(j);
            else if (j.remGpu > 0) gpuQ.push(j);
            else {
                j.finishTime = now;
                res.releaseMemory(j.memory);
                finished.push_back(j);
            }
        };

        // Give free servers to jobs at the front of a line (first come, first served)
        auto startJobs = [&](std::queue<Job>& q, std::vector<Job>& run, std::function<bool()> take){
            while (!q.empty() && take()){
                Job j = q.front();
                q.pop();
                if (j.startTime < 0) j.startTime = clock;     // first time anything served it
                run.push_back(j);
            }
        };

        // Do one tick of work for everything running on a resource
        auto doWork = [&](std::vector<Job>& run, int Job::*rem, long& busy, std::function<void()> release){
            for (auto it = run.begin(); it != run.end(); ){
                Job& j = *it;
                j.*rem -= 1;
                busy++;
                if (j.*rem <= 0){
                    Job done = j;
                    it = run.erase(it);
                    release();
                    route(done, clock + 1);     // goes on to its next stage
                } else {
                    ++it;
                }
            }
        };

        while ((int)finished.size() < total && clock < 100000){

            // 1. Arrivals join the RAM-admission line
            while (!jobsholder.empty() && jobsholder.front().arrival <= clock){
                admitQ.push(jobsholder.front());
                jobsholder.pop();
            }

            // 2. RAM admission, strictly in order: if the front job does not fit, everyone waits
            while (!admitQ.empty()){
                Job& next = admitQ.front();
                if (next.memory > res.memoryTotal()){
                    std::cout << "Skipping " << next.name << ": needs more RAM than the machine has\n";
                    admitQ.pop();
                    total--;
                    continue;
                }
                if (!res.allocateMemory(next.memory)) break;
                Job j = next;
                admitQ.pop();
                route(j, clock);
            }

            // 3. Hand out I/O channel, cores and GPU
            startJobs(ioQ,  ioRun,  [&]{ return res.takeIo(); });
            startJobs(cpuQ, cpuRun, [&]{ return res.takeCore(); });
            startJobs(gpuQ, gpuRun, [&]{ return res.takeGpu(); });

            // 4. One tick of work
            stats.memTicks += res.memoryUsed();
            doWork(ioRun,  &Job::remIo,  stats.ioBusy,  [&]{ res.releaseIo(); });
            doWork(cpuRun, &Job::remCpu, stats.cpuBusy, [&]{ res.releaseCore(); });
            doWork(gpuRun, &Job::remGpu, stats.gpuBusy, [&]{ res.releaseGpu(); });

            clock++;
        }

        stats.totalTime = clock;
        printResults(finished, stats, res);
    }
};


