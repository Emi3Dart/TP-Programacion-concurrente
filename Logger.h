#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <mutex>
#include <fstream>

class Logger {
private:
    std::mutex logMutex;
    std::ofstream file;

public:
    Logger(const std::string& filename);
    ~Logger();

    void log(int jobId, int prioridad, const std::string& evento);
};
extern Logger globalLogger;

#endif
