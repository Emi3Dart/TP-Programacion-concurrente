#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>


Logger globalLogger("sistema.log");

Logger::Logger(const std::string& filename) {
    file.open(filename, std::ios::app);
    if(!file.is_open()) {
        std::cerr << "Error al abrir el archivo de log: " << filename << std::endl;
    }
}

Logger::~Logger() {
    if(file.is_open()) {
        file.close();
    }
}

void Logger::log(int jobId, int prioridad, const std::string& evento) {
    std::lock_guard<std::mutex> lock(logMutex);

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    if(file.is_open()) {
        // Usa put_time para formatear la fecha como pide "Timestamp".
        file << "[" << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << "] - "
             << "Job ID: " << jobId << " - "
             << "Prioridad: " << (prioridad == 1 ? "Premium" : "Free") << " - "
             << "Evento: " << evento << "\n";

        std::flush(file); // Forzar escritura real a disco inmediata (buena práctica concurrente, al costo de I/O de sistema).
    }
}
