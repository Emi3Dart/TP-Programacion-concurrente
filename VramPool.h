#ifndef VRAM_POOL_H
#define VRAM_POOL_H

#include "Job.h"
#include "Semaforo.h"
#include <vector>
#include <mutex>

/**
 * Propósito: Define el "Buffer 2" solicitado en el TP. Es el espacio de las gráficas.
 * Almacén estrictamente persistente donde múltiples workers colisionan porque
 * hay límite bajo de espacios.
 * Escenario TP contemplado acá acá: Exclusión Mutua perfecta para un "Pool de VRAM limite 5".
 */
class VramPool {
private:
    std::vector<Job> slots; // Esquemático para ver "quién" está procesando. 
    
    // ESTA herramienta es la magia de esta clase para la exclusión mutuay el límite límite:
    // Al inicializarse en 5, garantiza que bajo absolutamenten inguna tormenta
    // pasarán 6 instancias a la vez al array de "slots". 
    Semaforo sem_VRAM; 
    
    // Este Mutex lo metemos para que, cuando toque editar el vector de `slots` expliciatemente 
    // en memoria, los Worker que de todos modos _sí\_ pasaron el filtro de capacidad_ no se colisionen en la reasignación vector[+]
    std::mutex mtx_slots; 

public:
    VramPool();
    
    // Método que modela todo el proceso del trabajador (asignar, 450ms, calcular, 600ms, borrar, 250ms).
    // OJO a las referencias (&): Pasan la variable contador y su mutex protector (exigido x TP).
    void procesarJob(Job job, int& total_jobs_finalizados, std::mutex& mtx_total);
};

#endif
