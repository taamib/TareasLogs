#ifndef EXTERNAL_MERGESORT_HPP
#define EXTERNAL_MERGESORT_HPP

#include <cstdio>

class ExternalMergeSort {
public:
    // Ordenar archivo usando Mergesort externo
    static void sort(FILE* input, FILE* output, size_t B, size_t M, int& disk_access);

    // Calcular aridad 'a' con búsqueda binaria
    static int calculate_arity(size_t B, size_t M);

private:
    // Función auxiliar para mezclar runs (implementar recursión)
    static void merge_runs(FILE* input, FILE* output, size_t left, size_t right, size_t B, size_t M, int& disk_access);


};

#endif