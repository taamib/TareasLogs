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
    size_t n_elements = f_size / sizeof(uint64_t);// Notemos que la cantidad de elementos del archivo corresponderá al tamaño total del archivo dividio en el tamaño de en bytes de cada uno.
    fseek(input, 0, SEEK_SET); // Devolvemos el cursos al inicio
    disk_access += 2; // Contamos los dos I/O (uno por cada fseek)

    // Caso base (el archivo es suficientemente pequeño para ser ordenado en memoria)
    if (f_size <= M){
        uint64_t* buff = new uint64_t[n_elements]; // Creamos un buffer para guardar los datos del archivo

        fread(buff, sizeof(uint64_t), n_elements,input); // Leemos el archivo en el buffer
        disk_access++; // Aumentamos un I/O por el fread

        // Notemos que std::sort recibe punteros al inicio y al final de un subarreglo, y ordena desde inicio a final -1
        std::sort(buff,buff + n_elements); // Ordenamos los datos en memoria

        fwrite(buff, sizeof(uint64_t), n_elements, output); // Escribimos los datos ya ordenados en el output
        disk_access++;// Aumentamos un I/O por el fwrite

        delete[] buff;// Liberamos el buffer para no tener problemas con el espacio
        return; // Finalizamos
    }
    
    // Caso recursivo (el archivo no cabe en memoria)
    // De no caber en memoria, el archivo debe ser particionado para hacer la recursión con los a subarreglos

    FILE* temp_part = tmpfile(); // Primero creamos un archivo temporal para hacer la particion

    partition(input,temp_part,0, n_elements - 1, B, a, disk_access); // Realizamos la particion y la guardamos en temp_part

    fseek(temp_part,0,SEEK_SET); // Fijamos el cursor de temp_part al inicio
    disk_access++; // Contamos un I/O por esta operación

    for (int i = 0; i < a; i++){ // Para cada partición, hacemos lo siguiente
        size_t p_size; // Consultamos el tamaño de la partición
        fread(&p_size, sizeof(size_t), 1, temp_part); // Para esto, accedemos al dato siguiente al índice en el archivo particionado
        disk_access++; // Contabilizamos el I/O

        if (p_size > 0){ // Si el arreglo i tiene elementos, hacemos lo siguiente
            
            FILE* p_file = tmpfile(); // Creamos un archivo temporal para la partición

            uint64_t* p_buff = new uint64_t[B / sizeof(uint64_t)]; // Creamos un buffer para copiar los datos de la partición en el archivo temporal
            size_t p_remaining = p_size; // Contabilizamos cuantos elementos faltan por ser leidos, inicialmente todos lo de la partición

            while (p_remaining > 0){ // Mientras nos queden elementos por leer, hacemos lo siguiente
                size_t to_read = std::min(B / sizeof(uint64_t), p_remaining); // Elementos que vamos a leer con el buffer, puede variar en el ultimo tramo
                fread(p_buff, sizeof(uint64_t), to_read, temp_part); // Leemos la cantidad correspondiente de elementos en el buffer desde el archivo particionado
                fwrite(p_buff,sizeof(uint64_t),to_read,p_file); // Escribimos dichos archivos desde el buffer al archivo temporal creado
                disk_access += 2; // Contabilizamos los I/Os
                p_remaining -= to_read; // Indicamos que la cantidad de elementos que faltan por leer de la partición se redujo
            }

            delete[] p_buff; // Cuando leemos todos los elementos de la partición, liberamos el buffer
            
            fseek(p_file,0,SEEK_SET); // Colocamos el cursor del archivo temporal al principio

            sort(p_file, output, B, M, a, disk_access); // Llamamos recursivamente a sort para ordenar la partición

            fclose(p_file); // Podemos liberar el archivo temporal de la partición
        }
    }

    fclose(temp_part); // Al terminar la recursión, podemos liberar el archivo en donde hicimos la partición
}


/**
 * 'partition' nos entrega un archivo output particionado en a subarreglos a partir de un archivo input
 * El output entregado tiene, para cada particion y de forma consecutiva:
 * - El índice de la partición 'i'
 * - El tamaño de la partición 'size'
 * - Los elementos de la partición en los siguientes 'size' espacios
 * Usamos los mismos parámetros que en sort, pero agregamos
 * -low: índice inicial del subarreglo a particionar
 * -high: índice final del subarreglo a particionar
 */
