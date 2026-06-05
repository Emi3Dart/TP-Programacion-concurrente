#ifndef JOB_H
#define JOB_H

#include <chrono>


struct Job {
    int id;
    int prioridad;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
};

#endif
