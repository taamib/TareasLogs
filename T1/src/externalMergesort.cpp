#include "../hpps/externalMergesort.hpp"
#include "fileHandler.cpp"
#include <algorithm>  
#include <cstdint>    
#include <stdexcept>  
#include <vector>
#include <string>
#include <cstdio>

const size_t M = 50 * 1024 * 1024; // TamAño de M que se indica que debe ser 50MB
const size_t B = 4096; // Tamaño de un bloque en disco que es 4Kb
const size_t ELEMENTS_PER_BLOCK = B / sizeof(uint64_t); // Cuántos elementos de 8 bytes caben en un bloque
#include "../hpps/externalMergesort.hpp"
#include "fileHandler.cpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <string>
#include <cstdio>
#include <queue>

const size_t M = 50 * 1024 * 1024; // 50MB
const size_t B = 4096; // 4KB
const size_t ELEMENTS_PER_BLOCK = B / sizeof(uint64_t);

ExternalMergeSort::ExternalMergeSort(size_t a) : aridad(a) {}

void ExternalMergeSort::mergesort_externo(const std::string& input_path, const std::string& output_path, size_t N) {
    if (N * sizeof(uint64_t) <= M) {
        // Caso base: ordeno en RAM
        std::vector<uint64_t> buffer(N);
        FILE* in = fopen(input_path.c_str(), "rb");
        fread(buffer.data(), sizeof(uint64_t), N, in);
        fclose(in);

        std::sort(buffer.begin(), buffer.end());

        FILE* out = fopen(output_path.c_str(), "wb");
        fwrite(buffer.data(), sizeof(uint64_t), N, out);
        fclose(out);
        return;
    }

    // Paso 1: dividir en 'a' subarreglos
    size_t sub_N = (N + aridad - 1) / aridad; // ceil(N / a)
    std::vector<std::string> sub_paths;

    FILE* input = fopen(input_path.c_str(), "rb");
    if (!input) throw std::runtime_error("No se pudo abrir archivo para dividir");

    for (size_t i = 0; i < aridad && i * sub_N < N; ++i) {
        std::string part_name = input_path + "_part" + std::to_string(i);
        FILE* out = fopen(part_name.c_str(), "wb");
        if (!out) throw std::runtime_error("No se pudo crear subarchivo");

        size_t cantidad = std::min(sub_N, N - i * sub_N);
        for (size_t j = 0; j < cantidad; ++j) {
            uint64_t x;
            fread(&x, sizeof(uint64_t), 1, input);
            fwrite(&x, sizeof(uint64_t), 1, out);
        }

        fclose(out);
        sub_paths.push_back(part_name);
    }

    fclose(input);

    // Paso 2: ordeno recursivamente cada uno
    std::vector<std::string> ordenados;
    for (size_t i = 0; i < sub_paths.size(); ++i) {
        std::string sorted = sub_paths[i] + "_sorted";
        size_t real_N = std::min(sub_N, N - i * sub_N);
        mergesort_externo(sub_paths[i], sorted, real_N);
        ordenados.push_back(sorted);
    }

    // Paso 3: merge final
    mergear_archivos(ordenados, output_path);
}

// estructura para el heap
struct EntradaHeap {
    uint64_t valor;
    size_t archivo;
    bool operator>(const EntradaHeap& otro) const {
        return valor > otro.valor;
    }
};

void ExternalMergeSort::mergear_archivos(const std::vector<std::string>& files, const std::string& output) {
    size_t k = files.size();
    std::vector<FILE*> f_in(k);
    std::vector<bool> terminado(k, false);
    std::vector<uint64_t> actual(k);

    for (size_t i = 0; i < k; ++i) {
        f_in[i] = fopen(files[i].c_str(), "rb");
        if (!f_in[i]) throw std::runtime_error("Error abriendo archivo para merge");
        if (fread(&actual[i], sizeof(uint64_t), 1, f_in[i]) != 1)
            terminado[i] = true;
    }

    FILE* f_out = fopen(output.c_str(), "wb");
    if (!f_out) throw std::runtime_error("No se pudo crear archivo de salida");

    std::priority_queue<EntradaHeap, std::vector<EntradaHeap>, std::greater<>> heap;
    for (size_t i = 0; i < k; ++i) {
        if (!terminado[i]) {
            heap.push({actual[i], i});
        }
    }

    while (!heap.empty()) {
        EntradaHeap top = heap.top(); heap.pop();
        fwrite(&top.valor, sizeof(uint64_t), 1, f_out);

        if (fread(&actual[top.archivo], sizeof(uint64_t), 1, f_in[top.archivo]) == 1) {
            heap.push({actual[top.archivo], top.archivo});
        } else {
            fclose(f_in[top.archivo]);
            terminado[top.archivo] = true;
        }
    }

    fclose(f_out);
}
