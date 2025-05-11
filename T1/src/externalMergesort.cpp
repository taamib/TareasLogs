#include "../hpps/externalMergesort.hpp"
#include "../hpps/fileHandler.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <string>
#include <cstdio>
#include <queue>

const size_t M = 50 * 1024 * 1024; // 
const size_t B = 4096;
const size_t ELEMENTS_PER_BLOCK = B / sizeof(uint64_t);

/** Divide un arreglo en a subarreglos del mismo tamaño
* Sus parámetros son:
* String con path del archivo
* Tamaño N del arreglo (cantidad de elementos que caben en el archivo)
* Accesos a disco que es para ir contando cuando se accede a memoria secundaria
*/
std::vector<std::string> ExternalMergeSort::dividir_arr(const std::string& input_path, size_t N, int& disk_access) {
    std::vector<std::string> partes;
    FILE* file = fopen(input_path.c_str(), "rb");
    if (!file) throw std::runtime_error("No se pudo abrir el archivo original");
    
    std::vector<uint64_t> buffer(ELEMENTS_PER_BLOCK); // Creamos buffer de tamaño B 
    size_t elements_per_part = (N + aridad - 1) / aridad;

    for (int i = 0; i < aridad && N > 0; ++i) {
        std::string part_name = input_path + "_part" + std::to_string(i);
        FILE* out = fopen(part_name.c_str(), "wb");
        if (!out) {
            fclose(file);
            throw std::runtime_error("No se pudo crear archivo temporal");
        }

        size_t to_write = std::min(elements_per_part, N);

        std::vector<uint64_t> temp_buffer;
        temp_buffer.reserve(elements_per_part); // Reservar espacio para toda la partición
        size_t written = 0;

        while (written < to_write) {
            size_t to_read = std::min(ELEMENTS_PER_BLOCK, to_write - written);
            fread(buffer.data(), sizeof(uint64_t), to_read, file);
            disk_access++;

            temp_buffer.insert(temp_buffer.end(), buffer.begin(), buffer.begin() + to_read);
            written += to_read;
        }
        fwrite(temp_buffer.data(), sizeof(uint64_t), temp_buffer.size(), out);
        disk_access++;

        fclose(out);
        partes.push_back(part_name);
        N -= to_write;
    }

    fclose(file);
    return partes;
}

/**Ordena un subarchivo en memoria o recursivamente si es demasiado grande.
 * Sus parámetros son:
 * String con path del archivo
 * Tamaño N del arreglo (cantidad de elementos que caben en el archivo)
 * Accesos a disco que es para ir contando cuando se accede a memoria secundaria
 */
std::string ExternalMergeSort::ordenar_subarr(const std::string& input_path, size_t N, int& disk_access) {
    FILE* input = fopen(input_path.c_str(), "rb");
    if (!input) throw std::runtime_error("No se pudo abrir archivo para ordenar");

    if (N * sizeof(uint64_t) <= M) {
        // Cargar en memoria en bloques para evitar sobrecarga
        std::vector<uint64_t> data;
        data.reserve(N);
        std::vector<uint64_t> buffer(ELEMENTS_PER_BLOCK);

        size_t remaining = N;
        while (remaining > 0) {
            size_t to_read = std::min(ELEMENTS_PER_BLOCK, remaining);
            fread(data.data() + (N - remaining), sizeof(uint64_t), to_read, input);
            disk_access++;
            remaining -= to_read;
        }
        
        fclose(input);

        std::sort(data.begin(), data.end());

        std::string output_name = input_path + "_ordenado";
        FILE* output = fopen(output_name.c_str(), "wb");
        if (!output) throw std::runtime_error("No se pudo crear archivo ordenado");

        remaining = N;
        size_t pos = 0;
        while (remaining > 0) {
            size_t to_write = std::min(ELEMENTS_PER_BLOCK, remaining);
            fwrite(&data[pos], sizeof(uint64_t), to_write, output);
            disk_access++;
            pos += to_write;
            remaining -= to_write;
        }
        fclose(output);
        return output_name;
    }

    // Caso recursivo
    std::string temp_output = input_path + "_temp";
    mergesort_externo(input_path, temp_output, N, disk_access);
    
    std::string final_output = input_path + "_ordenado";
    if (std::rename(temp_output.c_str(), final_output.c_str()) != 0) {
        throw std::runtime_error("Error al renombrar archivo temporal");
    }
    return final_output;
}

