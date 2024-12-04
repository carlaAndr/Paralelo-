#include <mpi.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>

using namespace std::chrono;
using namespace std;

map<string, int> build_vocab_map(const string& vocab_file, vector<string>& vocab_list) {
    ifstream infile(vocab_file);
    if (!infile) {
        cerr << "Error al abrir el archivo de vocabulario " << vocab_file << endl;
        return {};
    }
    map<string, int> vocab_map;
    string word;
    int index = 0;
    while (infile >> word) {
        if (vocab_map.find(word) == vocab_map.end()) {
            vocab_map[word] = index++;
            vocab_list.push_back(word);
        }
    }
    infile.close();
    return vocab_map;
}

vector<string> csvAarray(string archivo) {
    ifstream inputFile(archivo);
    if (!inputFile) { // Validar que el archivo se pueda abrir
        cerr << "Error: No se pudo abrir el archivo " << archivo << "\n";
        return {}; // Devolver un vector vacío en caso de error
    }
    vector<string> arreglo; // Arreglo para almacenar las líneas como strings
    string linea;

    // Leer el archivo línea por línea
      while (getline(inputFile, linea)) {
        stringstream ss(linea); // Convertir la línea en un flujo para dividirla
        string palabra;

        // Dividir la línea en palabras separadas por comas
        while (getline(ss, palabra, ',')) {
            arreglo.push_back(palabra); // Agregar cada palabra al arreglo
        }
    }
    
    inputFile.close();
    return arreglo; // Devolver el arreglo
}



void guardarMatrizConteosCSV(const vector<vector<int>>& matrizConteos, const vector<string>& vocabulario, const vector<string>& titulos, const string& archivoSalida) {
    ofstream outFile(archivoSalida);

    if (!outFile) {
        cerr << "Error: No se pudo abrir el archivo " << archivoSalida << " para escribir.\n";
        return;
    }

    // Escribir encabezado (nombre de los libros)
    outFile << "Palabra";
    for (const auto& titulo : titulos) {
        outFile << "," << titulo;
    }
    outFile << "\n";

    // Escribir conteos por palabra
    int vocab_size = vocabulario.size();
    int num_libros = titulos.size();

    for (int i = 0; i < vocab_size; i++) {
        outFile << vocabulario[i];
        for (int j = 0; j < num_libros; j++) {
            outFile << "," << matrizConteos[j][i];
        }
        outFile << "\n";
    }

    outFile.close();
    cout << "Resultados guardados en " << archivoSalida << "\n";
}

