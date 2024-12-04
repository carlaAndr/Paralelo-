#include <omp.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cmath>
#include <chrono>

#include <stdexcept>

using namespace std;

void noise_detection_serial(float** points, float epsilon, int min_samples, long long int size) {
    cout << "Paso 1: Identificar puntos core y outliers\n";
    // Paso 1: Identificar puntos core y outliers
    for (long long int i = 0; i < size; i++) {
        int neighbor_count = 0;
        for (long long int j = 0; j < size; j++) {
            if (i != j && sqrt(pow(points[i][0] - points[j][0], 2) + pow(points[i][1] - points[j][1], 2)) <= epsilon) {
                neighbor_count++;
            }
        }
        // Clasificación como core o ruido
        if (neighbor_count >= min_samples) {
            points[i][2] = 1; // Es un punto core
        } else {
            points[i][2] = 0; // Es un outlier
        }
    }

    // Paso 2: Identificar puntos epsilon-alcanzables (core de segundo grado)
    cout << "Paso 2: Determinar puntos epsilon-alcanzables\n";
    for (long long int i = 0; i < size; i++) {
        if (points[i][2] == 0) {  // Si es un outlier
            for (long long int j = 0; j < size; j++) {
                if (points[j][2] == 1 && sqrt(pow(points[i][0] - points[j][0], 2) + pow(points[i][1] - points[j][1], 2)) <= epsilon) {
                    // Si está dentro del epsilon de un punto core
                    points[i][2] = 2; // Es un punto alcanzable (core de segundo grado)
                    break; // Ya encontramos que es alcanzable, no necesitamos seguir
                }
            }
        }
    }

    cout << "Complete\n";
}
void writeCSV(const std::string& filename, const PointCloud& cloud, const std::vector<int>& clusters) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open the file for writing: " + filename);
    }

    for (size_t i = 0; i < cloud.size(); ++i) {
        const auto& point = cloud[i];
        int clusterId = clusters[i];
        file << point.x << "," << point.y << "," << clusterId << "\n";
    }

    if (file.fail()) {
        throw std::runtime_error("An error occurred while writing to the file: " + filename);
    }

    file.close();
}
void noise_detection_parallel(float** points, float epsilon, int min_samples, long long int size) {
    cout << "Paso 1: Identificar puntos core y outliers\n";

    long long int i = 0;
    long long int j = 0;

    // Paso 1: Identificar puntos core y outliers usando paralelización
    #pragma omp parallel for shared(points, size) private(i, j) // Paralelizamos el bucle externo
    for (long long int i = 0; i < size; i++) {
        int neighbor_count = 0;
        
        #pragma omp parallel for reduction(+:neighbor_count) // Reducimos el conteo de vecinos de forma segura
        for (long long int j = 0; j < size; j++) {
            if (i != j && sqrt(pow(points[i][0] - points[j][0], 2) + pow(points[i][1] - points[j][1], 2)) <= epsilon) {
                neighbor_count++;
            }
        }
        // Clasificación como core o ruido
        if (neighbor_count >= min_samples) {
            points[i][2] = 1; // Es un punto core
        } else {
            points[i][2] = 0; // Es un outlier
        }
    }

    // Paso 2: Identificar puntos epsilon-alcanzables (core de segundo grado)
    cout << "Paso 2: Determinar puntos epsilon-alcanzables\n";
    #pragma omp parallel for shared(points, size) private(i, j) // Paralelizamos la búsqueda de puntos alcanzables
    for (long long int i = 0; i < size; i++) {
        if (points[i][2] == 0) {  // Si es un outlier
            for (long long int j = 0; j < size; j++) {
                if (points[j][2] == 1 && sqrt(pow(points[i][0] - points[j][0], 2) + pow(points[i][1] - points[j][1], 2)) <= epsilon) {
                    // Si está dentro del epsilon de un punto core
                    points[i][2] = 2; // Es un punto alcanzable (core de segundo grado)
                    break; // Ya encontramos que es alcanzable, no necesitamos seguir
                }
            }
        }
    }
    

    cout << "Completo\n";
}

