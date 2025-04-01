#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define CHUNK_SIZE (1024 * 1024 * 4)  // 4MB buffer for random data

struct Shuffler {
    int urandom_fd;
    unsigned char* rand_buffer;
    size_t rand_pos;
    size_t rand_max;
};

void init_shuffler(struct Shuffler* s) {
    s->urandom_fd = open("/dev/urandom", O_RDONLY);
    s->rand_buffer = malloc(CHUNK_SIZE);
    s->rand_pos = CHUNK_SIZE;  // Force initial refill
}

size_t get_random(struct Shuffler* s, size_t max) {
    size_t value;
    const size_t byte_needed = sizeof(size_t);
    size_t mask = -1;

    if (max < (1ULL << 32)) {
        mask = (1ULL << 32) - 1;
    } else {
        mask = -1;
    }

    do {
        if (s->rand_pos + byte_needed > CHUNK_SIZE) {
            read(s->urandom_fd, s->rand_buffer, CHUNK_SIZE);
            s->rand_pos = 0;
        }
        
        memcpy(&value, s->rand_buffer + s->rand_pos, byte_needed);
        s->rand_pos += byte_needed;
        value &= mask;
    } while (value > max);

    return value;
}

void shuffle_lines(char** lines, size_t n, struct Shuffler* s) {
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = get_random(s, i);
        char* temp = lines[i];
        lines[i] = lines[j];
        lines[j] = temp;
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input output\n", argv[0]);
        return 1;
    }

    // Memory map input file
    int fd = open(argv[1], O_RDONLY);
    struct stat st;
    fstat(fd, &st);
    size_t fsize = st.st_size;
    
    char* data = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    // Build line index
    char** lines = malloc(71368384 * sizeof(char*));
    size_t line_count = 0;
    char* current = data;

    for (size_t i = 0; i < fsize; i++) {
        if (data[i] == '\n') {
            data[i] = '\0';
            lines[line_count++] = current;
            current = data + i + 1;
        }
    }
    if (current < data + fsize) lines[line_count++] = current;

    // Shuffle using high-performance random buffer
    struct Shuffler shuffler;
    init_shuffler(&shuffler);
    shuffle_lines(lines, line_count, &shuffler);

    // Write output
    FILE* out = fopen(argv[2], "w");
    for (size_t i = 0; i < line_count; i++) {
        fputs(lines[i], out);
        fputc('\n', out);
    }
    fclose(out);

    // Cleanup
    munmap(data, fsize);
    free(lines);
    close(shuffler.urandom_fd);
    free(shuffler.rand_buffer);

    return 0;
}