int main(int argc, char* argv[]) {

    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Verificar que al menos se haya proporcionado el nombre del vocabulario
    if (argc < 2) {
        if (rank == 0) {
            cerr << "Uso: " << argv[0] << " <archivo_vocabulario> [libro1 libro2 ... libroN]\n";
        }
        MPI_Finalize();
        return 1;
    }

    string archivoVocabulario = argv[1];

    // Lista de libros por defecto
    vector<string> librosPredeterminados = {
        "dickens_a_christmas_carol.txt",
        "dickens_a_tale_of_two_cities.txt",
        "dickens_oliver_twist.txt",
        "shakespeare_hamlet.txt",
        "shakespeare_romeo_juliet.txt",
        "shakespeare_the_merchant_of_venice.txt"
    };

    vector<string> libros;
    if (argc >= 3) {
        // Si se pasan nombres de libros como argumentos
        libros.assign(argv + 2, argv + argc);
    } else {
        // Usar la lista predeterminada si no se proporcionan argumentos
        libros = librosPredeterminados;
    }
    if (rank == 0) {
        cout << "Archivo de vocabulario: " << archivoVocabulario << "\n";
        cout << "Libros a procesar:\n";
        for (const auto& libro : libros) {
            cout << "- " << libro << "\n";
        }
    }

    int numLibros = libros.size();

    // Todos los procesos leen el vocabulario
    vector<string> vocab_list;
    map<string, int> vocab_map = build_vocab_map(archivoVocabulario, vocab_list);
    int vocab_size = vocab_list.size();

    cout << "Proceso " << rank << ": vocabulario leido con " << vocab_size << " palabras.\n";

    // Distribuir libros entre procesos
    int libros_per_proc = numLibros / size;
    int remainder = numLibros % size;
    int start = rank * libros_per_proc + min(rank, remainder);
    int end = start + libros_per_proc + (rank < remainder ? 1 : 0);
    int local_num_books = end - start;

    cout << "Proceso " << rank << " procesara libros de " << start << " a " << end - 1 << "\n";

    auto inicioParalelo = high_resolution_clock::now();

    // Cada proceso cuenta las palabras en sus libros asignados
    vector<vector<int>> counts_per_book(local_num_books, vector<int>(vocab_size, 0));

    for (int idx = 0; idx < local_num_books; idx++) {
        int i = start + idx;
        string titulo = libros[i];
        vector<string> libro = csvAarray(titulo);
        // Añade esta impresión
        cout << "Proceso " << rank << " leyendo el libro: " << titulo << " con " << libro.size() << " palabras.\n";



        vector<int>& book_counts = counts_per_book[idx];

        for (const string& palabra : libro) {
            auto it = vocab_map.find(palabra);
            if (it != vocab_map.end()) {
                int index = it->second;
                book_counts[index]++;
            }
        }
    }   


    // Preparar datos para enviar al proceso raíz
    vector<int> local_counts_flat(local_num_books * vocab_size);
    for (int b = 0; b < local_num_books; b++) {
        for (int v = 0; v < vocab_size; v++) {
            local_counts_flat[b * vocab_size + v] = counts_per_book[b][v];
        }
    }

    // Reunir el número de libros procesados por cada proceso
    vector<int> recv_counts(size);
    MPI_Gather(&local_num_books, 1, MPI_INT, recv_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Reunir los conteos de todos los procesos en el proceso raíz
    vector<int> recv_counts_flat;
    vector<int> displs;
    int total_books = 0;

    if (rank == 0) {
        total_books = 0;
        displs.push_back(0);
        for (int i = 0; i < size; i++) {
            total_books += recv_counts[i];
            if (i > 0) {
                displs.push_back(displs[i - 1] + recv_counts[i - 1] * vocab_size);
            }
        }
        recv_counts_flat.resize(size);
        for (int i = 0; i < size; i++) {
            recv_counts_flat[i] = recv_counts[i] * vocab_size;
        }
    }

    vector<int> all_counts_flat;
    if (rank == 0) {
        all_counts_flat.resize(total_books * vocab_size);
    }

    MPI_Gatherv(local_counts_flat.data(), local_counts_flat.size(), MPI_INT,
                all_counts_flat.data(), recv_counts_flat.data(), displs.data(), MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {
        // Reconstruir la matriz de conteos por libro
        vector<vector<int>> total_counts_per_book(total_books, vector<int>(vocab_size, 0));
        for (int b = 0; b < total_books; b++) {
            for (int v = 0; v < vocab_size; v++) {
                total_counts_per_book[b][v] = all_counts_flat[b * vocab_size + v];
            }
        }

        // Ordenar los títulos de los libros en el orden procesado
        vector<string> titulosOrdenados;
        for (int i = 0; i < size; i++) {
            int proc_start = i * libros_per_proc + min(i, remainder);
            int proc_end = proc_start + libros_per_proc + (i < remainder ? 1 : 0);
            for (int j = proc_start; j < proc_end; j++) {
                titulosOrdenados.push_back(libros[j]);
            }
        }

        // Guardar los resultados en un archivo CSV
        guardarMatrizConteosCSV(total_counts_per_book, vocab_list, titulosOrdenados, "resultados_paralelo.csv");

        auto finParalelo = high_resolution_clock::now();
        auto duracionParalelo = duration_cast<milliseconds>(finParalelo - inicioParalelo).count();
        cout << "Tiempo total (paralelo): " << duracionParalelo << " milisegundos\n";
    }

    MPI_Finalize();
    return 0;
}
