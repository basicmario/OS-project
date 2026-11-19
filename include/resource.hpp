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
    int arrival, io, cpu, gpu, memory, priority;   // lower priority number = more important

    // changes during the simulation
    int remIo = 0, remCpu = 0, remGpu = 0;
    int startTime = -1, finishTime = -1;
    int sliceUsed = 0;    // ticks used in the current Round Robin slice
    long seq = 0;         // order in which the job entered its current waiting line
};



struct Stats{
    int totalTime = 0;
    long cpuBusy = 0, gpuBusy = 0, ioBusy = 0, memTicks = 0;
};


struct Result{
    std::string algo;
    double avgWait = 0, avgTurn = 0, avgResp = 0, throughput = 0;
    double cpuUtil = 0, gpuUtil = 0, ioUtil = 0, memUtil = 0;
    int totalTime = 0;
    std::vector<Job> jobs;
};


void writeCsv(std::ofstream& csv, const std::string& title, const std::vector<Result>& results){
    for (const Result& r : results)
        csv << title << ',' << r.algo << ',' << r.avgWait << ',' << r.avgTurn << ',' << r.avgResp
            << ',' << r.throughput << ',' << r.cpuUtil << ',' << r.gpuUtil << ',' << r.ioUtil
            << ',' << r.memUtil << ',' << r.totalTime << '\n';
}


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

    const std::vector<Job>& getJobs() const { return jobs; }

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






class Scheduler{
public:
    virtual std::string name() const = 0;
    virtual bool before(const Job& a, const Job& b) const = 0;
    virtual int quantum() const { return 0; }
    virtual ~Scheduler() = default;
};

class FCFSScheduler : public Scheduler{
public:
    std::string name() const override { return "FCFS"; }
    bool before(const Job& a, const Job& b) const override { return a.seq < b.seq; }
};

class RoundRobinScheduler : public Scheduler{
    int q;
public:
    explicit RoundRobinScheduler(int quantum) : q(quantum) {}
    std::string name() const override { return "RR(q=" + std::to_string(q) + ")"; }
    // same FIFO order as FCFS; the difference is that CPU jobs are kicked off after q ticks
    bool before(const Job& a, const Job& b) const override { return a.seq < b.seq; }
    int quantum() const override { return q; }
};

class PriorityScheduler : public Scheduler{      // non-preemptive
public:
    std::string name() const override { return "Priority"; }
    bool before(const Job& a, const Job& b) const override {
        if (a.priority != b.priority) return a.priority < b.priority;
        return a.seq < b.seq;                    // tie: first come, first served
    }
};
















