#ifndef EXPERIMENT_HPP
#define EXPERIMENT_HPP

#include <string>

class Experiment {
public:
    // Generar archivo binario con números aleatorios
    static void generate_data(const std::string& filename, size_t N);

    // Ejecutar experimento para un tamaño N
    static void run_experiment(size_t N, size_t B, size_t M, int a);
};

#endif