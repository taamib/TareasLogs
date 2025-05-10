#include "../hpps/externalMergesort.hpp"
#include <iostream>
#include <cstdio>
#include <random>
#include <vector>
#include <string>
#include <stdexcept>

// Constantes
const size_t B = 4096; // 4KB
const size_t ELEMENTS_PER_BLOCK = B / sizeof(uint64_t);

// Función para imprimir un vector
void imprimir_vector(const std::vector<uint64_t>& vec, const std::string& mensaje) {
    std::cout << mensaje;
    for (const auto& val : vec) {
        std::cout << val << " ";
    }
    std::cout << "\n";
}

// Función para leer un archivo binario y retornar un vector
std::vector<uint64_t> leer_archivo(const std::string& nombre) {
    FILE* file = fopen(nombre.c_str(), "rb");
    if (!file) throw std::runtime_error("No se pudo abrir el archivo: " + nombre);

    std::vector<uint64_t> datos;
    uint64_t valor;
    while (fread(&valor, sizeof(uint64_t), 1, file) == 1) {
        datos.push_back(valor);
    }
    fclose(file);
    return datos;
}

// Función para crear un archivo de prueba
void crear_archivo_prueba(const std::string& nombre, const std::vector<uint64_t>& datos) {
    FILE* file = fopen(nombre.c_str(), "wb");
    if (!file) throw std::runtime_error("Error creando archivo de prueba");

    size_t escritos = fwrite(datos.data(), sizeof(uint64_t), datos.size(), file);
    if (escritos != datos.size()) {
        fclose(file);
        throw std::runtime_error("Error escribiendo datos de prueba");
    }
    fclose(file);
}

// Función para verificar si un archivo está ordenado
bool esta_ordenado(const std::string& nombre) {
    FILE* file = fopen(nombre.c_str(), "rb");
    if (!file) return false;

    uint64_t previo, actual;
    bool primero = true;
    bool ordenado = true;

    while (fread(&actual, sizeof(uint64_t), 1, file) == 1) {
        if (!primero && actual < previo) {
            ordenado = false;
            break;
        }
        previo = actual;
        primero = false;
    }

    fclose(file);
    return ordenado;
}

int main() {
    try {
        // 1. Crear archivo de prueba con datos controlados
        const std::string input = "test_input.bin";
        const std::vector<uint64_t> datos_prueba = {9, 2, 5, 1, 8, 3, 7, 4, 6, 0};
        crear_archivo_prueba(input, datos_prueba);

        // 2. Imprimir arreglo inicial
        imprimir_vector(datos_prueba, "Arreglo inicial: ");

        // 3. Dividir el archivo en subarreglos
        ExternalMergeSort sorter;
        int disk_access = 0;
        size_t N = datos_prueba.size();
        std::vector<std::string> partes = sorter.dividir_arr(input, N, disk_access);

        // 4. Imprimir subarreglos
        for (size_t i = 0; i < partes.size(); ++i) {
            std::vector<uint64_t> subarreglo = leer_archivo(partes[i]);
            imprimir_vector(subarreglo, "Subarreglo " + std::to_string(i) + ": ");
        }

        // 5. Ordenar subarreglos
        std::vector<std::string> ordenadas;
        size_t sub_N = (N + partes.size() - 1) / partes.size();
        size_t restantes = N;

        for (const auto& nombre : partes) {
            size_t tam = std::min(sub_N, restantes);
            std::string ordenado = sorter.ordenar_subarr(nombre, sub_N, disk_access);
            ordenadas.push_back(ordenado);
            restantes -= tam;
        }

        // 6. Imprimir subarreglos ordenados
        for (size_t i = 0; i < ordenadas.size(); ++i) {
            std::vector<uint64_t> subarreglo_ordenado = leer_archivo(ordenadas[i]);
            imprimir_vector(subarreglo_ordenado, "Subarreglo ordenado " + std::to_string(i) + ": ");
        }

        // 7. Merge final
        const std::string output = "test_output.bin";
        sorter.mergear_archivos(ordenadas, output, disk_access);

        // 8. Imprimir arreglo final ordenado
        std::vector<uint64_t> resultado = leer_archivo(output);
        imprimir_vector(resultado, "Arreglo final ordenado: ");

        // 9. Verificar si el archivo está ordenado
        if (esta_ordenado(output)) {
            std::cout << "ÉXITO: El archivo está correctamente ordenado\n";
        } else {
            std::cout << "FALLO: El archivo no está ordenado\n";
        }
        std::cout << "Accesos a disco: " << disk_access << "\n";

        // 10. Limpieza
        remove(input.c_str());
        remove(output.c_str());
        for (const auto& archivo : partes) remove(archivo.c_str());
        for (const auto& archivo : ordenadas) remove(archivo.c_str());

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