/*void load_CSV(string file_name, float** points, long long int size) {
    ifstream in(file_name);
    if (!in) {
        cerr << "Couldn't read file: " << file_name << "\n";
    }
    long long int point_number = 0; 
    while (!in.eof() && (point_number < size)) {
        char* line = new char[12];
        streamsize row_size = 12;
        in.read(line, row_size);
        string row = line;
        points[point_number][0] = stof(row.substr(0, 5));
        points[point_number][1] = stof(row.substr(6, 5));
        point_number++;
    }
}*/
void load_CSV(string file_name, float** points, long long int size) {
    ifstream in(file_name);
    if (!in) {
        cerr << "No se pudo leer el archivo: " << file_name << "\n";
        return;
    }

    string line;
    long long int point_number = 0;
    
    // Leer cada línea del archivo
    while (getline(in, line) && point_number < size) {
        try {
            // Encuentra la posición de la coma o el espacio que separa los valores
            size_t comma_pos = line.find(',');
            if (comma_pos == string::npos) {
                cerr << "Formato incorrecto en la línea " << point_number << "\n";
                continue;
            }

            // Convierte los valores antes y después de la coma a float
            points[point_number][0] = stof(line.substr(0, comma_pos));
            points[point_number][1] = stof(line.substr(comma_pos + 1));

            point_number++;
        } catch (const std::invalid_argument& e) {
            cerr << "Error al convertir valores en la fila " << point_number << ": " << e.what() << endl;
        }
    }
}

void save_to_CSV(string file_name, float** points, long long int size) {
    fstream fout;
    fout.open(file_name, ios::out);
    for (long long int i = 0; i < size; i++) {
        fout << points[i][0] << ","
             << points[i][1] << ","
             << points[i][2] << "\n";
    }
}
//Este es el main principal
int main(int argc, char** argv) {
    // Parámetros del experimento
    const float epsilon = 0.03;
    const int min_samples = 10;
    
    // Tamaños de los datasets para el experimento
    long long int sizes[] = {20000, 40000, 80000, 120000, 140000, 160000, 180000, 200000};
    int num_hilos[] = {1, omp_get_max_threads()/2, omp_get_max_threads(), omp_get_max_threads()*2};  // Diferentes configuraciones de hilos
    const int iteraciones = 10;
    // Crear archivos CSV para guardar resultados
    ofstream serial_file("serial.csv");
    ofstream paralelo_file("paralelo.csv");

    // Encabezados de los archivos CSV
    serial_file << "Datos,Iteración,Tiempo\n";
    paralelo_file << "Datos,Hilos,Iteración,Tiempo\n";

    // Experimento para cada tamaño de datos y configuración de hilos
    for (int s = 0; s < 8; s++) { // Cada tamaño de datos
        long long int size = sizes[s]; // Tamaño del dataset
        const string input_file_name = to_string(size) + "_data.csv";
        const string output_file_name = to_string(size) + "_results.csv";

        for (int it = 0; it < iteraciones; it++) { // Repetir 10 veces cada experimento

            // *Ejecución Serial*
            float** points = new float*[size];
            for (long long int i = 0; i < size; i++) {
                points[i] = new float[3]{0.0, 0.0, 0.0};
            }
            load_CSV(input_file_name, points, size);

            auto start_serial = chrono::high_resolution_clock::now();
            noise_detection_serial(points, epsilon, min_samples, size);
            auto end_serial = chrono::high_resolution_clock::now();
            auto duration_serial = chrono::duration_cast<chrono::microseconds>(end_serial - start_serial);
            double serial_time = static_cast<double>(duration_serial.count()) / 1000000;

            cout << "Tiempo serial para " << size << " puntos: " << serial_time << " segundos\n";
            serial_file << size << "," << it + 1 << "," << serial_time << "\n";

            for (long long int i = 0; i < size; i++) {
                delete[] points[i];
            }
            delete[] points;

            // *Ejecución Paralela para cada configuración de hilos*
            for (int h = 0; h < 4; h++) {
                omp_set_num_threads(num_hilos[h]);

                points = new float*[size];
                for (long long int i = 0; i < size; i++) {
                    points[i] = new float[3]{0.0, 0.0, 0.0};
                }
                load_CSV(input_file_name, points, size);

                double start_parallel = omp_get_wtime();
                noise_detection_parallel(points, epsilon, min_samples, size);
                double end_parallel = omp_get_wtime();
                double parallel_time = end_parallel - start_parallel;

                cout << "Tiempo paralelo para " << size << " puntos y " << num_hilos[h] << " hilos: " << parallel_time << " segundos\n";
                paralelo_file << size << "," << num_hilos[h] << "," << it + 1 << "," << parallel_time << "\n";

                save_to_CSV(output_file_name, points, size);

                for (long long int i = 0; i < size; i++) {
                    delete[] points[i];
                }
                delete[] points;
            }
        }
    }
    
    // Cerrar los archivos CSV
    serial_file.close();
    paralelo_file.close();
    return 0;
   
}


