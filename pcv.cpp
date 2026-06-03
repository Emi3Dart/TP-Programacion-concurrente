#include "pcv.h"
#include "aging.h"
#include "semaforo.h"
#include <iostream>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <ctime>
#include <fstream>

// Recursos compartidos
extern std::queue<Job> premiumQueue; // Cola para premium.
extern std::queue<Job> freeQueue; // Cola para free.
extern Semaforo hay_espacio;
extern Semaforo hay_datos;
extern Semaforo vram;
std::mutex mtx_buffer;
std::mutex mtx_log;
std::mutex mtx_asignacionVRAM;
std::mutex mtx_finalizadas;
std::mutex mtx_jobId;
std::ofstream archivoLog("sistema.log");

int tareasFinalizadas = 0;
int jobId = 0;

//generador atomico de IDs (para garantizar IDs unicos secuenciales sin Race Conditions)
int generarSiguienteId() {
    std::unique_lock<std::mutex> lock(mtx_jobId);
    return jobId++;
}

// Log
void logEvento(const Job& tarea, const std::string& evento){
    std::unique_lock<std::mutex> lock(mtx_log);

    std::time_t ahora = std::time(nullptr);
    std::tm* tiempo = std::localtime(&ahora);

    char hora[20];
    std::strftime(hora, sizeof(hora), "%H:%M:%S", tiempo);

    std::cout << "[" << hora << "] - " << "Job " << tarea.id << " - Prioridad " << tarea.prioridad << " - " << evento << std::endl;
    archivoLog << "[" << hora << "] - " << "Job " << tarea.id << " - Prioridad " << tarea.prioridad << " - " << evento << std::endl;
    if (archivoLog.is_open()) {
        archivoLog << "[" << hora << "] - " << "Job " << tarea.id << " - Prioridad " << tarea.prioridad << " - " << evento << "\n";
        archivoLog.flush(); // Asegura el volcado inmediato al disco
    }
}


void apiGateway() {
    for (int i = 0; i < tareasGateway; i++) {
        wait(hay_espacio); // Espera a que haya un hueco libre
        mtx_buffer.lock(); // Mutex de la cola

        // Creacion estructura job
        Job tarea;

        // Encapsulamiento del Mutex que protege acceso a los jobId evitando race condition.
        tarea.id = generarSiguienteId();

        tarea.prioridad = rand() %2; // Generacion de prioridad
        // (PRUEBA SATURACION DE RECURSOS) tarea.prioridad = 1 (Todos premium)
        tarea.llegada = std::chrono::steady_clock::now();
        logEvento(tarea, "CREADO");

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Retardo antes de encolar.

        // If de prioridad que decide donde encolar dependiendo prioridad.
       if (tarea.prioridad== 1) {
            premiumQueue.push(tarea);
       } else {
            freeQueue.push(tarea);
       }

        logEvento(tarea, "EN_COLA");
        mtx_buffer.unlock();

        signal(hay_datos); // 3. Avisa que hay un nuevo dato disponible
    }
}

void workerNode() {
    int totalTareas = NUM_GATEWAYS * tareasGateway; // Cantidad total de tareas generadas por todos los Gateways.
    int tareasPorWorker = totalTareas / NUM_WORKERS; // Cantidad de tareas que debe procesar cada Worker.

    for (int i = 0; i < tareasPorWorker; i++) {
        wait(hay_datos); // Espera a que haya al menos un dato
        mtx_buffer.lock(); // Mutex de la cola

        Job tarea; // Variable auxiliar a usar en el worker

        // Encolacion segun prioridad
        if(!freeQueue.empty() && agingActivado(freeQueue.front())) {
            // Si la cola de FREE no está vacía
            // Y además el primer job de esa cola ya superó el tiempo de espera (aging >= 5000ms)
            logEvento(freeQueue.front(), "AGING ACTIVADO");

            tarea = freeQueue.front(); // Se copia ese job a la variable local "tarea"
            freeQueue.pop(); // Se elimina de la cola porque ya va a ser procesado

        } else if (!premiumQueue.empty()) {
            // Si no se cumplió el caso de aging en FREE
            // pero hay jobs en la cola PREMIUM
            tarea = premiumQueue.front(); // Se toma el primer job premium (alta prioridad)
            premiumQueue.pop();

        } else {
            // Caso final: si no hay premium (o ya fueron consumidos).
            tarea = freeQueue.front(); // Se toma un job de la cola FREE sin importar aging
            freeQueue.pop();

        }

        mtx_buffer.unlock();
        signal(hay_espacio); // Avisa que libera un espacio

        // El semáforo "vram" gestiona internamente la concurrencia.
        // Esto permite que hasta 5 workers asignen memoria SIMULTÁNEAMENTE sin pisarse.

        wait(vram); // Pide 1 de los 5 slots. Si están los 5 ocupados, espera.

        std::this_thread::sleep_for(std::chrono::milliseconds(450)); // Retardo de asignación a VRAM.
        logEvento(tarea, "VRAM_ASIGNADO");


        std::this_thread::sleep_for(std::chrono::milliseconds(600)); // Retardo durante renderizado.
        logEvento(tarea, "FINALIZADO");

        {
            // Mutex local para gestionar las tareas finalizadas.
            std::unique_lock<std::mutex> lock(mtx_finalizadas);
            tareasFinalizadas++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250)); // Retardo al liberar.
        signal(vram);
    }
}
