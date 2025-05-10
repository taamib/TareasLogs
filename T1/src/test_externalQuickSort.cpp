#include "../hpps/externalQuicksort.hpp"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <algorithm>

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