#include "../hpps/externalQuicksort.hpp"
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

struct Resultados {
    float tiempo;
    int accesos;
};

/**
 * Los parámetros de la funcion sort son:
 * -input: archivo de entrada desordenado
 * -output: archivo de salida donde escribiremos el resultado ordenado
 * -B: tamaño de un bloque
 * -M: tamaño máximo que puede procesarse en memoria
 * -a: número de particiones
 * -disk_acces: contador de accesos a discos
 */
void ExternalQuickSort::sort(FILE* input, FILE* output, size_t B, size_t M, int a, int& disk_access){
    // Lo primero que hacemos es determinar el tamaño del archivo para ver que caso seguir
    fseek(input, 0, SEEK_END); // Movemos el cursor al final del archivo
    size_t f_size = ftell(input); // Accedemos a la posición actual, que en este caso coresponde al tamaño del archivo
    fseek(input, 0, SEEK_SET); // Devolvemos el cursos al inicio
    disk_access += 2; // Contamos los dos I/O (uno por cada fseek)

    // Caso base (el archivo es suficientemente pequeño para ser ordenado en memoria)
    if (f_size <= M){
        // Notemos que la cantidad de elementos del archivo corresponderá al tamaño total del archivo dividio en el tamaño de en bytes de cada uno.
        // (por eso usamos constantemente 'f_size / sizeof(uint64_t)')
        uint64_t* buff = new uint64_t[f_size / sizeof(uint64_t)]; // Creamos un buffer para guardar los datos del archivo

        fread(buff, sizeof(uint64_t), f_size/sizeof(uint64_t),input); // Leemos el archivo en el buffer
        disk_access++; // Aumentamos un I/O por el fread

        // Notemos que std::sort recibe punteros al inicio y al final de un subarreglo, y ordena desde inicio a final -1
        std::sort(buff,buff + (f_size / sizeof(uint64_t))); // Ordenamos los datos en memoria

        fwrite(buff, sizeof(uint64_t), f_size / sizeof(uint64_t), output); // Escribimos los datos ya ordenados en el output
        disk_access++;// Aumentamos un I/O por el fwrite

        delete[] buff;// Liberamos el buffer para no tener problemas con el espacio
        return; // Finalizamos
    }
    
    // Caso recursivo (el archivo no cabe en memoria)
    // De no caber en memoria, el archivo se particiona con la función partition
    partition(input, output, 0, f_size / sizeof(uint64_t) - 1, B, a, disk_access);
}


/**
 * Usamos los mismos parámetros que en sort, pero agregamos
 * -low: índice inicial del subarreglo a particionar
 * -high: índice final del subarreglo a particionar
 */
size_t ExternalQuickSort::partition(FILE* input, FILE* output, size_t low, size_t high, size_t B, int a, int& disk_access){
    // Primero, hacemos la selección de pivotes
    uint64_t* pvts = new uint64_t[a-1]; // Creamos un arreglo que contendrá los a-1 pivotes, los cuales arrojarán a subarreglos
    select_pivots(input,pvts,a,B); // Seleccionamos los pivotes y los colocamos en el arreglo
    disk_access++; // Contamos 1 I/O por hacer la selección
    std::sort(pvts, pvts + a - 1); // Ordenamos el arreglo de pivotes

    // Luego, creamos 'a' archivos temporales que corresponderán a las particiones
    FILE** tmp_files = new FILE*[a]; // Creamos un arreglo de tamañp a que contendra punteros a archivos
    for (int i = 0; i < a; i++){
        tmp_files[i] = tmpfile(); // Cada elemento del arreglo será un archivo temporal incialmente vacío
    }
}