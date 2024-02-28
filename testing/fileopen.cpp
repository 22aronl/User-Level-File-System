#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#define MAXN 100000
#define FILE_PATH "../nfs/example.txt"

int main() {
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < MAXN; i++) {
        // Open the file
        std::ofstream file(FILE_PATH);

        // Check if the file is opened successfully
        if (!file.is_open()) {
            std::cerr << "Error opening the file!" << std::endl;
            return 1; // Return with error status
        }

        // Touch the file by simply writing to it
        file << "Touching the file.";

        // Close the file
        file.close();

    }

    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Elapsed time: " << duration.count() << " milliseconds." << std::endl;


    return 0;
}
