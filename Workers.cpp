#include "Workers.h"
#include "Job.h"
#include "MessageQueue.h"
#include "VramPool.h"
#include "Logger.h"
#include <random>
#include <thread>
#include <chrono>

// Al igual que en la carpeta del profe (pcv4), usamos "extern" 
// para usar las variables que viven globalmente dentro de main.cpp
extern MessageQueue msg_queue;
extern VramPool vram_pool;
extern std::mutex mtx_counter;
extern int total_jobs_finalizados;

void productor_api(int id_productor, int n_peticiones) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> d_prioridad(0, 100);

    for (int i = 0; i < n_peticiones; ++i) {
        Job j;
        j.id = (id_productor * 10000) + i; 
        j.timestamp = std::chrono::steady_clock::now();
        j.prioridad = (d_prioridad(gen) < 30) ? 1 : 0; 
        
        globalLogger.log(j.id, j.prioridad, "CREADO"); 
        msg_queue.push(j); 

        // Requisito del PDF: "retardo de 100ms por tarea".
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void consumidor_worker(int id_consumidor) {
    while (true) {
        Job job_a_procesar = msg_queue.pop();
        
        // Píldora envenenada: Salimos del loop infinito.
        if (job_a_procesar.id == -1) {
            break; 
        }

        vram_pool.procesarJob(job_a_procesar, total_jobs_finalizados, mtx_counter);
    }
}
