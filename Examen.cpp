#include <omp.h>
#include <iostream>
#include <time.h>
#include <chrono>
using namespace std;

//Función serial
void conteoBMSerial(int* clientes,int size){
    int cBuenos=0;
    int cMalos=0;
    auto start_serial = std::chrono::high_resolution_clock::now(); // Inicia la medición de tiempo
    for (int i=0; i<size; i++){
        if(clientes[i]>50){
            //Clientes buenos
            cBuenos=cBuenos+1;
        }
        else{
            //<=50
            cMalos=cMalos+1;
        }
    }    
    auto end_serial = std::chrono::high_resolution_clock::now(); // Termina la medición de tiempo
    cout<<"Serial";
    cout<<"Candidatos potencialmente buenos"<<cBuenos;
    cout<<"Candidatos potencialemente malos"<<cMalos;
    
    auto duration = std::chrono::duration_cast<chrono::microseconds>(end_serial - start_serial); // Convierte la duración a microsegundos
    double serial_time = static_cast<double>(duration.count()) / 1000000; // Convierte el tiempo a segundos
    cout << "Tiempo de ejecución serial: " << serial_time << " segundos" << endl;
}

//Función paralela
void conteoBMParalelo(int* clientes,int size){
    int cBuenos=0;
    int cMalos=0;
    //Creo dos hilos
    omp_set_num_threads(2);
    //Variables para medir el tiempo
    double inicio;
    double fin;
    inicio=omp_get_wtime();
    //For reduction genera la suma de los buenos y de los malos
    #pragma omp parallel for reduction(+:cBuenos, cMalos)
    for (int i = 0; i < size; i++) {
        if (clientes[i] > 50) {
            cBuenos++;
        }
        else {
            cMalos++;
        }
    }
    fin=omp_get_wtime();
    cout<<"Paralela";
    cout<<"Candidatos potencialmente buenos"<<cBuenos;
    cout<<"Candidatos potencialmente malos"<<cMalos;
    cout << "Tiempo de ejecución paralelo: " << (fin - inicio) << " segundos" << endl;
}

int main(){
    long long int numero_clientes=100000000;
    //Crea los clientes
    int* scores_crediticios{new int[numero_clientes]{}};
    //Cambia semilla 
    srand(time(NULL));
    //Crea los scores
    for (long long int i=0; i<numero_clientes;i++)
        scores_crediticios[i]=rand()%100+1;

    //Manda a llamar las funciones 
    conteoBMSerial(scores_crediticios,numero_clientes);
    conteoBMParalelo(scores_crediticios, numero_clientes);

    delete [] scores_crediticios;
}