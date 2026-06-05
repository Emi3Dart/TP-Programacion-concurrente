#include "MessageQueue.h"
#include "Logger.h"
#include <chrono>

void MessageQueue::push(Job job) {
    // Tomamos posesividad total de la cola. Protege la inserción para que varios
    // Productores de API no colisionen intentando modificar el vector "cola" en simultáneo.
    std::lock_guard<std::mutex> lock(mtx);
    cola.push_back(job);

    if(job.id != -1) { // -1 es la píldora para matar workers. No la queremos en el log o lo ensuciará de basura
        globalLogger.log(job.id, job.prioridad, "EN_COLA");
    }

    // Notificamos a UN Worker (Consumidor) que estaba libre/durmiendo que ahora existe trabajo a realizar.
    cv_consumidor.notify_one();
}

Job MessageQueue::pop() {
    std::unique_lock<std::mutex> lock(mtx);

    // Requisito TP: Prueba de Vacuidad.
    // "Evalúa que los consumidores entren en espera... sin realizar consumo CPU innecesario"
    // Esta línea exactamente cumple eso. Evita el "Busy-Waiting" dejando el hilo
    // total y completamente inactivo (durmiendo) por orden de SO si la cola está vacía.
    cv_consumidor.wait(lock, [this]() { return !cola.empty(); });

    auto now = std::chrono::steady_clock::now();
    int index_a_procesar = -1;

    // ==========================================
    // ESTRATEGIA: AGING / EVITAR STARVATION
    // Requisito TP: Garantizar que Jobs "Free" no mueran por Premium
    // ==========================================
    for (size_t i = 0; i < cola.size(); ++i) {
        if (cola[i].prioridad == 1) {
            // Si encontramos un Premium (1), lo seleccionamos porque la regla es despacharlos primariamente
            index_a_procesar = i;
            break;
        } else {
            // Priority Free (0): Verificar si ha "envejecido" mucho
            // Tomamos el momento de creación, lo restamos al reloj actual y vemos por cuántos milisegundos esperó
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - cola[i].timestamp).count();

            if (duration >= MAX_WAIT_MS) {
                // Hilo envejeció demasiado (esperó 5000ms con un chorro Premium pisándolo).
                // Acá le inyectamos lógica de AGING: Le damos inmunidad absoluta, asume el turno
                // para ser despachado e interrumpe la búsqueda incluso pisando a posibles Premium futuros.
                index_a_procesar = i;
                globalLogger.log(cola[i].id, cola[i].prioridad, "PRIORIDAD_AGING"); // <-- Nuevo log de cambio de prioridad
                break;
            }
            if (index_a_procesar == -1) {
                // Si aún no vi a nadie, este free podría ser mi candidato inicial por descarte si total no había premiums
                index_a_procesar = i;
            }
        }
    }

    // Safety net: Si no matcheó lógicas complejas, es simplemente el elemento del frente de la fila.
    if (index_a_procesar == -1) {
        index_a_procesar = 0;
    }

    Job job_a_procesar = cola[index_a_procesar];

    // Eliminamos el trabajo y dejamos la fila más corta que antes.
    cola.erase(cola.begin() + index_a_procesar);

    return job_a_procesar;
}

bool MessageQueue::vacia() {
    std::lock_guard<std::mutex> lock(mtx);
    return cola.empty();
}
