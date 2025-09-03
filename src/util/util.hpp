#include <iostream>
#include <chrono>

using namespace std;


// Timer Part
auto timerStart()
{
    return chrono::high_resolution_clock::now();
}

template<typename TimePoint>
double timerCheck(TimePoint& timer)
{
    auto stop = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(stop - timer);
    return duration.count() / 1000000.0;
}

// Calculation Part
double averageNonZero(const double arr[], int size) {
    double sum = 0.0;
    int count = 0;

    for (int i = 0; i < size; ++i) {
        if (arr[i] != 0) {
            sum += arr[i];
            ++count;
        }
    }

    if (count == 0) {
        return 0; // Avoid division by zero if there are no non-zero elements
    }

    return sum / count;
}