void ExternalQuickSort::partition(FILE* input, FILE* output, size_t low, size_t high, size_t B, int a, int& disk_access){
    
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

    
    // Luego, distribuimos los elementos dentro de las particiones
    uint64_t* buffer = new uint64_t[B/sizeof(uint64_t)]; // Creamos un buffer que contenga la cantidad de elementos correspondientes a un tamaño B

    fseek(input, low * sizeof(uint64_t), SEEK_SET); // Posicionamos el cursor en el inicio del rango a procesar (marcado por low)
    disk_access++; // Contabilizamos el I/O correspondiente a esto

    size_t remaining = high - low + 1; // Calculamos el total que nos falta procesar, incialmente serán todos
    while (remaining > 0){ // Mientras queden elementos por distribuir repetimos lo siguiente
        
        size_t to_read = std::min(B / sizeof(uint64_t), remaining); // Consultar cuantos elementos se deben leer en el bloque (en el último bloque puede variar)

        fread(buffer, sizeof(uint64_t), to_read, input); // Leemos dicha cantidad de datos del input en el buffer que creamos
        disk_access++; // Contabilizamos el I/O correspondiente a esto

        for(size_t i = 0; i < to_read; i++){ // Luego, para cada elemento que leimos en el buffer
            uint64_t val = buffer[i]; // Accedemos al elemento
            int j = 0; // j será el indice del archivo de tmp_files en donde deberemos guardar val

            while (j < a - 1 && val > pvts[j]){ // Buscamos el archivo en el que tenemos que guardar el elemento según los pivotes
                j++;
            }

            fwrite(&val, sizeof(uint64_t), 1, tmp_files[j]); // Una vez encontrado, escribimos el elemento en su archivo correspondiente
            disk_access++; // Contabilizamos el I/O correspondiente
        }

        remaining -= to_read; // Y una vez distribuidos todos los elementos del bloque, indicamos que quedan menos elementos por revisar

    }

   delete[] buffer; // Cuando ya no tenemos elementos por distribuir, liberamos el buffer


   // Luego, escribimos los metadatos y datos en el archivo de salida 
   // Los metadatos serán:
   // - Identificador de partición (i)
   // - Tamaño de la partición

   // Para cada partición, hacemos lo siguiente
   for (int i = 0; i < a; i++){

    fseek(tmp_files[i],0,SEEK_END); // Llevamos el cursor al final
    size_t psize = (ftell(tmp_files[i]) / sizeof(uint64_t)); // Obtenemos el tamaño de la particion (en cantidad de objetos)
    fseek(tmp_files[i],0,SEEK_SET); // Devolvemos el cursor al inicio
    disk_access += 2; // Contabilizamos los I/Os

    // Escribimos los metadatos:
    fwrite(&i, sizeof(int), 1, output); // Identificador de partición
    fwrite(&psize, sizeof(size_t), 1, output); // Tamaño de la partición
    disk_access += 2; // Contabilizamos los I/Os de hacer esto

    // Copiamos los elementos de la partición en el output si es que hay
    if (psize > 0){
        uint64_t* p_buffer = new uint64_t[B/sizeof(uint64_t)]; // Creamos un buffer para una cantidad de elementos correspondiente a tamaño B, que nos servirá para hacer la copia de los elementos
        size_t p_remaining = psize; // Consultamos cuantos elementos nos falta copiar, inicialmente todos los de la partición

        while (p_remaining > 0){ // Si nos quedan elementos por revisar, hacemos lo siguiente
            size_t to_copy = std::min(B / sizeof(uint64_t), p_remaining); // Calculamos mínimo entre la cantidad de elementos que caben y la cantidad de elementos que quedan (misma lógica que con to_read)
            fread(p_buffer, sizeof(uint64_t), to_copy, tmp_files[i]); // Leemos la partición en el buffer
            fwrite(p_buffer, sizeof(uint64_t), to_copy, output); // Escribimos los datos leidos en el output de retorno
            disk_access += 2; // Contabilizamos los I/Os de hacer esto
            p_remaining -= to_copy; // Restamos el total de elementos que copiamos del total que nos quedan
        }

        delete[] p_buffer; // Al copiar todos los elementos de la partición, liberamos el buffer
    }

    fclose(tmp_files[i]); // Cuando copiamos todos los elementos de una partición, podemos cerrar el archivo temporal que la contenía
   }


   // Con este proceso ya tenemos lo que qeriamos, por lo que podemos eliminar los buffer de los pivotes y de los archivos temporales
   delete[] pvts;
   delete[] tmp_files;
}   