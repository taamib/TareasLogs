#include <iostream>
#include <chrono>
#include "../hpps/externalMergesort.hpp" // Incluye tu implementación

const size_t M = 50 * 1024 * 1024;
const size_t N = 60*M;
void probarMergesort(const std::string& inputPath) {
    ExternalMergeSort sorter(80); // Instancia de la clase
    int diskAccess = 0;       // Contador de accesos a disco

    // Calcular número de elementos en el archivo
    size_t numElementos = N/sizeof(uint64_t);

    auto start = std::chrono::high_resolution_clock::now(); // Iniciar cronómetro
    sorter.mergesort_externo(inputPath, inputPath + "_sorted", numElementos, diskAccess);
    auto end = std::chrono::high_resolution_clock::now();   // Finalizar cronómetro

    // Calcular tiempo de ejecución
    std::chrono::duration<double> elapsed = end - start;

    // Mostrar resultados
    std::cout << "Archivo ordenado generado: " << inputPath + "_sorted" << "\n";
    std::cout << "Número de elementos: " << numElementos << "\n";
    std::cout << "Tiempo de ejecución: " << elapsed.count() << " segundos\n";
    std::cout << "Accesos a disco: " << diskAccess << "\n";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <path archivo>\n";
        return 1;
    }

    std::string inputPath = argv[1];

    try {
        probarMergesort(inputPath);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}