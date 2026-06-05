#include <iostream>
#include <thread>
#include <vector>
#include "Job.h"
#include "MessageQueue.h"
#include "VramPool.h"
#include "Logger.h"
#include "Workers.h"

// ==============================================================
// VARIABLES GLOBALES (Memoria compartida de todo el cluster)
// ==============================================================

MessageQueue msg_queue;
VramPool vram_pool;

std::mutex mtx_counter;
int total_jobs_finalizados = 0;

int main() {
    std::cout << "INICIANDO SISTEMA DE RENDERIZACION EN LA NUBE" << std::endl;

    int num_productores = 3;
    int num_consumidores = 1;
    int peticiones_por_producto = 5;

    std::vector<std::thread> productores;
    std::vector<std::thread> consumidores;

    std::cout << "-> Iniciando " << num_productores << " API Gateways (Productores)" << std::endl;
    for (int i = 0; i < num_productores; ++i) {
        productores.push_back(std::thread(productor_api, i+1, peticiones_por_producto));
    }

    std::cout << "-> Iniciando " << num_consumidores << " Worker Nodes (Consumidores)" << std::endl;
    for (int i = 0; i < num_consumidores; ++i) {
        consumidores.push_back(std::thread(consumidor_worker, i+1));
    }

    // Esperar a los productores
    for (auto& t : productores) {
        t.join();
    }
    std::cout << "[INFO] Todos los productores finalizaron de inyectar jobs." << std::endl;

    // Esperar a que la cola de mensajes esté vacía antes de enviar las píldoras de muerte.
    // Esto asegura que todos los jobs creados sean procesados.
    while (!msg_queue.vacia()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Enviar UNA píldora venenosa (-1) por CADA hilo consumidor activo para matarlos de forma segura.
    for (int i = 0; i < num_consumidores; ++i) {
        Job death_pill;
        death_pill.id = -1;
        msg_queue.push(death_pill);
    }

    // Esperar a los consumidores
    for (auto& t : consumidores) {
        t.join();
    }

    std::cout << "\n==============================================" << std::endl;
    std::cout << "FIN DE LA SIMULACION DE ENTORNO" << std::endl;
    std::cout << "Peticiones inyectadas teoricas  : " << (num_productores * peticiones_por_producto) << std::endl;
    std::cout << "Total Jobs Finalizados por VRAM : " << total_jobs_finalizados << std::endl;
    std::cout << "Verificar archivo 'sistema.log' " << std::endl;

    return 0;
}
