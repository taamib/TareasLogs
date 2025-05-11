#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <climits>
#include <ctime>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>

const char* FILENAME = "datos.bin";

struct ContadorEscritura_Lectura {
    int contadorEscritura = 0;
    int contadorLectura = 0;
} contadorEscritura_Lectura;

void guardarEnDisco(int64_t* arreglo, int64_t N, int64_t B) {
    FILE* archivo = fopen(FILENAME, "wb");
    if (!archivo) { perror("Error abriendo archivo"); exit(1); }

    int64_t elementosPorBloque = B / sizeof(int64_t);
    int64_t* buffer = new int64_t[elementosPorBloque];

    int64_t i = 0;
    while (i < N) {
        int64_t cantidad = (i + elementosPorBloque <= N) ? elementosPorBloque : N - i;
        memcpy(buffer, &arreglo[i], cantidad * sizeof(int64_t));
        if (cantidad < elementosPorBloque) {
            memset(&buffer[cantidad], 0, (elementosPorBloque - cantidad) * sizeof(int64_t));
        }
        fwrite(buffer, sizeof(int64_t), elementosPorBloque, archivo);
        contadorEscritura_Lectura.contadorEscritura++;
        i += elementosPorBloque;
    }

    delete[] buffer;
    fclose(archivo);
}

void crearSubarchivosOrdenados(int64_t N, int64_t B, int64_t M, int& totalSubarchivos) {
    FILE* archivo = fopen(FILENAME, "rb");
    if (!archivo) { perror("Error abriendo archivo original"); exit(1); }

    int64_t elementosEnRAM = M / sizeof(int64_t);
    int64_t* buffer = new int64_t[elementosEnRAM];
    totalSubarchivos = 0;

    for (int64_t i = 0; i < N; i += elementosEnRAM) {
        int64_t cantidad = (i + elementosEnRAM <= N) ? elementosEnRAM : N - i;
        fseek(archivo, i * sizeof(int64_t), SEEK_SET);
          if (fread(buffer, sizeof(int64_t), cantidad, archivo) != cantidad) {
                  perror("Error al leer del archivo");
                  exit(1);
              }
        std::sort(buffer, buffer + cantidad);

        char nombreArchivo[32];
        sprintf(nombreArchivo, "subarchivoOrdenado%d.bin", totalSubarchivos++);
        FILE* f = fopen(nombreArchivo, "wb");
        fwrite(buffer, sizeof(int64_t), cantidad, f);
        fclose(f);

        contadorEscritura_Lectura.contadorLectura++;
        contadorEscritura_Lectura.contadorEscritura++;
    }

    delete[] buffer;
    fclose(archivo);
}

void fusionarArchivosOrdenados(char** nombresArchivos, int k, int64_t B, const char* archivoSalida) {
    FILE* salida = fopen(archivoSalida, "wb");
    if (!salida) { perror("Error abriendo archivo de salida"); exit(1); }

    int64_t enterosPorBloque = B / sizeof(int64_t);
    FILE** archivos = new FILE*[k];
    int64_t** buffers = new int64_t*[k];
    int* indices = new int[k];
    int* tamaños = new int[k];
    bool* activo = new bool[k];

    for (int i = 0; i < k; ++i) {
        archivos[i] = fopen(nombresArchivos[i], "rb");
        buffers[i] = new int64_t[enterosPorBloque];
        tamaños[i] = fread(buffers[i], sizeof(int64_t), enterosPorBloque, archivos[i]);
        contadorEscritura_Lectura.contadorLectura++;
        indices[i] = 0;
        activo[i] = tamaños[i] > 0;
    }

    int64_t* bufferSalida = new int64_t[enterosPorBloque];
    int cantidadSalida = 0;

    while (true) {
        int64_t minimo = INT64_MAX;
        int origen = -1;
        for (int i = 0; i < k; ++i) {
            if (activo[i] && indices[i] < tamaños[i] && buffers[i][indices[i]] < minimo) {
                minimo = buffers[i][indices[i]];
                origen = i;
            }
        }
        if (origen == -1) break;

        bufferSalida[cantidadSalida++] = minimo;
        indices[origen]++;

        if (indices[origen] >= tamaños[origen]) {
            tamaños[origen] = fread(buffers[origen], sizeof(int64_t), enterosPorBloque, archivos[origen]);
            contadorEscritura_Lectura.contadorLectura++;
            indices[origen] = 0;
            activo[origen] = tamaños[origen] > 0;
        }

        if (cantidadSalida == enterosPorBloque) {
            fwrite(bufferSalida, sizeof(int64_t), cantidadSalida, salida);
            contadorEscritura_Lectura.contadorEscritura++;
            cantidadSalida = 0;
        }
    }

    if (cantidadSalida > 0) {
        fwrite(bufferSalida, sizeof(int64_t), cantidadSalida, salida);
        contadorEscritura_Lectura.contadorEscritura++;
    }

    for (int i = 0; i < k; ++i) {
        delete[] buffers[i];
        fclose(archivos[i]);
    }

    delete[] archivos;
    delete[] buffers;
    delete[] indices;
    delete[] tamaños;
    delete[] activo;
    delete[] bufferSalida;
    fclose(salida);
}

