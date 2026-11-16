
#include <iostream>
#include <filesystem>
#include "include/resource.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {

    Resources r1;
    r1.checkresources();

    System s1("C:/Users/HP/OneDrive/Documents/Profesional Developmement");
    s1.initiatesystem(&s1);

    if (argc < 2) {
        std::cout << "Usage: program.exe <folder_path>\n";
        return 1;
    }


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
