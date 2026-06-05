#include "VramPool.h"
#include "Logger.h"
#include <thread>
#include <chrono>

// Requisito TP "Capacidad ... a 5 slots": Se implementa usando init(Semaforo, int) en el constructor
VramPool::VramPool() {
    init(sem_VRAM, 5);
}

void VramPool::procesarJob(Job job, int& total_jobs_finalizados, std::mutex& mtx_total) {
    // 1. Exclusión limitante primaria 
    // Si la VRAM tiene 5 jobs corriendo (y nadie hizo signal para bajar la cuenta),
    // el SEM bloqueará implícitamente al Consumidor en este esayo puntual. 
    // Esto es CLAVE para la "Saturación de Recursos" mencionada en la Cátedra ("Prueba de Saturación 8_premium").
    wait(sem_VRAM);

    // 2. Registro e insercion formal a nuestra metadata
    {
        // Scope pequeño para lockear solo lo necesario con el mtx de este vector y soltarlo apenas se metió (reduce Livelocks colaterales)
        std::lock_guard<std::mutex> lock(mtx_slots);
        slots.push_back(job);
    }
    
    globalLogger.log(job.id, job.prioridad, "ASIGNADO_VRAM");
    
    // 3. Requisito TP Formal de Control de Flujo:
    // "mínimo de 600ms antes (..) dar por finalizado" && "retardo de 450ms entre asignaciones"
    // Hacemos que el hilo trabajador (Consumidor) simule la carga bruta del uso de la GPU pesada.
    // Esto lo saca completamente del escenario, durmiendo temporalmente y haciendo que se ocupe tiempo de vida.
    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // 4. Extracción: Borramos nuestra simulación lógica del arreglo
    {
        std::lock_guard<std::mutex> lock(mtx_slots);
        for (auto it = slots.begin(); it != slots.end(); ++it) {
            if(it->id == job.id) {
                slots.erase(it);
                break; // Lo encontre y rompo la busqueda. Me quedo tranquilo
            }
        }
    }

    // 5. SOLUCIÓN RACE CONDITION CONTADOR:
    // El Requisito "Gestionar acceso concurrente al contador global" de la cátedra se soluciona justo aquí.
    // Si no pusiéramos `lock_total(mtx_total)`, dos hilos a la vez haciendo ++ intentarían leer la variable Global de MAIN en el mismo slot 
    // de ASM de CPU y colisionarían de forma impredecible arrojando una suma incorrecta tras la "Prueba Masiva de 1500".
    {
        std::lock_guard<std::mutex> lock_total(mtx_total);
        total_jobs_finalizados++;
    }

    globalLogger.log(job.id, job.prioridad, "FINALIZADO");

    // 6. Requisito Protocolar final TP: "retiro.. Deberá existir retardo de 250ms de la liberacion".
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    
    // 7. Liberamos capacidad (up/signal): Le damos un tick extra al semáforo de capacidad para que 
    // un Worker pobre que estaba trabado en wait(sem_VRAM) logre despertar al principio y tomar la posta vacía.
    signal(sem_VRAM);
}
