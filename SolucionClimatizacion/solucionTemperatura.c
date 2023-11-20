#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Definición de constantes
#define TEMPERATURE_MIN 18
#define TEMPERATURE_MAX 30
#define PRESSURE_INI 5
#define PRESSURE_HIGH_MAX 25
#define PRESSURE_LOW_MIN 0.5
#define SPEED_MIN 0
#define SPEED_MAX 100
#define OFF 0
#define MIN 1
#define MED 2
#define MAX 3

//COMUN
#define AJUSTE 0.01
#define EVAPORATOR_FAN_MIN 10
#define EVAPORATOR_FAN_MAX 100
typedef enum {
    CALENTAR,
    ENFRIAR,
    REPOSO
} Accion;


//VARIABLES globales
double desiredTemperature = 0;
double temperatureSensor = 0;
double pressureLow = PRESSURE_INI;
double pressureHigh = PRESSURE_INI;
int evaporatorFanSpeed = EVAPORATOR_FAN_MIN;
Accion accionActual = REPOSO;
//Variables frio
int compressorState = OFF;
int fanCondenser = SPEED_MIN;
// Variables calor
int electricResistance = OFF;
int waterPump = SPEED_MIN;


double leer(double minima, double maxima) {
    // Semilla para la generación de números aleatorios
    srand(time(NULL));

    // Genera un valor aleatorio dentro del rango
    double valor = minima + ((double)rand() / RAND_MAX) * (maxima - minima);

    /*Aqui realmente obtendriamos el valor real de temperatura, presion etc como no tenemos estas lecturas sacamos un valor random*/
    return valor;
}

// Función para escribir datos en un archivo CSV
void escribirCSV(double tiempo, int estado, int potencia) {

    FILE *archivo = fopen("datos.csv", "r");
    if (archivo != NULL) {//Existe
        fclose(archivo);
        FILE *archivo = fopen("datos.csv", "a");
    }
    else { //NO EXISTE
        FILE *archivo = fopen("datos.csv", "w");
    }

    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }

    fprintf(archivo, "tiempo=%.2f, proposito=%d:\n"
                      "estado=%d, potencia=%d, velocidadVentilador=%d, temperatura=%.2f, presionBaja=%.2f, presionAlta=%.2f\n", 
                      tiempo, accionActual, estado, potencia, evaporatorFanSpeed, temperatureSensor, pressureLow, pressureHigh);

    fclose(archivo);

    printf("Datos escritos en el archivo datos.csv\n");
}

void escribirNuevaTrazaCSV(int num) {

    FILE *archivo = fopen("datos.csv", "r");
    if (archivo != NULL) {//Existe
        fclose(archivo);
        FILE *archivo = fopen("datos.csv", "a");
    }
    else { //NO EXISTE
        FILE *archivo = fopen("datos.csv", "w");
    }

    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }

    fprintf(archivo, "TRAZA NUMERO %d\n", num);

    fclose(archivo);
}

// Función para escribir datos en un archivo JSON
void escribirJSON(double tiempo, Accion accion, int estado, int potencia, int velocidadVentilador, double temperatura, double presionBaja, double presionAlta) {
    FILE *archivo = fopen("datos.json", "w");

    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }

    fprintf(archivo, "{ \"tiempo\": %.2f, \"accion\": %d, \"estado\": %d, \"potencia\": %d, \"velocidadVentilador\": %d, \"temperatura\": %.2f, \"presionBaja\": %.2f, \"presionAlta\": %.2f }\n",
            tiempo, accion, estado, potencia, velocidadVentilador, temperatura, presionBaja, presionAlta);

    fclose(archivo);
}

void reset(void){
    pressureLow = PRESSURE_INI;
    pressureHigh = PRESSURE_INI;
    evaporatorFanSpeed = EVAPORATOR_FAN_MIN;
    accionActual = REPOSO;
    //Variables frio
    compressorState = OFF;
    fanCondenser = SPEED_MIN;
    // Variables calor
    electricResistance = OFF;
    waterPump = SPEED_MIN;
}

int sonIguales(double num1, double num2) {
    return fabs(num1 - num2) <= AJUSTE;
}

