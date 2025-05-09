#ifndef EXTERNAL_MERGESORT_HPP
#define EXTERNAL_MERGESORT_HPP

#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>


class ExternalMergeSort {
public:
    // Ordenar archivo usando Mergesort externo
    static void sort(FILE* input, FILE* output, size_t B, size_t M, int& disk_access);

    // Calcular aridad 'a' con búsqueda binaria
    static int calculate_arity(size_t B, size_t M);
    // Función auxiliar para mezclar runs (implementar recursión)
    static void merge_runs(FILE* input, FILE* output, size_t left, size_t right, size_t B, size_t M, int& disk_access);
    std::vector<std::string> dividir_arr(const std::string &input_path, size_t N, int &disk_access);
    std::string ordenar_subarr(const std::string &input_path, size_t N, int &disk_access);
    void mergear_archivos(const std::vector<std::string> &archivos, const std::string &output, int &disk_access);
    void mergesort_externo(const std::string &input_path, const std::string &output_path, size_t N, int &disk_access);
};

#endif