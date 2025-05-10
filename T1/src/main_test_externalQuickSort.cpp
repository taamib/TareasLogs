#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include "../src/externalQuicksort.cpp"


using namespace std;

//typedef unsigned long long uint64_t;

/**
 * Función que genera archivos binarios aleatorios, recibe como parámetros:
 * - filename: nombre del string a generar
 * - M: Tamaño de la memoria en bytes
 * - k: Multiplicador de la memoria para inidicar el tamaño del archivo (M * k)
 */
void randBinaryFile(const std::string& filename, size_t M, size_t k){

    size_t t_elements = (M * k) / sizeof(uint64_t); // Calculamos el total de elementos que tendrá el archivo
    cout << "Generando " << t_elements << "elementos" << endl;

    std:ofstream file(filename, std::ios::binary); // Chequeamos que el archivo se haya creado bien
    if (!file.is_open()){
        std::cerr << "Error al crear el archivo." << std::endl;
        exit(EXIT_FAILURE);
    }

    std::srand(std::time(0)); // Generamos una semilla aleatoria

    uint64_t* buff = new uint64_t[t_elements]; // Generamos los número aleatorios y los guardamos en un buffer
    for (size_t i = 0; i < t_elements; i++){
        buff[i] = static_cast<uint64_t>(std::rand()) << 32 | std::rand(); // Cada elemento del arreglo es un númeor de 64 bits aleatorio
    }

    delete[] buff; // Liberamos el buffer
    cout << "Archivo generado: " << filename << endl; // Confirmamos la creación
}


bool isFileSorted(const std::string& filename){


    std::ifstream file(filename, std::ios::binary); // Abrimos el archivo en modo binario


    if (!file.is_open()) { // Chequeamos que el archivo haya abierto correctamente
        std::cerr << "Error: No se pudo abrir " << filename << std::endl;
        return false;
    }


    uint64_t prev, current; // Creamos variables para ir comparando los elementos del archivo
    file.read(reinterpret_cast<char*>(&prev), sizeof(uint64_t)); // Leemos el primer número


    while (file.read(reinterpret_cast<char*>(&current), sizeof(uint64_t))) { // Revisamos el archivo completo
        if (current < prev) {
            return false; // Si encontramos un desorden, retornamos false
        }
        prev = current; // Vamos alternando las variables
    }

    file.close();
    std::cout << "✔ Archivo ordenado: " << filename << std::endl;
    return true;
}

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