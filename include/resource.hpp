#include <iostream>
#include <filesystem>
#include <sstream>
#include <queue>  
#include <deque>          // std::deque
#include <list>
#include <vector>
#include <fstream>      // std::ifstream


class Resources{

    private:

    int freeMemoryMB = 8192;
    int totalMemoryMB = 8192;   // total RAM
    int gpuFree = 1;           // one GPU
    bool ioBusy = false;       // one data-loading channel
    int cpuCoresFree = 2;

    public:

    Resources(){};
    ~Resources(){};

    void checkresources(){
        std::cout << "Memory: " << freeMemoryMB << std::endl;
    }

};




struct Job{
    std::string id;
    std::string name; 
    std::string type; 
    std::string arrival;
    std::string io; 
    std::string cpu; 
    std::string gpu;
    std::string memory; 
    std::string priority; 

};



class System{

    private:
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
        }else{
            std::cout << "System initiated successfully" << std::endl;
        }

        std::ifstream ifs;
        std::vector<std::string> holder;


        
        ifs.open(filepath, std::ifstream::in);
        if (!ifs.is_open()) {
            std::cout << "error opening file" << std::endl;
            return; 
        }

        std::string buffer;
        while (std::getline(ifs, buffer)) {
            
            if (buffer.empty() || buffer[0] == '#') {
                continue;
            }

            
            std::istringstream iss(buffer);
            std::vector<std::string> holder; 
            std::string holder1;

            while (std::getline(iss, holder1, ' ')) {
                
                if (!holder1.empty()) {
                    holder.push_back(holder1);
                }
            }

            // Ensure the line had at least 3 fields before accessing them
            if (holder.size() >= 9) {
                Job j1;
                j1.id = holder[0];
                j1.name = holder[1];
                j1.type = holder[2];
                j1.arrival = holder[3];
                j1.io = holder[4];
                j1.cpu = holder[5];
                j1.gpu = holder[6];
                j1.memory = holder[7];
                j1.priority = holder[8];

                jobs.push_back(j1);
                std::cout << "size: " << jobs.size() << std::endl;
            }
        }
        

        

    }

};  




class FCFS : public System {


    public: 
    int num;
    std::queue<Job> jobsholder; 

    FCFS(){};
    FCFS(int value){
        num = value;
    };

    void initiateFCFS(){

        for(int x = 0; x < jobs.size(); x++){
            jobsholder.push(jobs[x]);
        }


    }
};



