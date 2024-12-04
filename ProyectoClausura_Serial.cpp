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

map<string, int> csvDiccionario(string archivo) {
    ifstream inputFile(archivo);
    if (!inputFile) {
        cerr << "Error: No se pudo abrir el archivo " << archivo << "\n";
        return {};
    }

    map<string, int> dictionary; // Diccionario para almacenar palabras como clave con valor 0
    string linea, palabra;

    // Leer el archivo línea por línea
    while (getline(inputFile, linea)) {
        stringstream ss(linea); // Procesar la línea como un flujo de palabras

        while (ss >> palabra) { // Leer palabra por palabra
            if (dictionary.find(palabra) == dictionary.end()) {
                dictionary[palabra] = 0; // Insertar la palabra con valor inicial 0
            }
        }
    }

    inputFile.close();
    return dictionary;
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



void guardarMatrizConteosCSV(const vector<map<string, int>>& matrizConteos, const map<string, int>& vocabulario, int numLibros, const string& archivoSalida) {
    ofstream outFile(archivoSalida);

    if (!outFile) {
        cerr << "Error: No se pudo abrir el archivo " << archivoSalida << " para escribir.\n";
        return;
    }

    // Escribir encabezado (nombre de los libros)
    outFile << "Palabra";
    for (int i = 0; i < numLibros; i++) {
        outFile << ",Libro" << (i + 1);
    }
    outFile << "\n";

    // Escribir conteos por palabra
    for (const auto& [palabra, _] : vocabulario) {
        outFile << palabra;
        for (int i = 0; i < numLibros; i++) {
            outFile << "," << matrizConteos[i].at(palabra);
        }
        outFile << "\n";
    }

    outFile.close();
    cout << "Resultados guardados en " << archivoSalida << "\n";
}

void contarPalabras2(string* titulos, map<string, int>& vocabulario, int num) { 
    cout << "Inicia conteo\n";

    // Crear una matriz (vector de mapas) para almacenar los conteos por libro
    vector<map<string, int>> matrizConteos(num, vocabulario); // Inicializa con el vocabulario base

    for (int i = 0; i < num; i++) {
        string titulo = titulos[i];
        vector<string> libro = csvAarray(titulo); // Lee el archivo como vector de palabras

        // Contar palabras en el libro actual
        for (const string& palabra : libro) {
            if (matrizConteos[i].find(palabra) != matrizConteos[i].end()) {
                matrizConteos[i][palabra] += 1;
            }
        }
    }
    guardarMatrizConteosCSV(matrizConteos, vocabulario, num, "resultados.csv");

}

//Main
int main(int argc, char* argv[]) {

    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <archivo_vocabulario> [libro1 libro2 ... libroN]\n";
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
            // Saltamos argv[0] que es el nombre del programa y argv[1] diccionario
            libros.assign(argv + 2, argv + argc);
        } else {
            // Usar la lista predeterminada si no se proporcionan argumentos
            libros = librosPredeterminados;
        }

        int numLibros = libros.size();
        map<string, int> diccionario = csvDiccionario(archivoVocabulario);

        // Medir tiempo y ejecutar la versión serial
        auto inicioSerial = high_resolution_clock::now();
        contarPalabras2(libros.data(), diccionario, numLibros);
        auto finSerial = high_resolution_clock::now();

        // Calcular el tiempo transcurrido
        auto duracionSerial = duration_cast<milliseconds>(finSerial - inicioSerial).count();
        cout << "Tiempo transcurrido en ejecucion serial: " << duracionSerial << " milisegundos\n";

    return 0;
}


