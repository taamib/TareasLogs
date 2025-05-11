#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <sys/stat.h>
#include <vector>
#include <algorithm>
#include "externalMergesort.hpp"
#include <climits>

class OptimalAridadFinder {
public:
    struct TestResult {
        int aridad;
        int ios;
        double tiempo;
        
        bool operator<(const TestResult& other) const {
            // Priorizamos exclusivamente el menor número de I/Os
            return ios < other.ios;
        }
    };

    // Función para calcular el número de elementos en el archivo binario
    static size_t calcular_num_elementos(const std::string& filepath) {
<<<<<<< HEAD
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("No se pudo abrir el archivo: " + filepath);
        }
        
        std::streamsize file_size = file.tellg();
        file.close();
        
        if (file_size < 0) {
            throw std::runtime_error("Error al obtener tamaño del archivo");
        }
        
        if (file_size % sizeof(uint64_t) != 0) {
            throw std::runtime_error("Tamaño de archivo no es múltiplo de sizeof(uint64_t)");
        }
        
        return static_cast<size_t>(file_size) / sizeof(uint64_t);
    }

    static TestResult evaluate_aridad(const std::string& input_path, size_t N, int aridad) {
        int disk_access = 0; // Seteamos los I/Os a 0

        std::cout << "[Prueba] Evaluando aridad = " << aridad << std::endl; // Printeamos qué aridad evaluaremos
        
        auto start = std::chrono::high_resolution_clock::now(); // Guardamos hora de comienzo

        ExternalMergeSort sorter(aridad); // Instancia de merge sorter
        sorter.mergesort_externo(input_path, input_path + "_temp_output", N, disk_access); // Usamos la función para ejecutar mergesort ecterno

        auto end = std::chrono::high_resolution_clock::now(); // Ya retornó la función asi que guardamos hora de finalización
        double elapsed = std::chrono::duration<double>(end - start).count(); // Calculamos cuánto fue el tiempo total

        std::remove((input_path + "_temp_output").c_str()); // Borramos el archivo

        // Retornamos en pantalla los resultados
=======
        struct stat st;
        if (stat(filepath.c_str(), &st) != 0) {
            throw std::runtime_error("No se pudo obtener el tamaño del archivo");
        }
        
        size_t file_size = st.st_size;
        if (file_size % sizeof(uint64_t) != 0) {
            throw std::runtime_error("Tamaño de archivo no coincide con tamaño de uint64_t");
        }
        
        return file_size / sizeof(uint64_t);
    }

    static TestResult evaluate_aridad(const std::string& input_path, size_t N, int aridad) {
        std::cout << "[Prueba] Evaluando aridad = " << aridad << std::endl;
        
        int disk_access = 0;
        auto start = std::chrono::high_resolution_clock::now();

        ExternalMergeSort sorter(aridad);
        sorter.mergesort_externo(input_path, input_path + "_temp_output", N, disk_access);

        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();

        std::remove((input_path + "_temp_output").c_str());

>>>>>>> 66a16f5437f36f809cb9870e9c9635d002c2448c
        std::cout << "[Resultado] Aridad " << aridad 
                  << " - I/Os: " << disk_access 
                  << " - Tiempo: " << elapsed << "s\n";

        return {aridad, disk_access, elapsed};
    }

    static int find_optimal_aridad(const std::string& input_path) {
        std::cout << "\n=== INICIANDO BUSQUEDA DE ARIDAD OPTIMA ===\n";
        
        // Calcular número de elementos automáticamente
        size_t N;
        try {
            N = calcular_num_elementos(input_path);
            std::cout << "Archivo: " << input_path << " (" << N << " elementos)\n";
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return -1;
        }
        const size_t B = 4096;
        // Calcular límite superior teórico basado en el tamaño del bloque
        const size_t max_possible_aridad = B / sizeof(uint64_t);
        int high = std::min(512, static_cast<int>(max_possible_aridad));
        int low = 2;
        
        std::cout << "Rango de busqueda: [" << low << ", " << high << "]\n";
        std::cout << "Maxima aridad teorica: " << max_possible_aridad << "\n\n";

        std::vector<TestResult> evaluated;
        TestResult best = {0, INT_MAX, 0.0};

        // Búsqueda ternaria con máximo 10 iteraciones
        for (int iter = 1; iter <= 10 && (high - low) >= 3; ++iter) {
            std::cout << "\n--- Iteracion " << iter << " ---\n";
            std::cout << "Rango actual: [" << low << ", " << high << "]\n";

            int mid1 = low + (high - low) / 3;
            int mid2 = high - (high - low) / 3;

            std::cout << "Puntos de evaluacion: " << mid1 << " y " << mid2 << "\n";

            auto res1 = evaluate_aridad(input_path, N, mid1);
            auto res2 = evaluate_aridad(input_path, N, mid2);

            evaluated.push_back(res1);
            evaluated.push_back(res2);

            if (res1 < best) best = res1;
            if (res2 < best) best = res2;

            if (res1.ios < res2.ios) {
                high = mid2;
                std::cout << "Mejor resultado en primera parte (" << mid1 << ")\n";
                std::cout << "Nuevo rango: [" << low << ", " << mid2 << "]\n";
            } else if (res2.ios < res1.ios) {
                low = mid1;
                std::cout << "Mejor resultado en segunda parte (" << mid2 << ")\n";
                std::cout << "Nuevo rango: [" << mid1 << ", " << high << "]\n";
            } else {
                // Si iguales, estrechamos el rango por ambos lados
                low = mid1;
                high = mid2;
                std::cout << "Mismos I/Os en ambos puntos\n";
                std::cout << "Nuevo rango: [" << mid1 << ", " << mid2 << "]\n";
            }
        }

        // Evaluar los puntos restantes en el rango final
        std::cout << "\n--- Evaluacion final del rango [" << low << ", " << high << "] ---\n";
        for (int aridad = low; aridad <= high; ++aridad) {
            auto res = evaluate_aridad(input_path, N, aridad);
            evaluated.push_back(res);
            if (res < best) best = res;
        }

        // Mostrar resultados
        std::cout << "\n=== RESULTADOS FINALES ===\n";
        std::cout << "Mejor aridad encontrada: " << best.aridad << "\n";
        std::cout << "Menor numero de I/Os: " << best.ios << "\n";
        std::cout << "Tiempo correspondiente: " << best.tiempo << "s\n";

        // Mostrar gráfico de resultados
        std::cout << "\nResumen de evaluaciones:\n";
        std::sort(evaluated.begin(), evaluated.end());
        for (const auto& res : evaluated) {
            std::cout << "Aridad " << res.aridad << ": " << res.ios << " I/Os";
            if (res.aridad == best.aridad) std::cout << " <-- MEJOR";
            std::cout << "\n";
        }

        return best.aridad;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <archivo_entrada.bin>\n";
        return 1;
    }

    std::string input_path = argv[1];
    int optimal_aridad = OptimalAridadFinder::find_optimal_aridad(input_path);

    if (optimal_aridad > 0) {
        std::cout << "\n[CONCLUSION] Aridad optima: " << optimal_aridad << "\n";
        return 0;
    } else {
        std::cerr << "\n[ERROR] No se pudo determinar la aridad optima\n";
        return 1;
    }
}