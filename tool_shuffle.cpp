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

#define MAX_LINES 80000000

class FastCorpusShuffler {
private:
    std::vector<std::string> corpus1_;
    std::vector<std::string> corpus2_;
    std::vector<int32_t> indices;
    std::mt19937_64 rng_;
    
public:
    FastCorpusShuffler() : rng_(std::random_device{}()) {
        // Pre-allocate vectors to avoid reallocations
        corpus1_.reserve(MAX_LINES);  // ~80M lines
        corpus2_.reserve(MAX_LINES);
        indices.reserve(MAX_LINES);

        std::string shuffle_file_path_ = ".\\shuffle_indices.bin";
        std::ifstream shuffle_file(shuffle_file_path_, std::ios::binary);
        
        if (shuffle_file.is_open()) {
            std::cout << "Found existing shuffle indices file. Loading..." << std::endl;
            // std::vector<int32_t> indices(MAX_LINES);
            shuffle_file.read(reinterpret_cast<char*>(indices.data()), MAX_LINES * sizeof(int32_t));
            shuffle_file.close();
        } else {
            std::iota(0, MAX_LINES, 0);
            std::shuffle(0, MAX_LINES, rng_);
            std::ofstream out_file(shuffle_file_path_, std::ios::binary);
            if (!out_file.is_open()) {
                std::cerr << "Error: Cannot create shuffle indices file" << std::endl;
            } else {
                out_file.write(reinterpret_cast<char*>(indices.data()), MAX_LINES * sizeof(int32_t));
                out_file.close();
            }
        }
    }

    bool LoadFiles(const char* file1, const char* file2, size_t max_lines = 0) {
        auto start = std::chrono::high_resolution_clock::now();
        std::cout << "Loading files..." << std::endl;

        if (!LoadSingleFile(file1, corpus1_, max_lines) || !LoadSingleFile(file2, corpus2_, max_lines)) {
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

    bool SaveFiles(const char* out1, const char* out2, const char* out1_sample, const char* out2_sample, size_t sample_lines = 0) {
        auto start = std::chrono::high_resolution_clock::now();
        
        size_t lines_to_write = corpus1_.size() - sample_lines;

        if (sample_lines!=0 &&(!SaveSingleFile(out1_sample, corpus1_, 0, sample_lines) || !SaveSingleFile(out2_sample, corpus2_, 0, sample_lines))) {
            return false;
        }
        if (!SaveSingleFile(out1, corpus1_, sample_lines, lines_to_write) || !SaveSingleFile(out2, corpus2_, sample_lines, lines_to_write)) {
            return false;
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        std::cout << "Saving completed in " << duration.count() << " seconds\n";
        return true;
    }

private:
    bool LoadSingleFile(const char* filename, std::vector<std::string>& lines, size_t max_lines) {
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
            if (max_lines==count) {
                break;
            }
            if (count % 1000000 == 0) {
                std::cout << "Loaded " << count / 1000000 << "M lines from " << filename << std::endl;
            }
        }
        return true;
    }

    bool SaveSingleFile(const char* filename, const std::vector<std::string>& lines, int offset, size_t num_lines) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open output file " << filename << std::endl;
            return false;
        }

        // Use large buffer for writing
        const size_t BUFFER_SIZE = 1 << 20;  // 1MB buffer
        std::unique_ptr<char[]> buffer(new char[BUFFER_SIZE]);
        file.rdbuf()->pubsetbuf(buffer.get(), BUFFER_SIZE);

        for (size_t i = offset; i < num_lines; ++i) {
            file << lines[i] << '\n';
            if ((i + 1) % 1000000 == 0) {
                std::cout << "Wrote " << (i + 1) / 1000000 << "M lines to " << filename << std::endl;
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
    if (argc < 3 || argc > 5) {
        std::cerr << "Usage: " << argv[0] << std::endl
                  << "shuffle <from_code> <to_code> [max_lines] [sample_lines]" << std::endl
                  << "from_code: language code, ex: en, de, it, zh, ja ..." << std::endl
                  << "to_code: language code, ex: en, de, it, zh, ja ..." << std::endl
                  << "max_lines: max line count to be processed in corpus. if zero or undefined, will be process all." << std::endl
                  << "sample_lines: sample line count to be used for training validation." << std::endl
                  << std::endl;
        return 1;
    }

    FastCorpusShuffler shuffler;
    char* from_code = argv[1];
    char* to_code = argv[2];
    
    size_t max_lines = 0;
    size_t sample_lines = 0;
    
    if (argc > 3) {
        max_lines = std::stoull(argv[3]);
        if (argc > 4) sample_lines = std::stoull(argv[4]);
    }
    std::string corpora_dir = std::string("..\\corpora\\");
    std::string source = corpora_dir + from_code + ".txt";
    std::string target = corpora_dir + to_code + ".txt";
    std::string source_shuffled = corpora_dir + from_code + ".shuffled.txt";
    std::string target_shuffled = corpora_dir + to_code + ".shuffled.txt";
    std::string source_sample = corpora_dir + from_code + ".sample.txt";
    std::string target_sample = corpora_dir + to_code + ".sample.txt";
    if (!shuffler.LoadFiles(source.c_str(), target.c_str(), max_lines)) {
        return 1;
    }

    shuffler.Shuffle();

    if (!shuffler.SaveFiles(source_shuffled.c_str(), target_shuffled.c_str(), source_sample.c_str(), target_sample.c_str(), sample_lines)) {
        return 1;
    }

    return 0;
}