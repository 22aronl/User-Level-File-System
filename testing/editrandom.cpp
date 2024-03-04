#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <chrono>

using namespace std;

const int MAX_FILES = 3; // Maximum number of files

#define FILE_PATH "../ulfs/mountpoint/"
#define MAXN 50

int main() {
    // Seed the random number generator
    srand(time(0));

    // List of files
    string files[MAX_FILES] = {"file1.txt", "file2.txt", "file3.txt"};

    // Preallocate file descriptors for each file
    int fileDescriptors[MAX_FILES];

    auto start = std::chrono::steady_clock::now();

    // Open all files and get file descriptors
    for (int i = 0; i < MAX_FILES; ++i) {
        fileDescriptors[i] = open((FILE_PATH + files[i]).c_str(), O_RDWR);
        if (fileDescriptors[i] == -1) {
            cerr << "Error: Unable to open file " << files[i] << endl;
            return 1;
        }
    }

    // Number of times to perform random access and modification
    // int numIterations = 5; // You can change this value

    for (int i = 0; i < MAXN; ++i) {
        // Start time for the operation
        // clock_t startTime = clock();

        // Choose a random file from the list
        int randomIndex = rand() % MAX_FILES;

        // Choose a random position in the file
        off_t fileSize = lseek(fileDescriptors[randomIndex], 0, SEEK_END);
        off_t randomPos = rand() % fileSize;

        // Read a byte from the random position
        lseek(fileDescriptors[randomIndex], randomPos, SEEK_SET);
        char byte;
        read(fileDescriptors[randomIndex], &byte, 1);

        // Replace the byte with another random byte
        char newByte = rand() % 256;

        // Write the modified byte to the file
        lseek(fileDescriptors[randomIndex], randomPos, SEEK_SET);
        write(fileDescriptors[randomIndex], &newByte, 1);

        // End time for the operatio
    }

    // Stop measuring time
    auto end = std::chrono::steady_clock::now();

    // Calculate elapsed time
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nElapsed time: " << duration.count() << " milliseconds." << std::endl;

    // Close all files
    for (int i = 0; i < MAX_FILES; ++i) {
        close(fileDescriptors[i]);
    }

    return 0;
}
