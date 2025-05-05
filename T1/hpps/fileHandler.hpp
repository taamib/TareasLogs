#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include <cstdio>
#include <cstdint>

class FileHandler {
public:
    // Leer un bloque del archivo
    static void read_block(FILE* file, size_t block_number, void* buffer, size_t B);

    // Escribir un bloque en el archivo
    static void write_block(FILE* file, size_t block_number, void* buffer, size_t B);

    // Convertir bloque binario a arreglo de números (64 bits)
    static void block_to_array(void* block, uint64_t* array, size_t elements);

    // Convertir arreglo a bloque binario
    static void array_to_block(uint64_t* array, void* block, size_t elements);
};

#endif