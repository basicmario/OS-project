#include <iostream>
#include <filesystem>
#include <queue>  
#include <deque>          // std::deque
#include <list>
#include <vector>


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
    int id;
    std::string name; 
    std::string type; 

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




    }

};  




class FCFS : public System {


    public: 
    int num;
    std::queue<int> jobs; 

    FCFS(){};
    FCFS(int value){
        num = value;
    };
};



