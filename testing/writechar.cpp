#include <iostream>
#include <fstream>
#include <random>

#define FILE_PATH "../ulfs/mount_point/largefile.txt"

int main() {
    const char *filename = FILE_PATH;
    const int megabyte = 10 * 1024 * 1024; // 1 megabyte
    const int buffer_size = 1024; // Buffer size for each write operation

    // Open the file for writing
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening the file for writing!" << std::endl;
        return 1; // Return with error status
    }

    // Set up a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(32, 126); // ASCII range for printable characters

    // Write random characters to the file
    for (int i = 0; i < megabyte; i += buffer_size) {
        int remaining_bytes = megabyte - i;
        int write_size = std::min(buffer_size, remaining_bytes);
        char buffer[buffer_size];
        
        // Fill the buffer with random characters
        for (int j = 0; j < write_size; ++j) {
            buffer[j] = static_cast<char>(dis(gen));
        }

        // Write the buffer to the file
        file.write(buffer, write_size);
    }

    // Close the file
    file.close();

    std::cout << "Successfully wrote 1 megabyte of random characters to " << filename << std::endl;

    return 0; // Return success
}
