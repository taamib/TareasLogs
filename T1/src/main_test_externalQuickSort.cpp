#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <stdio.h>
#include "../src/externalQuicksort.cpp"
#include "../src/test_externalQuickSort.cpp"

//typedef unsigned long long uint64_t;

int main() {
    // Configuración
    const std::string input_file = "input.bin";
    const std::string output_file = "output.bin";
    const size_t M = 50 * 1024 * 1024; // 50 MB (tamaño de memoria)
    const size_t total_elements = (M * 2) / sizeof(uint64_t); // 200 MB (~26.2 millones de elementos)
    const size_t B = 4096; // Tamaño de bloque (ej: 4KB)
    const int a = 5; // Número de particiones
    int disk_access = 0; // Contador de accesos a disco

    // Paso 1: Generar archivo aleatorio
    randBinaryFile(input_file, M, 8);

    // Paso 2: Abrir archivos (FILE* para C)
    FILE* input = fopen(input_file.c_str(), "rb");
    FILE* output = fopen(output_file.c_str(), "wb");
    if (!input || !output) {
        std::cerr << "Error al abrir archivos" << std::endl;
        return EXIT_FAILURE;
    }

    // Paso 3: Ordenar el archivo
    std::cout << "Ordenando con ExternalQuickSort..." << std::endl;
    ExternalQuickSort::sort(input, output, B, M, a, disk_access);
    std::cout << "Accesos a disco realizados: " << disk_access << std::endl;

    // Cerrar archivos
    fclose(input);
    fclose(output);

    // Paso 4: Verificar ordenación
    if (isFileSorted(output_file)) {
        std::cout << "✅ ¡Prueba exitosa! El archivo está ordenado." << std::endl;
    } else {
        std::cout << "❌ Error: El archivo NO está ordenado." << std::endl;
    }

    return EXIT_SUCCESS;
}