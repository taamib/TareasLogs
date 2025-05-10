#include "../hpps/externalMergesort.hpp"
#include "../hpps/fileHandler.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <queue>

const size_t M = 50 * 1024 * 1024;
const size_t B = 4096;
const size_t ELEMENTS_PER_BLOCK = B / sizeof(uint64_t);
const int aridad = 2;

std::vector<std::string> ExternalMergeSort::dividir_arr(const std::string& input_path, size_t N, int& disk_access) {
    std::vector<std::string> partes;
    FILE* input = fopen(input_path.c_str(), "rb");
    if (!input) throw std::runtime_error("No se pudo abrir el archivo original");

    size_t sub_N = (N + aridad - 1) / aridad;
    size_t restantes = N;
    std::vector<uint8_t> block(B);
    std::vector<uint64_t> buffer(ELEMENTS_PER_BLOCK);

    for (int i = 0; i < aridad && restantes > 0; i++) {
        std::string nombre = input_path + "_part" + std::to_string(i);
        FILE* out = fopen(nombre.c_str(), "wb");
        if (!out) throw std::runtime_error("No se pudo crear archivo temporal");

        size_t a_escribir = std::min(sub_N, restantes);
        size_t escritos = 0;

        while (escritos < a_escribir) {
            size_t cantidad = std::min(ELEMENTS_PER_BLOCK, a_escribir - escritos);
            size_t posicion_inicial = (N - restantes + escritos);

            // Leer directamente los elementos (no por bloques completos)
            fseek(input, posicion_inicial * sizeof(uint64_t), SEEK_SET);
            fread(buffer.data(), sizeof(uint64_t), cantidad, input);
            disk_access++;

            // Escribir los elementos
            fseek(out, escritos * sizeof(uint64_t), SEEK_SET);
            fwrite(buffer.data(), sizeof(uint64_t), cantidad, out);
            disk_access++;

            escritos += cantidad;
        }

        fclose(out);
        partes.push_back(nombre);
        restantes -= a_escribir;
    }

    fclose(input);
    return partes;
}

std::string ExternalMergeSort::ordenar_subarr(const std::string& input_path, size_t N, int& disk_access) {
    if (N * sizeof(uint64_t) <= M) {
        std::vector<uint64_t> buffer(N);
        FILE* input = fopen(input_path.c_str(), "rb");
        if (!input) throw std::runtime_error("No se pudo abrir archivo para ordenar");

        // Leer todos los elementos de una vez
        fread(buffer.data(), sizeof(uint64_t), N, input);
        disk_access++;
        fclose(input);

        std::sort(buffer.begin(), buffer.end());

        std::string salida = input_path + "_ordenado";
        FILE* output = fopen(salida.c_str(), "wb");
        if (!output) throw std::runtime_error("No se pudo crear archivo ordenado");

        // Escribir todos los elementos de una vez
        fwrite(buffer.data(), sizeof(uint64_t), N, output);
        disk_access++;
        fclose(output);

        return salida;
    }

    std::string salida = input_path + "_ordenado";
    mergesort_externo(input_path, salida, N, disk_access);
    return salida;
}

void ExternalMergeSort::mergear_archivos(const std::vector<std::string>& archivos, const std::string& output, int& disk_access) {
    size_t k = archivos.size();
    std::vector<FILE*> fuentes(k);
    std::vector<std::vector<uint64_t>> buffers(k, std::vector<uint64_t>(ELEMENTS_PER_BLOCK));
    std::vector<size_t> indices(k, 0);
    std::vector<size_t> sizes(k, 0);
    std::vector<bool> activos(k, true);

    // Abrir archivos y cargar los primeros bloques
    for (size_t i = 0; i < k; ++i) {
        fuentes[i] = fopen(archivos[i].c_str(), "rb");
        if (!fuentes[i]) throw std::runtime_error("No se pudo abrir subarchivo");
        sizes[i] = fread(buffers[i].data(), sizeof(uint64_t), ELEMENTS_PER_BLOCK, fuentes[i]);
        disk_access++;
    }

    FILE* out = fopen(output.c_str(), "wb");
    if (!out) throw std::runtime_error("No se pudo crear archivo de salida");

    std::vector<uint64_t> buffer_out(ELEMENTS_PER_BLOCK);
    size_t index_out = 0;

    auto cmp = [&](size_t a, size_t b) {
        return buffers[a][indices[a]] > buffers[b][indices[b]];
    };
    std::priority_queue<size_t, std::vector<size_t>, decltype(cmp)> heap(cmp);

    for (size_t i = 0; i < k; ++i) {
        if (sizes[i] > 0) {
            heap.push(i);
        } else {
            activos[i] = false;
            fclose(fuentes[i]);
        }
    }

    while (!heap.empty()) {
        size_t idx = heap.top(); heap.pop();
        buffer_out[index_out++] = buffers[idx][indices[idx]++];
        if (index_out == ELEMENTS_PER_BLOCK) {
            fwrite(buffer_out.data(), sizeof(uint64_t), ELEMENTS_PER_BLOCK, out);
            disk_access++;
            index_out = 0;
        }

        if (indices[idx] == sizes[idx]) {
            sizes[idx] = fread(buffers[idx].data(), sizeof(uint64_t), ELEMENTS_PER_BLOCK, fuentes[idx]);
            disk_access++;
            indices[idx] = 0;
            if (sizes[idx] == 0) {
                activos[idx] = false;
                fclose(fuentes[idx]);
            } else {
                heap.push(idx);
            }
        } else {
            heap.push(idx);
        }
    }

    if (index_out > 0) {
        fwrite(buffer_out.data(), sizeof(uint64_t), index_out, out);
        disk_access++;
    }

    fclose(out);
}


void ExternalMergeSort::mergesort_externo(const std::string& input_path, const std::string& output_path, size_t N, int& disk_access) {
    if (N * sizeof(uint64_t) <= M) {
        std::string ordenado = ordenar_subarr(input_path, N, disk_access);
        std::rename(ordenado.c_str(), output_path.c_str());
        return;
    }

    std::vector<std::string> partes = dividir_arr(input_path, N, disk_access);
    std::vector<std::string> ordenadas;
    size_t sub_N = (N + aridad - 1) / aridad;
    size_t restantes = N;

    for (const auto& nombre : partes) {
        size_t tam = std::min(sub_N, restantes);
        ordenadas.push_back(ordenar_subarr(nombre, tam, disk_access));
        restantes -= tam;
    }

    mergear_archivos(ordenadas, output_path, disk_access);

    for (const auto& archivo : partes) std::remove(archivo.c_str());
    for (const auto& archivo : ordenadas) std::remove(archivo.c_str());
}