/**
* Realiza el merge de múltiples archivos ordenados en uno solo.
* Sus parámetros son
* Vector con rutas de archivos temporales ordenados
* Ruta del archivo de salida final
* Contador de accesos a disco (por referencia)
*/
void ExternalMergeSort::mergear_archivos(const std::vector<std::string>& archivos, const std::string& output, int& disk_access) {
    const size_t k = archivos.size();
    if (k == 0) return;

    // Estructura para manejar los buffers de entrada
    struct FileBuffer {
        FILE* file;
        std::vector<uint64_t> buffer;
        size_t current_pos;
        size_t valid_items;
        bool active;
        
        FileBuffer() : file(nullptr), current_pos(0), valid_items(0), active(false) {}
    };

    std::vector<FileBuffer> file_buffers(k);
    std::vector<uint64_t> output_buffer(ELEMENTS_PER_BLOCK);
    size_t output_pos = 0;

    // Abrir archivos y cargar primeros bloques
    for (size_t i = 0; i < k; ++i) {
        file_buffers[i].file = fopen(archivos[i].c_str(), "rb");
        if (!file_buffers[i].file) {
            for (size_t j = 0; j < i; ++j) {
                if (file_buffers[j].file) fclose(file_buffers[j].file);
            }
            throw std::runtime_error("Error abriendo archivo para merge");
        }
        
        file_buffers[i].buffer.resize(ELEMENTS_PER_BLOCK);
        file_buffers[i].valid_items = fread(file_buffers[i].buffer.data(), sizeof(uint64_t), ELEMENTS_PER_BLOCK, file_buffers[i].file);
        disk_access++;
        file_buffers[i].active = (file_buffers[i].valid_items > 0);
        file_buffers[i].current_pos = 0;
    }

    // Comparador para el min-heap
    auto cmp = [&](size_t a, size_t b) {
        return file_buffers[a].buffer[file_buffers[a].current_pos] > file_buffers[b].buffer[file_buffers[b].current_pos];
    };
    std::priority_queue<size_t, std::vector<size_t>, decltype(cmp)> heap(cmp);

    // Inicializar heap con archivos activos
    for (size_t i = 0; i < k; ++i) {
        if (file_buffers[i].active) {
            heap.push(i);
        }
    }

    FILE* out_file = fopen(output.c_str(), "wb");
    if (!out_file) {
        for (auto& fb : file_buffers) {
            if (fb.file) fclose(fb.file);
        }
        throw std::runtime_error("Error creando archivo de salida");
    }

    // Proceso principal de merge
    while (!heap.empty()) {
        size_t current = heap.top();
        heap.pop();

        // Escribir elemento al buffer de salida
        output_buffer[output_pos++] = file_buffers[current].buffer[file_buffers[current].current_pos++];

        // Buffer de salida lleno?
        if (output_pos == ELEMENTS_PER_BLOCK) {
            fwrite(output_buffer.data(), sizeof(uint64_t), output_pos, out_file);
            disk_access++;
            output_pos = 0;
        }

        // Necesitamos recargar buffer de este archivo?
        if (file_buffers[current].current_pos >= file_buffers[current].valid_items) {
            file_buffers[current].valid_items = fread(
                file_buffers[current].buffer.data(), 
                sizeof(uint64_t), 
                ELEMENTS_PER_BLOCK, 
                file_buffers[current].file
            );
            disk_access++;
            file_buffers[current].current_pos = 0;
            file_buffers[current].active = (file_buffers[current].valid_items > 0);
        }

        // Volver a meter en el heap si todavía tiene datos
        if (file_buffers[current].active) {
            heap.push(current);
        } else {
            fclose(file_buffers[current].file);
            file_buffers[current].file = nullptr;
        }
    }

    // Escribir los elementos restantes en el buffer de salida
    if (output_pos > 0) {
        fwrite(output_buffer.data(), sizeof(uint64_t), output_pos, out_file);
        disk_access++;
    }

    fclose(out_file);

    // Cerrar archivos que queden abiertos
    for (auto& fb : file_buffers) {
        if (fb.file) fclose(fb.file);
    }
}

/** Implementa el algoritmo de MergeSort externo para ordenar archivos grandes que no caben en memoria.
 * Sus parámetros son:
 * String con path del archivo de entrada
 * STring con path archivo de salida
 * Tamaño N del arreglo (cantidad de elementos que caben en el archivo)
 * Accesos a disco que es para ir contando cuando se accede a memoria secundaria
 */
void ExternalMergeSort::mergesort_externo(const std::string& input_path, const std::string& output_path, size_t N, int& disk_access) {
    // Caso base: cabe en memoria
    if (N * sizeof(uint64_t) <= M) {
        std::string sorted = ordenar_subarr(input_path, N, disk_access);
        if (std::rename(sorted.c_str(), output_path.c_str()) != 0) {
            throw std::runtime_error("Error al renombrar archivo ordenado");
        }
        return;
    }

    // Dividir
    std::vector<std::string> parts = dividir_arr(input_path, N, disk_access);
    std::vector<std::string> sorted_parts;
    sorted_parts.reserve(parts.size());

    // Ordenar cada parte
    for (const auto& part : parts) {
        FILE* f = fopen(part.c_str(), "rb");
        if (!f) {
            for (const auto& p : parts) std::remove(p.c_str());
            throw std::runtime_error("Error abriendo parte para calcular tamaño");
        }
        
        fseek(f, 0, SEEK_END);
        size_t size_bytes = ftell(f);
        fclose(f);
        
        size_t part_size = size_bytes / sizeof(uint64_t);
        sorted_parts.push_back(ordenar_subarr(part, part_size, disk_access));
    }

    // Mergear
    mergear_archivos(sorted_parts, output_path, disk_access);

    // Limpieza
    for (const auto& part : parts) std::remove(part.c_str());
    for (const auto& sorted : sorted_parts) std::remove(sorted.c_str());
}
