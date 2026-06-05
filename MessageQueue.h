#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include "Job.h"
#include <vector>
#include <mutex>
#include <condition_variable>

/**
 * Propósito: Define el "Buffer 1" solicitado en el TP. Es la sala de espera general
 * (Message Queue) de los jobs. Su tamaño es infinito lógicamente (se usa vector), 
 * pero hay que tener cuidado de que no vuele la RAM. 
 * Acá rige el escenario problemático Nro 5: El "Starvation" o Inanición.
 */
class MessageQueue {
private:
    std::vector<Job> cola; // Cola base, usamos vector para poder escanear fechas por debajo en lugar de solo leer el principio y fin o una simple 'queue' de c++ standard.
    
    // Estos dos controlan la concurrencia Básica Productor-Consumidor.
    std::mutex mtx; 
    std::condition_variable cv_consumidor; // Los 'Worker nodes' que estén libres esperarán acá tranquilos.
    
    // Requisito TP Anti-Starvation: Define tras qué lapso los Jobs "Free" 
    // dejan de ser ignorados y pasan al frente sin importar nada.
    const int MAX_WAIT_MS = 5000; 

public:
    // Método que llaman los Productores (APIs Gateway)
    void push(Job job);
    
    // Método que llaman los Consumidores (Worker Nodes) para llevarse un elemento
    Job pop();
    
    // Método diagnóstico (Opcional, puede servir en las pruebas de vacuidad)
    bool vacia();
};

#endif
