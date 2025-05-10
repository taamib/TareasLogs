// Inclusión del archivo de cabecera que contiene la declaración de la clase FileHandler
#include "../hpps/fileHandler.hpp"

// Inclusión de bibliotecas estándar necesarias
#include <cstdio>      // Para operaciones de archivo (fseeko, ftello, fread, fwrite)
#include <cstring>     // Para memcpy
#include <stdexcept>   // Para std::runtime_error

// Define esto para asegurar off_t de 64 bits en sistemas compatibles
// Esto es importante para manejar archivos grandes (>2GB)
#define FILE_OFFSET_BITS 64

// Implementación del metodo read_block
void FileHandler::read_block(FILE* file, const size_t block_number, void* buffer, size_t B) {
    // Calcula el offset en bytes multiplicando el número de bloque por el tamaño del bloque
    const uint64_t offset = static_cast<uint64_t>(block_number) * B;

    // Mueve el puntero de archivo al final para verificar el tamaño
    if (fseeko(file, 0, SEEK_END) != 0) {
        throw std::runtime_error("Error al buscar el final del archivo");
    }

    // Obtiene el tamaño actual del archivo
    const off_t file_size = ftello(file);
    if (file_size == -1) {
        throw std::runtime_error("Error al obtener el tamaño del archivo");
    }

    // Verifica que el offset solicitado no exceda el tamaño del archivo
    if (offset >= static_cast<uint64_t>(file_size)) {
        throw std::runtime_error("Offset inválido: bloque fuera del archivo");
    }

    // Posiciona el puntero de archivo en el offset calculado
    if (fseeko(file, static_cast<off_t>(offset), SEEK_SET) != 0) {
        throw std::runtime_error("Error en fseeko (lectura)");
    }

    // Lee B bytes del archivo al buffer proporcionado
    if (const size_t read = fread(buffer, 1, B, file); read != B) {
        throw std::runtime_error("Error en fread: Bloque incompleto o inválido");
    }
}

// Implementación del metodo write_block
void FileHandler::write_block(FILE* file, const size_t block_number, const void* buffer, const size_t B) {
    // Calcula el offset y posiciona el puntero de archivo
    if (const uint64_t offset = static_cast<uint64_t>(block_number) * B;
        fseeko(file, static_cast<off_t>(offset), SEEK_SET) != 0) {
        throw std::runtime_error("Error en fseeko (escritura)");
    }

    // Escribe B bytes del buffer al archivo
    if (const size_t written = fwrite(buffer, 1, B, file); written != B) {
        throw std::runtime_error("Error en fwrite: Escritura incompleta");
    }
}

// Implementación del metodo block_to_array
void FileHandler::block_to_array(const void* block, uint64_t* array, size_t B) {
    // Verifica que el tamaño del bloque sea múltiplo de 8 (tamaño de uint64_t)
    if (B % sizeof(uint64_t) != 0) {
        throw std::runtime_error("Tamaño del bloque no es multiplo de 8");
    }

    // Copia los datos binarios del bloque al array de enteros de 64 bits
    memcpy(array, block, B);
}

// Implementación del metodo array_to_block
void FileHandler::array_to_block(const uint64_t* array, void* block, size_t B) {
    // Verifica que el tamaño del bloque sea múltiplo de 8 (tamaño de uint64_t)
    if (B % sizeof(uint64_t) != 0) {
        throw std::runtime_error("Tamaño de bloque no es múltiplo de 8");
    }

    // Copia los datos del array de enteros de 64 bits al bloque binario
    memcpy(block, array, B);
}
