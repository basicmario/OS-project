#include <iostream>
#include <filesystem>


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




class System{

    private:
    std::string filepath;

    public:
    System(){};
    System(std::string filePath){
        filepath = filePath;
    };

    ~System(){};

    void initiatesystem(System* s1){

        if(s1 == nullptr){
            std::cout << "need a system to initiate" << std::endl;
            return;
        }

        if (!std::filesystem::exists(s1->filepath) || !std::filesystem::is_directory(s1->filepath)) {
            std::cout << "Invalid directory no file path.\n";
            return;
        }else{
            std::cout << "System initiated successfully" << std::endl;
        }
    }

};  