#include "aging.h"
#include <chrono>

bool agingActivado(const Job& tarea) {
    // Calculamos cuánto tiempo pasó desde que el job fue creado
    auto espera = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - tarea.llegada).count(); // tarea.llegada guarda el momento en que se encoló el job

        // Si el job lleva esperando 5000 ms (5 segundos) o más
        // se activa el aging -> el job de baja prioridad deja de ser ignorado
        return espera >= 5000;
}
