#include "../hpps/experiment.hpp"
#include <vector>

#include "../hpps/externalMergesort.hpp"

int main() {
    const size_t B = 4096;  // Tamaño de bloque (ajustar según sistema)
    const size_t M = 50 * 1024 * 1024;  // 50 MB
    const std::vector<size_t> N_values = {4*M, 8*M, /*... hasta 60M */};

    // Paso 1: Calcular aridad 'a' (usar función de Mergesort)
    int a = ExternalMergeSort::calculate_arity(B, M);

    // Paso 2: Ejecutar experimentos para cada N
    for (size_t N : N_values) {
        Experiment::run_experiment(N, B, M, a);
    }

    return 0;
}
