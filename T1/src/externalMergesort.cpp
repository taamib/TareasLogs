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
    // Vector que almacenará los nombres de los archivos temporales creados
    std::vector<std::string> partes;
    
    // Abrir el archivo de entrada en modo lectura binaria
    FILE* file = fopen(input_path.c_str(), "rb");
    if (!file) throw std::runtime_error("No se pudo abrir el archivo original");
    
    std::vector<uint64_t> bufferb(ELEMENTS_PER_BLOCK); // Buffer para almacenar un bloque de datos

    size_t restantes = N;
    size_t posactual = 0;
    size_t sub_N = (N + aridad - 1)/ aridad; // Tamaño aproximado de cada subarchivo

    // Crear cada subarchivo
    for (int i = 0; i < aridad && restantes > 0; i++) {
        std::string nombre = input_path + "_part" + std::to_string(i); // Generar nombre para el archivo temporal
        
        FILE* out = fopen(nombre.c_str(), "wb"); // Abrir archivo temporal en modo escritura binaria
        if (!out){
            fclose(file);
            throw std::runtime_error("No se pudo crear archivo temporal");
        }
        size_t a_escribir = std::min(sub_N, restantes); // Calcular cuántos elementos escribir en este subarchivo
        size_t escritos = 0; // Cuantos elementos ya escritos
        
        // Escribir los elementos en bloques
        while (escritos < a_escribir) {
            size_t cantidadleer = std::min(ELEMENTS_PER_BLOCK, a_escribir - escritos); // Calcular cuántos elementos leer en esta iteración (último bloque puede tener menos elementos)
            
            // Leer elementos del archivo original
            fseek(file, posactual*sizeof(uint64_t), SEEK_SET);
            fread(bufferb.data(), sizeof(uint64_t), cantidadleer, file);
            disk_access++;  // Contar acceso de lectura
            
            // Escribir elementos en el archivo temporal
            fwrite(bufferb.data(), sizeof(uint64_t), cantidadleer, out);
            disk_access++;  // Contar acceso de escritura
            
            escritos += cantidadleer;  // Actualizar contador de elementos escritos
            posactual += cantidadleer;
        }

        // Cerrar archivo temporal y agregar su nombre a la lista
        fclose(out);
        partes.push_back(nombre);
        restantes -= a_escribir;  // Actualizar elementos restantes
    }

    // Cerrar archivo original y devolver lista de archivos temporales
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

    // Verificar si el archivo cabe en memoria
    if (N <= M) {
        fseek(input, 0, SEEK_SET);
        size_t leidos = 0;
        size_t elementos_leer = std::min(ELEMENTS_PER_BLOCK, N - leidos);

        std::vector<uint64_t> data;
        data.reserve(N);
        std::vector<uint64_t> buffera(ELEMENTS_PER_BLOCK); // Buffer para almacenar todos los elementos del archivo
        while (leidos < N) {
            fread(buffera.data(), sizeof(uint64_t), elementos_leer, input);
            disk_access++;
            
            for (size_t i = 0; i < elementos_leer; i++) {
                data.push_back(buffera[i]);
            }
            leidos += elementos_leer;
        }
        fclose(input);
        std::sort(data.begin(), data.end()); // Ordenar los elementos en memoria

        std::string salida = input_path + "_ordenado"; // Nombre para el archivo ordenado
        
        FILE* output = fopen(salida.c_str(), "wb"); // Crear archivo de salida
        if (!output) {
            throw std::runtime_error("No se pudo crear archivo ordenado");
        }

        leidos = 0;
        while (leidos < N){
            size_t elementos_escribir = std::min(ELEMENTS_PER_BLOCK, N - leidos);
            fwrite(&data[leidos], sizeof(uint64_t), elementos_escribir, output);
            disk_access++;
            leidos += elementos_escribir;
        }
        fclose(output);
        return salida;
    }

    // Si el archivo no cabe en memoria, procesarlo recursivamente
    std::string temparch = input_path + "_temp";
    std::string salida = input_path + "_ordenado";
    
    // Llamar a mergesort_externo para archivos grandes
    size_t N_actual = N / sizeof(uint64_t);
    mergesort_externo(input_path, temparch, N, disk_access);

    // Renombrar el archivo temporal al nombre final
    if (std::rename(temparch.c_str(), salida.c_str()) != 0) {
        throw std::runtime_error("Error al renombrar archivo temporal");
    }
    return salida;
}

