#include <iostream>
#include <fstream>
#include <chrono>
#include <string>

using namespace std;

#define MAXN 3000
#define FILE_PATH "../../lab2/t"

int main() {
    // Initialize the sum
    int total_sum = 0;
    auto start = std::chrono::steady_clock::now();
    // Iterate over files t1.txt to t100.txt
    for(int j = 0; j < MAXN; j++) {
        for (int i = 1; i <= 100; ++i) {
            string filename = FILE_PATH + to_string(i) + ".txt";

            ifstream file(filename);
            if (file.is_open()) {
                // Read the first number from the file
                int first_number;
                file >> first_number;
                // Add the number to the sum
                total_sum += first_number;
                file.close();
            } else {
                // Handle the case where the file is not found
                cout << "File " << filename << " not found" << endl;
            }
        }
    }

    // Print the total sum
    cout << "Total sum: " << total_sum << endl;

    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nElapsed time: " << duration.count() << " milliseconds." << std::endl;

    return 0;
}
