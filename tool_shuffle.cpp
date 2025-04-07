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
#include <filesystem>

#define MAX_LINES 80000000

class FastCorpusShuffler {
private:
    std::vector<std::string> corpus1_;
    std::vector<std::string> corpus2_;
    std::vector<int32_t> indices_;  // Added underscore for consistency and made it a member
    
public:
    FastCorpusShuffler() {
        corpus1_.reserve(MAX_LINES);
        corpus2_.reserve(MAX_LINES);
        indices_.reserve(MAX_LINES);  // Reserve space for indices
    }

    bool InitializeShuffleIndices(size_t corpus_size) {
        // std::cout << "corpus_size: " << corpus_size << std::endl;
        // format text
        std::string shuffle_file_path = ".\\shuffle-" + std::to_string(corpus_size) + ".bin";  // Changed path to match original
        std::ifstream shuffle_file(shuffle_file_path, std::ios::binary);
        indices_.resize(corpus_size);  // Resize instead of creating new vector
        
        if (shuffle_file.is_open()) {
            std::cout << "Found existing shuffle indices file. Loading..." << std::endl;
            shuffle_file.read(reinterpret_cast<char*>(indices_.data()), corpus_size * sizeof(int32_t));
            shuffle_file.close();
            
            if (shuffle_file.gcount() == corpus_size * sizeof(int32_t)) {
                return true;  // Don't apply permutation here - we use indices_ directly
            }
        }
        // Always seed properly
        std::random_device rd;
        std::mt19937 rng(rd());
        // Use std::shuffle directly
        // std::shuffle(data.begin(), data.end(), rng);
        std::cout << "Creating new shuffle indices file..." << std::endl;
        std::iota(indices_.begin(), indices_.end(), 0);
        std::shuffle(indices_.begin(), indices_.end(), rng);

        std::ofstream out_file(shuffle_file_path, std::ios::binary);
        if (!out_file.is_open()) {
            std::cerr << "Error: Cannot create shuffle indices file" << std::endl;
            return false;
        }

        out_file.write(reinterpret_cast<char*>(indices_.data()), corpus_size * sizeof(int32_t));
        out_file.close();

        return true;  // Don't apply permutation here
    }

    bool LoadFiles(const char* dir, const char* from_code, const char* to_code, size_t max_lines = 0) {
        auto start = std::chrono::high_resolution_clock::now();
        std::cout << "Loading files..." << std::endl;

        std::string from_ext = std::string(".") + from_code;
        std::string to_ext = std::string(".") + to_code;
        
        // List all files in directory
        std::string dir_path = dir;
        for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
            std::string filename = entry.path().filename().string();
            
            // Check if file ends with from_code extension
            if (filename.length() >= from_ext.length() && 
                filename.compare(filename.length() - from_ext.length(), from_ext.length(), from_ext) == 0) {
                
                // Find corresponding to_code file by replacing extension
                std::string base_name = filename.substr(0, filename.length() - from_ext.length());
                std::string target_file = base_name + to_ext;
                
                if (std::filesystem::exists(dir_path + "/" + target_file)) {
                    std::cout << "Processing pair: " << filename << " and " << target_file << std::endl;
                    
                    if (!LoadSingleFile((dir_path + "/" + filename).c_str(), corpus1_, max_lines) || 
                        !LoadSingleFile((dir_path + "/" + target_file).c_str(), corpus2_, max_lines)) {
                        return false;
                    }
                }
            }
        }

        if (corpus1_.empty() || corpus1_.size() != corpus2_.size()) {
            std::cerr << "Error: No valid file pairs found or files have different number of lines" << std::endl;
            return false;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
        std::cout << "Loading completed in " << duration.count() << " seconds\n";
        std::cout << "Total lines: " << corpus1_.size() << std::endl;
        InitializeShuffleIndices(max_lines==0 ? corpus1_.size() : max_lines);
        return true;
    }

    // void Shuffle() {
    //     auto start = std::chrono::high_resolution_clock::now();
    //     std::cout << "Starting shuffle..." << std::endl;

    //     // Create index vector for shuffling
    //     std::vector<size_t> indices(corpus1_.size());
    //     std::iota(indices.begin(), indices.end(), 0);
        
    //     // Shuffle indices
    //     std::shuffle(indices.begin(), indices.end(), rng_);

    //     // Apply permutation to both vectors
    //     ApplyPermutation(indices);

    //     auto end = std::chrono::high_resolution_clock::now();
    //     auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    //     std::cout << "Shuffle completed in " << duration.count() << " seconds\n";
    // }

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
        
        std::string line;
        line.reserve(8192);  // Reserve typical line length
        
        size_t count = 0;
        while (std::getline(file, line)) {
            // Skip empty lines or lines with only whitespace
            if (line.empty() || std::all_of(line.begin(), line.end(), ::isspace)) {
                continue;
            }
            
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
        // const size_t BUFFER_SIZE = 1 << 20;  // 1MB buffer
        // std::unique_ptr<char[]> buffer(new char[BUFFER_SIZE]);
        // file.rdbuf()->pubsetbuf(buffer.get(), BUFFER_SIZE);
        
        size_t count = lines.size();
        
        // std::cout << "Offset " << offset + num_lines << " Count " << count << " lines" << std::endl;

        for (size_t i = offset; i < offset + num_lines && i < count; i++) {  // Fixed bounds checking
            file << lines[indices_[i]] << '\n';  // Use indices_ member variable
            // if ((i + 1) % 10 == 0) {
            //     std::cout << indices_[i] << std::endl;
            // }
            if ((i + 1) % 1000000 == 0) {
                std::cout << "Wrote " << (i + 1) / 1000000 << "M lines to " << filename << std::endl;
            }
        }
        return true;
    }

    // void ApplyPermutation(const std::vector<size_t>& indices) {
    //     std::vector<std::string> temp1(corpus1_.size());
    //     std::vector<std::string> temp2(corpus2_.size());
        
    //     #pragma omp parallel for
    //     for (size_t i = 0; i < indices.size(); ++i) {
    //         temp1[i] = std::move(corpus1_[indices[i]]);
    //         temp2[i] = std::move(corpus2_[indices[i]]);
    //     }
        
    //     corpus1_ = std::move(temp1);
    //     corpus2_ = std::move(temp2);
    // }
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
    std::string corpora_dir = std::string(".\\corpora\\");
    // std::string source = corpora_dir + from_code + ".txt";
    // std::string target = corpora_dir + to_code + ".txt";
    std::string source_train = corpora_dir + from_code + ".train.corpus";
    std::string target_train = corpora_dir + to_code + ".train.corpus";
    std::string source_valid = corpora_dir + from_code + ".valid.corpus";
    std::string target_valid = corpora_dir + to_code + ".valid.corpus";

    if (!shuffler.LoadFiles(corpora_dir.c_str(), from_code, to_code, max_lines)) {
        return 1;
    }
    
    // shuffler.Shuffle();
    if (!shuffler.SaveFiles(source_train.c_str(), target_train.c_str(), source_valid.c_str(), target_valid.c_str(), sample_lines)) {
        return 1;
    }
    return 0;
}