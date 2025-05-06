#include "../hpps/fileHandler.hpp"
#include <cstdio>
#include <cstring>
#include <stdexcept>


void FileHandler::read_block(FILE* file, const size_t block_number, void* buffer, size_t B) {
    const uint64_t offset = static_cast<uint64_t>(block_number) * B;
    // 1. Verificar si el offset es válido (necesita conocer el tamaño del archivo)
    fseek(file, 0, SEEK_END); // Ir al final del archivo
    if (const long file_size = ftell(file); offset >= file_size) {
        throw std::runtime_error("Offset inválido: bloque fuera del archivo");
    }

    // 2. Posicionarse y leer
    if (fseek(file, offset, SEEK_SET) != 0) {
        throw std::runtime_error("Error en fseek (lectura)");
    }

    if (const size_t read = fread(buffer, 1, B, file); read != B) {
        throw std::runtime_error("Error en fread: Bloque incompleto o inválido");
    }
}



void FileHandler::write_block(FILE* file, const size_t block_number, const void* buffer, const size_t B) {

    // Implementar: fseek + fwrite

    if (const long offset = static_cast<long>(block_number * B); fseek(file, offset, SEEK_SET) != 0) {
        throw std::runtime_error("File seek error (write)");
    }

    if (const size_t written = fwrite(buffer, 1, B, file); written != B) {
        throw std::runtime_error("File write error: incomplete write");
    }
}

void FileHandler::block_to_array(const void* block, uint64_t* array, size_t B) {
    if (B % sizeof(uint64_t) != 0) {
        throw std::runtime_error("Tamaño del bloque no es multiplo de 8");
    }
    memcpy(array, block, B);
}

void FileHandler::array_to_block(const uint64_t* array, void* block, size_t B) {
    if (B % sizeof(uint64_t) != 0) {
        throw std::runtime_error("Tamaño de bloque no es múltiplo de 8");
    }
    memcpy(block, array, B);
}

