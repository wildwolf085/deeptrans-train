#include <iostream>
#include <fstream>
#include <string>

std::ifstream is_;

bool ReadLine(std::string *line) {
    return static_cast<bool>(std::getline(is_, *line));
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    char* filename = argv[1];
    is_.open(filename);
    if (!is_.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return 1;
    }

    int count = 0;
    std::string line;
    while (ReadLine(&line)) {
        count++;
        if (count % 1000000 == 0) {
            std::cout << "Found " << count << " lines" << std::endl;
            // break;
        }
    }
    // pe  it 
    std::cout << "Found " << count << " lines" << std::endl;
    std::cout << line << std::endl;
    return 0;
}