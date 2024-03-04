#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <thread>

using namespace std;

#define MAXN 2
#define FILE_PATH "../ulfs/mount_point/t"

int main() {
    // Initialize the sum
    int total_sum = 0;
    auto start = std::chrono::steady_clock::now();
    // Iterate over files t1.txt to t100.txt
    auto n1 = std::chrono::steady_clock::now();
    std::chrono::milliseconds skip_duration = std::chrono::duration_cast<std::chrono::milliseconds>(n1 - start);
    for(int j = 0; j < MAXN; j++) {
        for (int i = 1; i <= 100; ++i) {
            cout << "Reading file " << i << endl;
            string filename = FILE_PATH + to_string(i) + ".txt";

            int fd = open(filename.c_str(), O_RDWR);
            cout << "File descriptor: " << fd << endl;
            if (fd != -1) {
                // Read the first number from the file
                int first_number;
                if (read(fd, &first_number, sizeof(first_number)) > 0) {
                    // Add the number to the sum
                    total_sum += first_number;
                } else {
                    cout << "Error reading file " << filename << endl;
                }

                //write a new random number to the file
                int new_number = rand() % 100;
                lseek(fd, 0, SEEK_SET);
                if(write(fd, &new_number, sizeof(new_number)) < 0) {
                    cout << "Error writing to file " << filename << endl;
                }

                close(fd);
            } else {
                // Handle the case where the file is not found
                cout << "File " << filename << " not found" << endl;
            }

            auto s1 = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            auto s2 = std::chrono::steady_clock::now();
            auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(s2 - s1);
            skip_duration += duration1;
        }
    }

    // Print the total sum
    cout << "Total sum: " << total_sum << endl;

    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nElapsed time: " << duration.count() << " milliseconds." << std::endl;
    std::cout << "Skip time: " << skip_duration.count() << " milliseconds." << std::endl;
    std::cout << "difference time: " << duration.count() - skip_duration.count() << " milliseconds. " << std::endl;

    return 0;
}
