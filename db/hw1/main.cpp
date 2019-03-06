#include <chrono>
#include <iostream>
#include <unistd.h>
class Timer
{
    using clock_t = std::chrono::high_resolution_clock;
    using microseconds = std::chrono::microseconds;
public:
    Timer()
        : start_(clock_t::now())
    {
    }

    ~Timer()
    {
        const auto finish = clock_t::now();
        const auto us =
            std::chrono::duration_cast<microseconds>
                (finish - start_).count();
        std::cout << us << " us" << std::endl;
    }

private:
    const clock_t::time_point start_;
};

constexpr int N = 1 << 14;

void dumb_transpose(int *matrix1, int *matrix2) {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            matrix2[i * N + j] = matrix1[j*N + i];
        }
}

void smart_transpose(int *matrix1, int *matrix2, int n) {
    if (n == 1) {
        *matrix2 = *matrix1;
        return;
    }
    int t = n >> 1;
    smart_transpose(matrix1, matrix2, t);
    smart_transpose(matrix1 + 2 * t * t, matrix2 + t * t, t);
    smart_transpose(matrix1 + t * t, matrix2 + 2 * t * t, t);
    smart_transpose(matrix1 + 3 * t * t, matrix2 + 3 * t * t, t);
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    int *matrix1 = (int*) malloc(sizeof(int) * N * N);
    int *matrix2 = (int*) malloc(sizeof(int) * N * N);
    for (int i = 0; i < N * N; ++i) {
        matrix1[i] = rand();
    }
    if (argc > 1) {
        Timer t;
        smart_transpose(matrix1, matrix2, N);
        return 0;
    }
    {
        Timer t;
        dumb_transpose(matrix1, matrix2);
    }
    return 0;
}
