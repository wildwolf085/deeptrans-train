#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <memory>
#include <thread>
#include <algorithm>
#include <numeric>  // Add this include for std::iota

class FastCorpusShuffler {
private:
    std::vector<std::string> corpus1_;
    std::vector<std::string> corpus2_;
    std::mt19937_64 rng_;
    
public:
    FastCorpusShuffler() : rng_(std::random_device{}()) {
        // Pre-allocate vectors to avoid reallocations
        corpus1_.reserve(80000000);  // ~80M lines
        corpus2_.reserve(80000000);
    }

    bool LoadFiles(const char* file1, const char* file2) {
        auto start = std::chrono::high_resolution_clock::now();
        std::cout << "Loading files..." << std::endl;

        if (!LoadSingleFile(file1, corpus1_) || !LoadSingleFile(file2, corpus2_)) {
            return false;
        }

        if (corpus1_.size() != corpus2_.size()) {
            std::cerr << "Error: Files have different number of lines" << std::endl;
            return false;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        std::cout << "Loading completed in " << duration.count() << " seconds\n";
        std::cout << "Total lines: " << corpus1_.size() << std::endl;
        return true;
    }

    void Shuffle() {
        auto start = std::chrono::high_resolution_clock::now();
        std::cout << "Starting shuffle..." << std::endl;

        // Create index vector for shuffling
        std::vector<size_t> indices(corpus1_.size());
        std::iota(indices.begin(), indices.end(), 0);
        
        // Shuffle indices
        std::shuffle(indices.begin(), indices.end(), rng_);

        // Apply permutation to both vectors
        ApplyPermutation(indices);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        std::cout << "Shuffle completed in " << duration.count() << " seconds\n";
    }

    bool SaveFiles(const char* out1, const char* out2, size_t num_lines = 0) {
        auto start = std::chrono::high_resolution_clock::now();
        
        size_t lines_to_write = (num_lines > 0 && num_lines < corpus1_.size()) 
            ? num_lines : corpus1_.size();

        if (!SaveSingleFile(out1, corpus1_, lines_to_write) || 
            !SaveSingleFile(out2, corpus2_, lines_to_write)) {
            return false;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        std::cout << "Saving completed in " << duration.count() << " seconds\n";
        return true;
    }

private:
    bool LoadSingleFile(const char* filename, std::vector<std::string>& lines) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << filename << std::endl;
            return false;
        }

        // Use large buffer for reading
        const size_t BUFFER_SIZE = 1 << 20;  // 1MB buffer
        std::unique_ptr<char[]> buffer(new char[BUFFER_SIZE]);
        file.rdbuf()->pubsetbuf(buffer.get(), BUFFER_SIZE);
        
        std::string line;
        line.reserve(1024);  // Reserve typical line length
        
        size_t count = 0;
        while (std::getline(file, line)) {
            lines.push_back(std::move(line));
            count++;
            if (count % 1000000 == 0) {
                std::cout << "Loaded " << count / 1000000 << "M lines from " 
                         << filename << std::endl;
            }
        }
        return true;
    }

    bool SaveSingleFile(const char* filename, const std::vector<std::string>& lines, 
                       size_t num_lines) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open output file " << filename << std::endl;
            return false;
        }

        // Use large buffer for writing
        const size_t BUFFER_SIZE = 1 << 20;  // 1MB buffer
        std::unique_ptr<char[]> buffer(new char[BUFFER_SIZE]);
        file.rdbuf()->pubsetbuf(buffer.get(), BUFFER_SIZE);

        for (size_t i = 0; i < num_lines; ++i) {
            file << lines[i] << '\n';
            if ((i + 1) % 1000000 == 0) {
                std::cout << "Wrote " << (i + 1) / 1000000 << "M lines to " 
                         << filename << std::endl;
            }
        }
        return true;
    }

    void ApplyPermutation(const std::vector<size_t>& indices) {
        std::vector<std::string> temp1(corpus1_.size());
        std::vector<std::string> temp2(corpus2_.size());
        
        #pragma omp parallel for
        for (size_t i = 0; i < indices.size(); ++i) {
            temp1[i] = std::move(corpus1_[indices[i]]);
            temp2[i] = std::move(corpus2_[indices[i]]);
        }
        
        corpus1_ = std::move(temp1);
        corpus2_ = std::move(temp2);
    }
};

int main(int argc, char* argv[]) {
    if (argc < 5 || argc > 6) {
        std::cerr << "Usage: " << argv[0] 
                  << " <input_file1> <input_file2> <output_file1> <output_file2> [num_lines]" 
                  << std::endl;
        return 1;
    }

    FastCorpusShuffler shuffler;
    
    if (!shuffler.LoadFiles(argv[1], argv[2])) {
        return 1;
    }

    shuffler.Shuffle();

    size_t num_lines = 0;
    if (argc == 6) {
        num_lines = std::stoull(argv[5]);
    }

    if (!shuffler.SaveFiles(argv[3], argv[4], num_lines)) {
        return 1;
    }

    return 0;
}