// Función para actuar en el compresor
int control(double difTemperatura, int *potencia) {
    int state;

    //Meto logica de accionamiento inventada
    if (difTemperatura > 3.0) {
        state = MAX;
        *potencia = SPEED_MAX;
        evaporatorFanSpeed = EVAPORATOR_FAN_MAX;
    } 
    else if (difTemperatura > 1.5){
        state = MED;
        *potencia = 50;
        evaporatorFanSpeed = 50;
    } 
    else if (difTemperatura > 0){
        state = MIN;
        *potencia = 20;
        evaporatorFanSpeed = 20;
    } 
    else { //difTemperatura = 0
        state = OFF;
        *potencia = SPEED_MIN;
        evaporatorFanSpeed = EVAPORATOR_FAN_MIN;
    }

    if (state!=OFF){
        if (accionActual == ENFRIAR){
            //accionar condensador y ventilador
            if(temperatureSensor - desiredTemperature <= 2){
                temperatureSensor = leer(desiredTemperature, temperatureSensor);
            }
            else {
                temperatureSensor = leer(temperatureSensor-2, temperatureSensor);
            }
            if(PRESSURE_HIGH_MAX-pressureHigh <= 2){
                pressureHigh = leer (pressureHigh, PRESSURE_HIGH_MAX);
            }
            else {
                pressureHigh = leer (pressureHigh, pressureHigh + 2);
            }
            if(pressureLow-PRESSURE_LOW_MIN <= 0.3){
                pressureLow =  leer (PRESSURE_LOW_MIN, pressureLow);
            }
            else {
                pressureLow =  leer (pressureLow -0.3, pressureLow);
            }
            compressorState = state;
            fanCondenser = *potencia;
        } 
        else { //CALENTAR
            //accionar bomba y resistencia
            if(desiredTemperature-temperatureSensor <= 2){
                temperatureSensor = leer(temperatureSensor, desiredTemperature);
            }
            else {
                temperatureSensor = leer(temperatureSensor, temperatureSensor+2);
            }   
            electricResistance = state;
            waterPump = *potencia;
            
        }
    }

    if (sonIguales(temperatureSensor,desiredTemperature)){
        reset();
        state = OFF;
        temperatureSensor = desiredTemperature;
        *potencia = 0;
    }
        
    return state;
}

// Función principal
int main() {
 
    //variables auxiliares
    double difTemp;
    int state = OFF, time = 0, potencia = SPEED_MIN, it = 0;
    char respuesta[3];
     
     do {
        // Entrada de temperatura deseada por el conductor
        printf("Ingrese la temperatura deseada (entre 18 y 30 grados Celsius): ");
        scanf("%lf", &desiredTemperature);

        // Verificación de rango de temperatura
        while(desiredTemperature < TEMPERATURE_MIN || desiredTemperature > TEMPERATURE_MAX){
            printf("Error: La temperatura deseada debe estar entre 18 y 30 grados Celsius.\n");
            printf("Ingrese la temperatura deseada (entre 18 y 30 grados Celsius): ");
            scanf("%lf", &desiredTemperature);
        }
        
        //Leemos la temperatura aleatoria ya que no tengo datos del sensor
        if(it==0){
            temperatureSensor = leer(0,50);
        }
        it++;
        difTemp = desiredTemperature - temperatureSensor;
        escribirNuevaTrazaCSV(it);

        //Actuamos un sistema u otro en funcion del resultado
        if (difTemp < 0){
            //si la temperatura es negativa tendremos que aplicar frio
            accionActual = ENFRIAR;
            difTemp = (double)fabs(difTemp);
            printf("tiempo, accion, estado, potencia, velocidadVentilador, temperatura, presionBaja, presionAlta\n");
            // Salida de resultados (aquí se imprime a la consola)
            printf("%d, %d, %d, %d, %d, %.2f, %.2f, %.2f\n", 
                    time, accionActual, state, potencia, evaporatorFanSpeed, temperatureSensor, pressureLow, pressureHigh);
            escribirCSV (time, state, potencia);
        }
        else if(difTemp > 0){
            //si la temperatura es positiva tendremos que aplicar calor
            accionActual = CALENTAR;
            printf("tiempo, accion, estado, potencia, velocidadVentilador, temperatura, presionBaja, presionAlta\n");
            // Salida de resultados (aquí se imprime a la consola)
            printf("%d, %d, %d, %d, %d, %.2f, %.2f, %.2f\n", 
                    time, accionActual, state, potencia, evaporatorFanSpeed, temperatureSensor, pressureLow, pressureHigh);
            escribirCSV (time, state, potencia);
        }
        else {
            accionActual = REPOSO;
        }

        // Cálculos y simulación del sistema de control de clima
        while ((accionActual!=REPOSO)&&(difTemp!=0)){
            
            //Solo para trazabilidad, el sistema se para cuando la difftemperatura llega a cero
            time++;

            state = control(difTemp,&potencia);
            difTemp = (double)fabs(desiredTemperature - temperatureSensor);

            // Salida de resultados (aquí se imprime a la consola)
            printf("%d, %d, %d, %d, %d, %.2f, %.2f, %.2f\n", 
                    time, accionActual, state, potencia, evaporatorFanSpeed, temperatureSensor, pressureLow, pressureHigh);
            escribirCSV (time, state, potencia);
        } 

        if ((accionActual==REPOSO)||(difTemp==0)){
            reset();
        }
        printf("La temperatura es correcta.\n");

        printf("¿Quieres continuar? (si/no): ");
        scanf("%s", respuesta);

        // Limpiar el búfer de entrada
        while (getchar() != '\n');

    } while (strcmp(respuesta, "si") == 0); 

    system("pause");

    return 0;
}
