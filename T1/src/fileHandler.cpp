#include "../hpps/fileHandler.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>

// Implementación de read_block
void FileHandler::read_block(FILE* file, const size_t block_number, void* buffer, size_t B) {
    // Calcula el offset en bytes multiplicando el número de bloque por el tamaño del bloque
    const uint64_t offset = static_cast<uint64_t>(block_number) * B;

    // Verificación del tamaño del archivo:
    // 1. Mueve el puntero al final del archivo para determinar su tamaño
    fseek(file, 0, SEEK_END);

    // 2. Obtiene el tamaño actual del archivo
    if (const long file_size = ftell(file); offset >= file_size) {
        // Si el offset solicitado es mayor que el tamaño del archivo, lanza excepción
        throw std::runtime_error("Offset inválido: bloque fuera del archivo");
    }

    // Posicionamiento y lectura:
    // 1. Mueve el puntero al offset calculado
    if (fseek(file, offset, SEEK_SET) != 0) {
        // Si fseek falla, lanza excepción
        throw std::runtime_error("Error en fseek (lectura)");
    }

    // 2. Lee exactamente B bytes del archivo al buffer
    if (const size_t read = fread(buffer, 1, B, file); read != B) {
        // Si no se leyeron todos los bytes, lanza excepción
        throw std::runtime_error("Error en fread: Bloque incompleto o inválido");
    }
}

// Implementación de write_block
void FileHandler::write_block(FILE* file, const size_t block_number, const void* buffer, const size_t B) {

    // Calcula el offset en bytes
    // Nota: Aquí hay un problema potencial porque block_number * B podría desbordar un long
    if (const long offset = static_cast<long>(block_number * B); fseek(file, offset, SEEK_SET) != 0) {
        // Si fseek falla, lanza excepción
        throw std::runtime_error("File seek error (write)");
    }

    // Escribe exactamente B bytes del buffer al archivo
    if (const size_t written = fwrite(buffer, 1, B, file); written != B) {
        // Si no se escribieron todos los bytes, lanza excepción
        throw std::runtime_error("File write error: incomplete write");
    }
}

// Implementación de block_to_array
void FileHandler::block_to_array(const void* block, uint64_t* array, size_t B) {
    // Verifica que el tamaño del bloque sea múltiplo del tamaño de uint64_t (8 bytes)
    if (B % sizeof(uint64_t) != 0) {
        throw std::runtime_error("Tamaño del bloque no es multiplo de 8");
    }
    // Copia los datos binarios del bloque al array de uint64_t
    memcpy(array, block, B);
}

// Implementación de array_to_block
void FileHandler::array_to_block(const uint64_t* array, void* block, size_t B) {
    // Verifica que el tamaño del bloque sea múltiplo del tamaño de uint64_t (8 bytes)
    if (B % sizeof(uint64_t) != 0) {
        throw std::runtime_error("Tamaño de bloque no es múltiplo de 8");
    }
    // Copia los datos del array de uint64_t al bloque binario
    memcpy(block, array, B);
}

