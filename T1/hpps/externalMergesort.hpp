#include <string>
#include <vector>
#include <cstdint>

class ExternalMergeSort {
public:
    std::vector<std::string> dividir_arr(const std::string& input_path, size_t N, int& disk_access);
    std::string ordenar_subarr(const std::string& input_path, size_t N, int& disk_access);
    void mergear_archivos(const std::vector<std::string>& archivos, const std::string& output, int& disk_access);
    void mergesort_externo(const std::string& input_path, const std::string& output_path, size_t N, int& disk_access);

private:
    const size_t M = 50 * 1024 * 1024; // 50MB
    const size_t B = 4096; // Tamaño de bloque (4KB)
    const size_t ELEMENTS_PER_BLOCK = B / sizeof(uint64_t);
    const int aridad = 3;
};