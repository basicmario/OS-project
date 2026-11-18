
#include <iostream>
#include <filesystem>
#include "include/resource.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {

    Resources r1;
    //r1.checkresources();

    System s1(R"(C:\Users\HP\Documents\Coding Files\OSproject\data\test.tx)");
    s1.initiatesystem("C:\\Users\\HP\\OneDrive\\Documents\\Coding Files\\OSproject\\data\\test.txt");

    //FCFS f1;
    //f1.initiatesystem(R"(C:\Users\HP\Documents\Coding Files\OSproject\data\test.txt)")


    /*
    
    fs::path folder = argv[1];

    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        std::cout << "Invalid directory.\n";
        return 1;
    }

    for (const auto& entry : fs::directory_iterator(folder)) {

        if (entry.is_regular_file()) {
            std::cout << entry.path().filename().string() << '\n';
        }

    }

    */
    

    return 0;
}
