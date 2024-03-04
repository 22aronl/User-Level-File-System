#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <vector>
#include <random>

#define MAXN 100
#define FILE_PATH "../ulfs/mount_point/largefile.txt"

int main() {
    const char *filename = FILE_PATH;

    // Start measuring time
    auto start = std::chrono::steady_clock::now();

    // Open the file
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        std::cerr << "Error opening the file1: " << strerror(errno) << std::endl;
        return 1; // Return with error status
    }

    std::cerr << "FILE OPENED" << std::endl;

    // Determine the file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        std::cerr << "Error determining file size: " << strerror(errno) << std::endl;
        close(fd);
        return 1; // Return with error status
    }

    // Allocate buffer for reading
    std::vector<char> buffer(file_size);

    // Start random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255); // Range for random bytes

    for(int i = 0; i < MAXN; i++) {
        std::cout << "Reading file " << i << std::endl;
        // Reset file pointer to the beginning
        if (lseek(fd, 0, SEEK_SET) == -1) {
            std::cerr << "Error resetting file pointer: " << strerror(errno) << std::endl;
            close(fd);
            return 1; // Return with error status
        }

        // Read the entire file into buffer
        ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
        if (bytes_read == -1) {
            std::cerr << "Error reading file1: " << strerror(errno) << std::endl;
            close(fd);
            return 1; // Return with error status
        }

        // Replace some bytes with random bytes
        for (int j = 0; j < bytes_read; j++) {
            // Replace with random byte with a certain probability (e.g., 10%)
            if (dis(gen) < 25) {
                buffer[j] = static_cast<char>(dis(gen)); // Replace with random byte
            }
        }

        // Write the modified buffer back to the file
        if (lseek(fd, 0, SEEK_SET) == -1) {
            std::cerr << "Error resetting file pointer: " << strerror(errno) << std::endl;
            close(fd);
            return 1; // Return with error status
        }

        if (write(fd, buffer.data(), bytes_read) == -1) {
            std::cerr << "Error writing back to file: " << strerror(errno) << std::endl;
            close(fd);
            return 1; // Return with error status
        }
    }

    // Stop measuring time
    auto end = std::chrono::steady_clock::now();

    // Calculate elapsed time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nElapsed time: " << duration.count() << " milliseconds." << std::endl;

    // Close the file descriptor
    if(close(fd) == -1) {
        std::cerr << "Error closing the file1: " << strerror(errno) << std::endl;
        return 1; // Return with error status
    }

    end = std::chrono::steady_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nElapsed time: " << duration2.count() << " milliseconds." << std::endl;

    return 0; // Return success
}