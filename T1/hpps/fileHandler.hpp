#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include <cstdio>
#include <cstdint>
#include <cassert>

class FileHandler {
public:
    // Leer un bloque del archivo
    static void read_block(FILE* file, size_t block_number, void* buffer, size_t B);

    // Escribir un bloque en el archivo
    static void write_block(FILE* file, size_t block_number, const void* buffer, size_t B);

    // Convertir bloque binario a arreglo de números (64 bits)
    static void block_to_array(const void* block, uint64_t* array, size_t elements);

    // Convertir arreglo a bloque binario
    static void array_to_block(const uint64_t* array, void* block, size_t elements);

    // Calcular elementos por bloque (B debe ser múltiplo de 8)
    static constexpr size_t elements_per_block(size_t B) {
        assert(B % sizeof(uint64_t) == 0 && "B debe ser múltiplo de 8");
        return B / sizeof(uint64_t);
    }
};

#endif