void mergesortExterno(int64_t N, int64_t B, int64_t M, int aridad) {
    int totalSubarchivos;
    crearSubarchivosOrdenados(N, B, M, totalSubarchivos);

    char** nombresSubarchivos = new char*[totalSubarchivos];
    for (int i = 0; i < totalSubarchivos; ++i) {
        nombresSubarchivos[i] = new char[32];
        sprintf(nombresSubarchivos[i], "subarchivoOrdenado%d.bin", i);
    }

    int etapa = 0;
    while (totalSubarchivos > 1) {
        int nuevosSubarchivosCount = (totalSubarchivos + aridad - 1) / aridad;
        char** nuevosNombresSubarchivos = new char*[nuevosSubarchivosCount];

        for (int i = 0; i < nuevosSubarchivosCount; ++i) {
            int inicio = i * aridad;
            int fin = (inicio + aridad <= totalSubarchivos) ? (inicio + aridad) : totalSubarchivos;
            int k = fin - inicio;

            nuevosNombresSubarchivos[i] = new char[32];
            sprintf(nuevosNombresSubarchivos[i], "merge_etapa%d_%d.bin", etapa, i);

            fusionarArchivosOrdenados(&nombresSubarchivos[inicio], k, B, nuevosNombresSubarchivos[i]);
        }

        for (int i = 0; i < totalSubarchivos; ++i) delete[] nombresSubarchivos[i];
        delete[] nombresSubarchivos;

        nombresSubarchivos = nuevosNombresSubarchivos;
        totalSubarchivos = nuevosSubarchivosCount;
        etapa++;
    }

    rename(nombresSubarchivos[0], "salida.bin");
    delete[] nombresSubarchivos[0];
    delete[] nombresSubarchivos;
}

int evaluarAridad(int64_t* arreglo, int64_t N, int64_t B, int64_t M, int aridad) {
    guardarEnDisco(arreglo, N, B);
    contadorEscritura_Lectura = {0, 0};
    mergesortExterno(N, B, M, aridad);
    return contadorEscritura_Lectura.contadorLectura + contadorEscritura_Lectura.contadorEscritura;
}

int encontrarMejorAridad(int64_t* arreglo, int64_t N, int64_t B, int64_t M) {
    int b = B / sizeof(int64_t);
    int low = 2, high = b;

    while (high - low > 3) {
        int m1 = low + (high - low) / 3;
        int m2 = high - (high - low) / 3;

        int costo1 = evaluarAridad(arreglo, N, B, M, m1);
        int costo2 = evaluarAridad(arreglo, N, B, M, m2);

        if (costo1 < costo2) {
            high = m2;
        } else {
            low = m1;
        }
    }

    int mejorA = low;
    int minCosto = evaluarAridad(arreglo, N, B, M, mejorA);
    for (int a = low + 1; a <= high; ++a) {
        int costo = evaluarAridad(arreglo, N, B, M, a);
        if (costo < minCosto) {
            minCosto = costo;
            mejorA = a;
        }
    }
    return mejorA;
}

void MergeSort() {
    const int64_t M = 50 * 1024 * 1024; // Memoria principal (50MB)
    const int64_t B = 4096;             // Tamaño del bloque (4KB)
    const int64_t N = 64;               // Número total de elementos pequeños para prueba rápida

    // Re-generar los datos para asegurar condiciones iniciales iguales
    int64_t arreglo[N];
    std::mt19937_64 rng(std::random_device{}());
    for (int64_t i = 0; i < N; ++i) arreglo[i] = i;
    std::shuffle(arreglo, arreglo + N, rng);

    // Determinar la mejor aridad
    int mejorAridad = encontrarMejorAridad(arreglo, N, B, M);

    // Re-escribir los datos
    guardarEnDisco(arreglo, N, B);

    // Ejecutar mergesort externo
    mergesortExterno(N, B, M, mejorAridad);
}

void QuickSort() {
    // No implementado aún
    std::cout << "[INFO] QuickSort externo no implementado.\n";
}

int main() {
    const int64_t N = 64;
    const int B = 64;

    std::mt19937_64 rng(std::random_device{}());
    int64_t arreglo[N];
    for (int64_t i = 0; i < N; ++i) arreglo[i] = i;
    std::shuffle(arreglo, arreglo + N, rng);

    // Escritura a disco
    auto t1 = std::chrono::high_resolution_clock::now();
    guardarEnDisco(arreglo, N, B);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::cout << "Escritura a disco completada en "
              << std::chrono::duration<double>(t2 - t1).count() << " segundos.\n";

    // MergeSort externo
    contadorEscritura_Lectura.contadorLectura = 0;
    contadorEscritura_Lectura.contadorEscritura = 0;
    auto t3 = std::chrono::high_resolution_clock::now();
    MergeSort();
    auto t4 = std::chrono::high_resolution_clock::now();
    std::cout << "[Merge] Tiempo: "
              << std::chrono::duration<double>(t4 - t3).count()
              << " s | Lecturas: " << contadorEscritura_Lectura.contadorLectura
              << " | Escrituras: " << contadorEscritura_Lectura.contadorEscritura << "\n";

    // QuickSort externo
    contadorEscritura_Lectura.contadorLectura = 0;
    contadorEscritura_Lectura.contadorEscritura = 0;
    auto t5 = std::chrono::high_resolution_clock::now();
    QuickSort();
    auto t6 = std::chrono::high_resolution_clock::now();
    std::cout << "[Quick] Tiempo: "
              << std::chrono::duration<double>(t6 - t5).count()
              << " s | Lecturas: " << contadorEscritura_Lectura.contadorLectura << "\n";

    return 0;
}