/**
* Realiza el merge de múltiples archivos ordenados en uno solo.
* Sus parámetros son
* Vector con rutas de archivos temporales ordenados
* Ruta del archivo de salida final
* Contador de accesos a disco (por referencia)
*/
void ExternalMergeSort::mergear_archivos(const std::vector<std::string>& archivos, const std::string& output, int& disk_access) {
    const size_t k = archivos.size(); // Número de archivos a fusionar

    std::vector<FILE*> fuentes(k, nullptr); // Array de handles para archivos de entrada
    std::vector<std::vector<uint64_t>> buffers(k, std::vector<uint64_t>(ELEMENTS_PER_BLOCK)); // Buffers para almacenar bloques de cada archivo (k buffers)
    std::vector<size_t> indices(k, 0); // Índices actuales dentro de cada buffer
    std::vector<size_t> sizes(k, 0); // Tamaños válidos en cada buffer
    std::vector<bool> activos(k, true); // Estado de cada archivo (activo/inactivo)

    // Abrir todos los archivos de entrada y leer su primer bloque
    for (size_t i = 0; i < k; ++i) {
        fuentes[i] = fopen(archivos[i].c_str(), "rb"); // Abrir archivo i-ésimo en modo lectura binaria
        if (!fuentes[i]) { // Verificar si hubo error
            // Limpieza de archivos ya abiertos antes de fallar
            for (size_t j = 0; j < i; ++j) { // Recorrer archivos previamente abiertos
                if (fuentes[j]) fclose(fuentes[j]); // Cerrar cada archivo abierto
            }
            throw std::runtime_error("Error abriendo archivo: " + archivos[i]); // Reportar error
        }
        sizes[i] = fread(buffers[i].data(),sizeof(uint64_t), ELEMENTS_PER_BLOCK, fuentes[i]); // Leer primer bloque del archivo
        disk_access++;                                                  // Incrementar contador de acceso a disco
    }

    FILE* out = fopen(output.c_str(), "wb"); // Abrir archivo de salida en modo escritura binaria
    if (!out) { // Verificar si hubo error
        for (size_t i = 0; i < k; ++i) { // Recorrer todos los archivos de entrada
            if (fuentes[i]) fclose(fuentes[i]); // Cerrar cada archivo abierto
        }
        throw std::runtime_error("Error creando archivo de salida"); // Reportar error
    }

    std::vector<uint64_t> buffer_out(ELEMENTS_PER_BLOCK); // Buffer para almacenar el bloque de salida
    size_t index_out = 0; // Índice actual en el buffer de salida

    // Comparador para el min-heap (cola de prioridad)
    auto cmp = [&](size_t a, size_t b) { // Función lambda para comparar elementos
        return buffers[a][indices[a]] > buffers[b][indices[b]]; // Comparar elementos de los buffers
    };
    std::priority_queue<size_t, std::vector<size_t>, decltype(cmp)>heap(cmp); // Crear min-heap                                                      // basado en el comparador

    // Inicializar el heap con los primeros elementos de cada archivo
    for (size_t i = 0; i < k; ++i) { // Recorrer todos los archivos
        if (sizes[i] > 0) { // Si el archivo tiene datos
            heap.push(i); // Agregar al heap
        } else { // Si el archivo está vacío
            activos[i] = false; // Marcarlo como inactivo
            fclose(fuentes[i]); // Cerrar archivo
            fuentes[i] = nullptr; // Marcar como nulo
        }
    }

    // Proceso principal de merge
    while (!heap.empty()) { // Mientras haya elementos en el heap
        size_t idx = heap.top(); // Obtener el índice del menor elemento
        heap.pop(); // Eliminar del heap

        buffer_out[index_out++] = buffers[idx][indices[idx]++]; // Copiar elemento al buffer de salida

        if (index_out == ELEMENTS_PER_BLOCK) { // Si el buffer de salida está lleno
            fwrite(buffer_out.data(), sizeof(uint64_t), ELEMENTS_PER_BLOCK, out);                                   // Escribir bloque completo
            disk_access++; // Incrementar contador de acceso
            index_out = 0; // Resetear índice de salida
        }

        if (indices[idx] == sizes[idx]) { // Si se llegó al final del buffer actual
            sizes[idx] = fread(buffers[idx].data(), sizeof(uint64_t), ELEMENTS_PER_BLOCK, fuentes[idx]); // Leer nuevo bloque del archivo
            disk_access++; // Incrementar contador de acceso
            indices[idx] = 0; // Resetear índice del buffer

            if (sizes[idx] == 0) { // Si no hay más datos en el archivo
                activos[idx] = false; // Marcarlo como inactivo
                fclose(fuentes[idx]); // Cerrar archivo
                fuentes[idx] = nullptr; // Marcar como nulo
            } else {
                heap.push(idx); // Volver a agregar al heap
            }
        } else {
            heap.push(idx); // Volver a agregar al heap
        }
    }

    // Escribir los datos restantes en el buffer de salida
    if (index_out > 0) { // Si quedan elementos por escribir
        fwrite(buffer_out.data(),sizeof(uint64_t), index_out, out); // Escribir elementos restantes
        disk_access++; // Incrementar contador de acceso
    }

    fclose(out); // Cerrar archivo de salida

    // Cerrar archivos de entrada que aún estén abiertos
    for (size_t i = 0; i < k; ++i) { // Recorrer todos los archivos
        if (fuentes[i]) fclose(fuentes[i]); // Cerrar si aún está abierto
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
    // CASO BASE: Si los datos caben en memoria
    if (N * sizeof(uint64_t) <= M) {
        std::string ordenado = ordenar_subarr(input_path, N, disk_access); // Ordenar el archivo completo en memoria
        std::rename(ordenado.c_str(), output_path.c_str()); // Renombrar el archivo temporal al nombre de salida final
        return;
    }

    // Partir el archivo grande en sub-arreglos más pequeños
    std::vector<std::string> partes = dividir_arr(input_path, N, disk_access);
    std::vector<std::string> ordenadas;

    // Procesar cada sub-arreglo recursivamente
    for (const auto& nombre : partes) {
        FILE* f = fopen(nombre.c_str(), "rb");
        if (!f){
            for (const auto& archivo : partes) std::remove(archivo.c_str());
            throw std::runtime_error("No se pudo abrir subarchivo para calcular tamaño");
        }
        fseek(f, 0, SEEK_END);
        size_t tam_bytes = ftell(f);
        fclose(f);
    
        size_t tam = tam_bytes / sizeof(uint64_t);  // cantidad real de elementos
        ordenadas.push_back(ordenar_subarr(nombre,tam, disk_access));
    }
    

    // Unir todos los sub-arreglos ordenados
    mergear_archivos(ordenadas, output_path, disk_access);

    // LIMPIEZA: Eliminar archivos temporales intermedios
    for (const auto& archivo : partes) std::remove(archivo.c_str());
    for (const auto& archivo : ordenadas) std::remove(archivo.c_str());
}
