#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <vector>

#define MAXN 30000
#define FILE_PATH "../ulfs/mount_point/largefile.txt"

int main() {
    const char *filename = FILE_PATH;

    // Start measuring time
    auto start = std::chrono::steady_clock::now();

    // Open the file
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        std::cerr << "Error opening the file: " << strerror(errno) << std::endl;
        return 1; // Return with error status
    }

    // Determine the file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        std::cerr << "Error determining file size: " << strerror(errno) << std::endl;
        close(fd);
        return 1; // Return with error status
    }

    auto secondstart = std::chrono::steady_clock::now();

    // Allocate buffer for reading
    std::vector<char> buffer(file_size);

    for(int i = 0; i < MAXN; i++) {
        std::cout << "Reading file " << i << std::endl;
        // Reset file pointer to the beginning
        if (lseek(fd, 0, SEEK_SET) == -1) {
            std::cerr << "Error resetting file pointer: " << strerror(errno) << std::endl;
            close(fd);
            return 1; // Return with error status
        }

        // Read the entire file
        ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
        if (bytes_read == -1) {
            std::cerr << "Error reading file: " << strerror(errno) << std::endl;
            close(fd);
            return 1; // Return with error status
        }
    }

    // Display the contents
    std::cout << "File contents:" << std::endl;
    // std::cout.write(buffer.data(), bytes_read);

    // // Check if there was an error during the read
    // if (bytes_read == -1) {
    //     std::cerr << "Error reading file: " << strerror(errno) << std::endl;
    //     close(fd);
    //     return 1; // Return with error status
    // }

    // Stop measuring time
    auto end = std::chrono::steady_clock::now();

    // Calculate elapsed time
    

    auto secondduration = std::chrono::duration_cast<std::chrono::milliseconds>(end - secondstart);
    std::cout << "\nElapsed time: " << secondduration.count() << " milliseconds." << std::endl;

    // Close the file descriptor
    close(fd);

    end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nElapsed time: " << duration.count() << " milliseconds." << std::endl;

    return 0; // Return success
}
