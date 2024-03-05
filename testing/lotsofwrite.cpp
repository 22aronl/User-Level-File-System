#include <chrono>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

using namespace std;

#define FILE_PATH "../ulfs/mount_point/emptyfile.txt"
#define MAXN 1

int main() {
    const int GB = 1;             // Size of data to write in GB
    const int CHUNK_SIZE_MB = 10; // Chunk size in MB
    const long long int BYTES_PER_MB = 1024 * 1024;
    const long long int CHUNK_SIZE_BYTES = CHUNK_SIZE_MB * BYTES_PER_MB;
    const long long int BYTES_PER_GB = 1024 * 1024 * 1024;

    // Generate data to write (random bytes)
    char *data = new char[CHUNK_SIZE_BYTES];
    for (long long int i = 0; i < CHUNK_SIZE_BYTES; ++i) {
        data[i] = rand() % 256;
    }

    // Start time
    auto start = std::chrono::steady_clock::now();

    // Open file for writing
    int fd = open((FILE_PATH), O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd == -1) {
        cerr << "Error: Unable to open file for writing." << endl;
        return 1;
    }

    // Write data to file
    for (int i = 0; i < MAXN; i++) {
        cout << "Writing file " << i << endl;
        // Reset file pointer to the beginning
        if (lseek(fd, 0, SEEK_SET) == -1) {
            cerr << "Error: Unable to reset file pointer." << endl;
            close(fd);
            return 1;
        }
        // Write data in chunks
        long long int bytesWrittenTotal = 0;
        while (bytesWrittenTotal < BYTES_PER_GB) {
            // Write data in chunks
            ssize_t bytesWritten = write(fd, data, CHUNK_SIZE_BYTES);
            if (bytesWritten == -1) {
                cerr << "Error: Failed to write data to file." << endl;
                close(fd);
                return 1;
            }

            bytesWrittenTotal += bytesWritten;
        }
    }

    auto end2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start);
    std::cout << "Elapsed time1: " << duration.count() << " milliseconds." << std::endl;

    // Delete the written data
    if (ftruncate(fd, 0) == -1) {
        cerr << "Error: Failed to delete written data." << endl;
        close(fd);
        return 1;
    }

    auto end = std::chrono::steady_clock::now();

    // Calculate elapsed time

    auto secondduration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nElapsed time2: " << secondduration.count() << " milliseconds." << std::endl;

    // Cleanup
    // delete data;

    close(fd);
    delete[] data;

    return 0;
}
