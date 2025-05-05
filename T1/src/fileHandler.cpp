#include "../hpps/fileHandler.hpp"
#include <cstring>

void FileHandler::read_block(FILE* file, size_t block_number, void* buffer, size_t B) {
    // Implementar: fseek + fread
}

void FileHandler::write_block(FILE* file, size_t block_number, void* buffer, size_t B) {
    // Implementar: fseek + fwrite
}

// Resto de funciones (block_to_array, array_to_block)...
