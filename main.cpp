#include "pcv.h"
#include "semaforo.h"
#include <iostream>
#include <thread>
#include <queue>
#include <vector>

// Recursos compartidos.
std::queue<Job> premiumQueue;
std::queue<Job> freeQueue;

// Buffer 1.
Semaforo hay_espacio;
Semaforo hay_datos;

// Buffer 2.
Semaforo vram;

// Escenario A: NUM_GATEWAYS = 1 / NUM_WORKERS = 2;
// Escenario B: NUM_GATEWAYS = 3 / NUM_WORKERS = 1;
// Escenario C: NUM_GATEWAYS = 3 / NUM_WORKERS = 3;
// Variables globales.
int NUM_GATEWAYS = 3;
int NUM_WORKERS = 3;

int main() {

    init(hay_espacio, 50); // Cantidad de espacios.
    init(hay_datos, 0); // Inicializacion de espacios.
    init(vram, 5); // Pool de VRAM

    std::vector<std::thread> gateways; // Vector de gateways.
    std::vector<std::thread> workers; // Vector de workers.

    // API Gateways
    for (int i = 0; i < NUM_GATEWAYS; ++i) {
        gateways.emplace_back(apiGateway);
    }

    // Workers
    for (int i = 0; i < NUM_WORKERS; ++i) {
        workers.emplace_back(workerNode);
    }

    for (auto& g: gateways)
    {
        g.join();
    }

    for (auto& w: workers)
    {
        w.join();
    }

    // Contador final
    std::cout << "\nTareas finalizadas: "
          << tareasFinalizadas
          << std::endl;

    return 0;
}
