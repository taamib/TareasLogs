#ifndef EXTERNAL_QUICKSORT_HPP
#define EXTERNAL_QUICKSORT_HPP

#include <cstdio>

class ExternalQuickSort {
public:
    // Ordenar archivo usando Quicksort externo
    static void sort(FILE* input, FILE* output, size_t B, size_t M, int a, int& disk_access);

private:
    // Particionar el archivo usando pivotes
    static size_t partition(FILE* input, FILE* output, size_t low, size_t high, size_t B, int a, int& disk_access);

    // Seleccionar pivotes aleatorios desde un bloque
    static void select_pivots(FILE* file, uint64_t* pivots, int a, size_t B);
};

#endif