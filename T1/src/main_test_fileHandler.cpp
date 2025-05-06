#include "../src/fileHandler.cpp"
#include <cstdio>
#include <iostream>
#include <random>
#include <cstdint>
#include <cstring>

int main() {
    const std::string filename = "test.bin";
    const size_t B = 4096; // Tamaño de bloque estándar (múltiplo de 8)
    const size_t elements = FileHandler::elements_per_block(B);

    // 1. Test básico de escritura/lectura
    std::cout << "=== Test 1: Escritura/Lectura Básica ===" << std::endl;
    try {
        FILE* file = fopen(filename.c_str(), "w+b");
        uint64_t write_data[elements];
        uint64_t read_data[elements];

        // Generar datos aleatorios
        std::mt19937_64 rng(std::random_device{}());
        for (size_t i = 0; i < elements; ++i) {
            write_data[i] = rng();
        }

        // Escribir y leer bloque 0
        FileHandler::write_block(file, 0, write_data, B);
        FileHandler::read_block(file, 0, read_data, B);

        // Verificar integridad
        if (memcmp(write_data, read_data, B) == 0) {
            std::cout << "Test 1 PASSED\n";
        } else {
            std::cerr << "Test 1 FAILED: Datos corruptos\n";
        }
        fclose(file);
    } catch (const std::exception& e) {
        std::cerr << "Test 1 FAILED: " << e.what() << "\n";
    }

    // 2. Test de conversiones
    std::cout << "\n=== Test 2: Conversiones Binario/Array ===" << std::endl;
    try {
        void* block = malloc(B);
        uint64_t original[elements];
        uint64_t converted[elements];

        // Llenar con patrón alternante
        for (size_t i = 0; i < elements; ++i) {
            original[i] = (i % 2 == 0) ? 0xAAAAAAAAAAAAAAAA : 0x5555555555555555;
        }

        FileHandler::array_to_block(original, block, B);
        FileHandler::block_to_array(block, converted, B);

        bool success = true;
        for (size_t i = 0; i < elements; ++i) {
            if (original[i] != converted[i]) {
                std::cerr << "Error en posición " << i << ": "
                          << std::hex << original[i] << " vs " << converted[i] << "\n";
                success = false;
            }
        }
        if (success) std::cout << "Test 2 PASSED\n";
        free(block);
    } catch (const std::exception& e) {
        std::cerr << "Test 2 FAILED: " << e.what() << "\n";
    }

    // 3. Test de límites y errores
    std::cout << "\n=== Test 3: Manejo de Errores ===" << std::endl;
    try {
        FILE* file = fopen("empty.bin", "w+b");
        uint64_t dummy[elements];

        // Test 3a: Lectura en archivo vacío
        try {
            FileHandler::read_block(file, 0, dummy, B);
            std::cerr << "Test 3a FAILED: No se lanzó excepción\n";
        } catch (const std::runtime_error&) {
            std::cout << "Test 3a PASSED\n";
        }

        // Test 3b: Escritura en bloque lejano
        try {
            const size_t big_block = static_cast<size_t>(1) << 34; // 16 GB
            FileHandler::write_block(file, big_block, dummy, B);
            std::cout << "Test 3b PASSED (sistemas 64-bit)\n";
        } catch (const std::exception& e) {
            std::cerr << "Test 3b WARNING: " << e.what() << "\n";
        }

        fclose(file);
    } catch (const std::exception& e) {
        std::cerr << "Test 3 FAILED: " << e.what() << "\n";
    }

    return 0;
}
