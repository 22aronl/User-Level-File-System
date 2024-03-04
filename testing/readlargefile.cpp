#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <vector>

#define MAXN 5000
#define FILE_PATH "../ulfs/mount_point/largefile.txt"
#define File_PATH2 "../ulfs/mount_point/largefile2.txt"

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

    // int write_fd = open(File_PATH2, O_RDWR, S_IRUSR | S_IWUSR);
    // if (write_fd == -1) {
    //     std::cerr << "Error opening the file2: " << strerror(errno) << std::endl;
    //     return 1; // Return with error status
    // }

    std::cerr << "FILES OPENED" << std::endl;

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
        // std::cout << "Reading file " << i << std::endl;
        // Reset file pointer to the beginning
        if (lseek(fd, 0, SEEK_SET) == -1) {
            std::cerr << "Error resetting file pointer: " << strerror(errno) << std::endl;
            close(fd);
            return 1; // Return with error status
        }

        // if(lseek(write_fd, 0, SEEK_SET) == -1) {
        //     std::cerr << "Error resetting file pointer: " << strerror(errno) << std::endl;
        //     close(write_fd);
        //     return 1; // Return with error status
        // }

        // Read the entire file
        ssize_t bytes_read = read(fd, buffer.data(), buffer.size());
        if (bytes_read == -1) {
            std::cerr << "Error reading file1: " << strerror(errno) << std::endl;
            close(fd);
            return 1; // Return with error status
        }

        // if(write(write_fd, buffer.data(), bytes_read) == -1) {
        //     std::cerr << "Error writing file2: " << strerror(errno) << std::endl;
        //     close(write_fd);
        //     return 1; // Return with error status
        // }
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

    // std::cout << "closing file 2" << std::endl;
    // if(close(write_fd) == -1) {
    //     std::cerr << "Error closing the file2: " << strerror(errno) << std::endl;
    //     return 1; // Return with error status
    // }

    std::cout << "closing file 1" << std::endl;
    // Close the file descriptor
    if(close(fd) == -1) {
        std::cerr << "Error closing the file1: " << strerror(errno) << std::endl;
        return 1; // Return with error status
    }
    end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nElapsed time: " << duration.count() << " milliseconds." << std::endl;

    return 0; // Return success
}
