#ifndef PCV_H
#define PCV_H

#include <string>
#include <chrono>

struct Job {
    int id; // Identificador unico del Job
    int prioridad; // 1 = premium / 0 = free
    std::chrono::steady_clock::time_point llegada;  // Medicion de tiempo de espera encolado (anti-starvation)
};

void apiGateway(); // Oroductor
void workerNode(); // Consumidor

void logEvento(const Job& tarea,
               const std::string& evento);

// ------------------------------
// CONFIGURACIÓN GENERAL DEL SISTEMA
// ------------------------------

const int tareasGateway = 10; // Cantidad de tareas que genera cada gateway. Ej = 3 productores (gateway) * 10 tareas = 30 Jobs.
// (PRUEBA DE VACUIDAD) tareasGateway = 0.
// (PRUEBA DE CARGA MASIVA) tareasGateway = 500 (Teniendo 3 productores) = 1500 jobs. ESCENARIO B Y C.
// (PRUEBA DE CARGA MASIVA) tareasGateway = 1500 (Teniendo 1 productor) = 1500 jobs. ESCENARIO A.

extern int NUM_WORKERS;
extern int NUM_GATEWAYS;
extern int tareasFinalizadas;

#endif // PCV_H_INCLUDED