Result simulate(std::vector<Job> jobs, const Scheduler& sched, Resources res){

    std::stable_sort(jobs.begin(), jobs.end(),
        [](const Job& a, const Job& b){ return a.arrival < b.arrival; });

    // One waiting line per resource, and the jobs currently being served
    std::vector<Job> admitQ, ioQ, cpuQ, gpuQ;
    std::vector<Job> ioRun, cpuRun, gpuRun;
    std::vector<Job> finished;

    size_t nextArrival = 0;
    int total = jobs.size();
    long seqCounter = 0;
    long cpuBusy = 0, gpuBusy = 0, ioBusy = 0, memTicks = 0;
    int clock = 0;

    // Order a waiting line using the scheduler's rule
    auto sortQ = [&](std::vector<Job>& q){
        std::stable_sort(q.begin(), q.end(),
            [&](const Job& a, const Job& b){ return sched.before(a, b); });
    };

    // Send a job to the line for its next stage (I/O -> CPU -> GPU), or finish it
    auto route = [&](Job j, int now){
        j.seq = seqCounter++;
        if      (j.remIo  > 0) ioQ.push_back(j);
        else if (j.remCpu > 0) cpuQ.push_back(j);
        else if (j.remGpu > 0) gpuQ.push_back(j);
        else {
            j.finishTime = now;
            res.releaseMemory(j.memory);
            finished.push_back(j);
        }
    };

    // Give free servers (channel / cores / GPU) to the best waiting jobs
    auto startJobs = [&](std::vector<Job>& q, std::vector<Job>& run, std::function<bool()> take){
        sortQ(q);
        while (!q.empty() && take()){
            Job j = q.front();
            q.erase(q.begin());
            j.sliceUsed = 0;
            if (j.startTime < 0) j.startTime = clock;    // first time anything served it
            run.push_back(j);
        }
    };

    // One tick of work on a resource. A stage that finishes moves on; a job that
    // used its whole quantum (CPU only) goes to the back of the line.
    auto doWork = [&](std::vector<Job>& run, std::vector<Job>& q, int Job::*rem,
                      long& busy, std::function<void()> release, int quantum){
        for (auto it = run.begin(); it != run.end(); ){
            Job& j = *it;
            j.*rem -= 1;
            j.sliceUsed++;
            busy++;
            if (j.*rem <= 0){
                Job done = j;
                it = run.erase(it);
                release();
                route(done, clock + 1);
            } else if (quantum > 0 && j.sliceUsed >= quantum){
                Job preempted = j;
                it = run.erase(it);
                release();
                preempted.seq = seqCounter++;
                q.push_back(preempted);
            } else {
                ++it;
            }
        }
    };

    while ((int)finished.size() < total && clock < 100000){

        // 1. Arrivals
        while (nextArrival < jobs.size() && jobs[nextArrival].arrival <= clock){
            Job j = jobs[nextArrival++];
            j.seq = seqCounter++;
            admitQ.push_back(j);
        }

        // 2. RAM admission, in the scheduler's order. If the first job does not fit, the rest wait.
        sortQ(admitQ);
        while (!admitQ.empty()){
            Job& next = admitQ.front();
            if (next.memory > res.memoryTotal()){
                std::cout << "Skipping " << next.name << ": needs more RAM than the machine has\n";
                admitQ.erase(admitQ.begin());
                total--;
                continue;
            }
            if (!res.allocateMemory(next.memory)) break;
            Job j = next;
            admitQ.erase(admitQ.begin());
            route(j, clock);
        }

        // 3. Hand out the I/O channel, cores and GPU
        startJobs(ioQ,  ioRun,  [&]{ return res.takeIo(); });
        startJobs(cpuQ, cpuRun, [&]{ return res.takeCore(); });
        startJobs(gpuQ, gpuRun, [&]{ return res.takeGpu(); });

        // 4. One tick of work
        memTicks += res.memoryUsed();
        doWork(ioRun,  ioQ,  &Job::remIo,  ioBusy,  [&]{ res.releaseIo(); },   0);
        doWork(cpuRun, cpuQ, &Job::remCpu, cpuBusy, [&]{ res.releaseCore(); }, sched.quantum());
        doWork(gpuRun, gpuQ, &Job::remGpu, gpuBusy, [&]{ res.releaseGpu(); },  0);

        clock++;
    }

    // ---- metrics
    Result r;
    r.algo = sched.name();
    r.totalTime = clock;
    r.jobs = finished;
    std::sort(r.jobs.begin(), r.jobs.end(),
        [](const Job& a, const Job& b){ return a.arrival < b.arrival; });

    int n = finished.size();
    if (n == 0 || clock == 0) return r;
    double w = 0, t = 0, rs = 0;
    for (const Job& j : finished){
        int turnaround = j.finishTime - j.arrival;
        w  += turnaround - (j.io + j.cpu + j.gpu);   // time spent NOT being served
        t  += turnaround;
        rs += j.startTime - j.arrival;               // time until first service
    }
    r.avgWait = w / n;  r.avgTurn = t / n;  r.avgResp = rs / n;
    r.throughput = (double)n / clock;
    r.cpuUtil = 100.0 * cpuBusy / ((double)res.cores() * clock);
    r.gpuUtil = 100.0 * gpuBusy / ((double)res.gpus() * clock);
    r.ioUtil  = 100.0 * ioBusy  / ((double)res.ioChannels() * clock);
    r.memUtil = 100.0 * memTicks / ((double)res.memoryTotal() * clock);
    return r;
}



void printComparison(const std::string& title, const std::vector<Result>& results){
    std::cout << "\n=== Workload: " << title << " ===\n";
    std::cout << std::left << std::setw(11) << "Algorithm" << std::right
              << std::setw(9) << "AvgWait" << std::setw(9) << "AvgTurn" << std::setw(9) << "AvgResp"
              << std::setw(12) << "Throughput" << std::setw(7) << "CPU%" << std::setw(7) << "GPU%"
              << std::setw(7) << "IO%" << std::setw(7) << "Mem%" << std::setw(7) << "Total" << "\n";
    for (const Result& r : results){
        std::cout << std::fixed << std::left << std::setw(11) << r.algo << std::right
                  << std::setprecision(2)
                  << std::setw(9) << r.avgWait << std::setw(9) << r.avgTurn << std::setw(9) << r.avgResp
                  << std::setw(12) << r.throughput << std::setprecision(1)
                  << std::setw(7) << r.cpuUtil << std::setw(7) << r.gpuUtil
                  << std::setw(7) << r.ioUtil << std::setw(7) << r.memUtil
                  << std::setw(7) << r.totalTime << "\n";
    }
}

void printJobs(const Result& r){
    std::cout << "\n  -- " << r.algo << " per-job --\n";
    std::cout << "  " << std::left << std::setw(14) << "Job" << std::right
              << std::setw(8) << "Arrive" << std::setw(8) << "Start" << std::setw(8) << "Finish"
              << std::setw(8) << "Wait" << std::setw(8) << "Turn" << std::setw(8) << "Resp" << "\n";
    for (const Job& j : r.jobs){
        int turn = j.finishTime - j.arrival;
        std::cout << "  " << std::left << std::setw(14) << j.name << std::right
                  << std::setw(8) << j.arrival << std::setw(8) << j.startTime
                  << std::setw(8) << j.finishTime << std::setw(8) << turn - (j.io + j.cpu + j.gpu)
                  << std::setw(8) << turn << std::setw(8) << j.startTime - j.arrival << "\n";
    }
}