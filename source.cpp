#include <iostream> //Entrada/salida de consola
#include <fstream>  
#include <queue>    //Cola FIFO (queue)
#include <deque>    
#include <set>  //Conjuntos ordenados
#include <string>   //Uso de strings para validar varios caracteres
#include <unordered_map>
#include <vector>
#include <sys/types.h>   // Para pid_t
#include <unistd.h>      // Para fork(), exec()
#include <signal.h>      // Para kill() y señales (SIGSTOP, SIGCONT, etc)
#include <sys/wait.h>    // Para wait() 

using namespace std;

// ===== SISTEMA DE LOGS =====
void logAction(const string& action) {
    ofstream logFile("logs.txt", ios::app);
    if (logFile.is_open()) {
        logFile << action << endl;
        logFile.close();
    }
}

// ========== ESTRUCTURA DE USUARIO ==========
struct Usuario {
    string nombre;
    string tipo; // "root" o "user"
};

// ========== ESTRUCTURA DE PROCESOS ==========
//Nombre del programa,Cantidad de ciclos que se necesitan ejecutarse,En que momento llega al sistema
struct Program {
    string programa;
    int rafagas = 0;
    int llegada = 0;

    Program() = default;

    Program(string p, int r, int l) : programa(p), rafagas(r), llegada(l) {}
};

ostream& operator<<(ostream& os, const Program& p) {
    os << "Programa: " << p.programa << ", Rafagas: " << p.rafagas << ", Llegada: " << p.llegada;
    return os;
}

struct CompareByPriority {
    bool operator()(const Program& a, const Program& b) const {
        return a.llegada < b.llegada;
    }
};

Program PrgCreate() {
    Program temp;
    cout << "Nombre del programa: ";
    cin >> temp.programa;
    cout << "Numero de rafagas necesarias: ";
    cin >> temp.rafagas;
    cout << "Tiempo de llegada: ";
    cin >> temp.llegada;
    logAction("Programa creado: " + temp.programa);
    return temp;
}

void FIFO() {
    queue<Program> programQueue;
    int input = 1;

    do {
        programQueue.push(PrgCreate());
        cout << "¿Deseas agregar otro programa? (1=Sí, 0=No): ";
        cin >> input;
    } while (input != 0);

    set<Program, CompareByPriority> sortedSet;
    while (!programQueue.empty()) {
        sortedSet.insert(programQueue.front());
        programQueue.pop();
    }

    for (const Program& p : sortedSet) {
        programQueue.push(p);
    }

    queue<Program> current;
    int counter = 0;
    bool completed = false;

    while (!completed) {
        if (!programQueue.empty() && programQueue.front().llegada == counter) {
            current.push(programQueue.front());
            programQueue.pop();
        }

        if (!current.empty()) {
            current.front().rafagas--;
            if (current.front().rafagas == 0) {
                cout << "Finalizado: " << current.front().programa << endl;
                logAction("Finalizado: " + current.front().programa);
                current.pop();
            }
        }

        if (programQueue.empty() && current.empty())
            completed = true;

        counter++;
    }
}

void RR() {
    queue<Program> programQueue;
    int input = 1;
    const int quantum = 2;

    do {
        programQueue.push(PrgCreate());
        cout << "¿Agregar otro programa? (1=Si, 0=No): ";
        cin >> input;
    } while (input != 0);

    set<Program, CompareByPriority> sortedSet;
    while (!programQueue.empty()) {
        sortedSet.insert(programQueue.front());
        programQueue.pop();
    }

    for (const Program& p : sortedSet) {
        programQueue.push(p);
    }

    deque<Program> readyQueue;
    int counter = 0;
    bool completed = false;

    while (!completed) {
        while (!programQueue.empty() && programQueue.front().llegada <= counter) {
            readyQueue.push_back(programQueue.front());
            programQueue.pop();
        }

        if (!readyQueue.empty()) {
            Program current = readyQueue.front();
            readyQueue.pop_front();

            int executed = min(quantum, current.rafagas);
            for (int i = 0; i < executed; ++i) {
                current.rafagas--;
                counter++;

                while (!programQueue.empty() && programQueue.front().llegada <= counter) {
                    readyQueue.push_back(programQueue.front());
                    programQueue.pop();
                }
            }

            if (current.rafagas > 0) {
                readyQueue.push_back(current);
            }
            else {
                cout << "Finalizado: " << current.programa << " en tiempo " << counter << endl;
                logAction("Finalizado: " + current.programa + " en tiempo " + to_string(counter));
            }
        }
        else {
            counter++;
        }

        if (programQueue.empty() && readyQueue.empty())
            completed = true;
    }
}

class Archivo {
private:
    string nombre;
    string propietario;
    string permisos;

public:
    Archivo(string _nombre, string _propietario, string _permisos)
        : nombre(_nombre), propietario(_propietario), permisos(_permisos) {}

    void crear() {
        ofstream archivo(nombre);
        if (archivo.is_open()) {
            archivo << "META:" << propietario << ":" << permisos << "\n";
            archivo.close();
            cout << "Archivo creado: " << nombre << endl;
            logAction("Archivo creado: " + nombre);
        }
    }

    void escribir(string usuario, string contenido) {
        if (usuario == propietario || permisos.find('w') != string::npos || usuario == "root") {
            ofstream archivo(nombre, ios::app);
            archivo << usuario << ": " << contenido << "\n";
            archivo.close();
            cout << "Escritura exitosa.\n";
            logAction("Archivo escrito por " + usuario + ": " + nombre);
        } else {
            cout << "Permiso denegado para escribir en el archivo.\n";
        }
    }

    void leer(string usuario) {
        if (usuario == propietario || permisos.find('r') != string::npos || usuario == "root") {
            ifstream archivo(nombre);
            string linea;
            while (getline(archivo, linea)) {
                cout << linea << endl;
            }
            archivo.close();
            logAction("Archivo leído por " + usuario + ": " + nombre);
        } else {
            cout << "Permiso denegado para leer el archivo.\n";
        }
    }

    void cerrar() {
        cout << "Archivo cerrado (simulado).\n";
        logAction("Archivo cerrado: " + nombre);
    }
};

unordered_map<string, Archivo*> sistemaArchivos;

void crearArchivo(string usuario) {
    string nombre, permisos;
    cout << "Nombre del archivo: ";
    cin >> nombre;
    cout << "Permisos (r, w, rw): ";
    cin >> permisos;

    Archivo* nuevo = new Archivo(nombre, usuario, permisos);
    nuevo->crear();
    sistemaArchivos[nombre] = nuevo;
}

void menuArchivos(string usuario) {
    int opcion;
    string nombre, contenido;

    do {
        cout << "\n--- MENÚ ARCHIVOS ---\n";
        cout << "1. Crear archivo\n2. Leer archivo\n3. Escribir archivo\n4. Cerrar archivo\n5. Volver\n";
        cin >> opcion;

        switch (opcion) {
        case 1:
            crearArchivo(usuario);
            break;
        case 2:
            cout << "Nombre del archivo a leer: ";
            cin >> nombre;
            if (sistemaArchivos.count(nombre)) {
                sistemaArchivos[nombre]->leer(usuario);
            } else {
                cout << "Archivo no existe.\n";
            }
            break;
        case 3:
            cout << "Nombre del archivo a escribir: ";
            cin >> nombre;
            cin.ignore();
            cout << "Contenido: ";
            getline(cin, contenido);
            if (sistemaArchivos.count(nombre)) {
                sistemaArchivos[nombre]->escribir(usuario, contenido);
            } else {
                cout << "Archivo no existe.\n";
            }
            break;
        case 4:
            cout << "Nombre del archivo a cerrar: ";
            cin >> nombre;
            if (sistemaArchivos.count(nombre)) {
                sistemaArchivos[nombre]->cerrar();
            } else {
                cout << "Archivo no existe.\n";
            }
            break;
        }
    } while (opcion != 5);
}

// pproceso real
struct Proceso
{
    pid_t pid;
    string nombre;
    string estado;
};

vector<Proceso> procesos;

void crearProceso()
{
    string nombre;
    cout << "Nombre simbólico del proceso: ";
    cin >> nombre;

    pid_t pid = fork();

    if (pid == 0) {
        cout << "Soy el hijo con PID: " << getpid() << endl;
        pause();
        exit(0);
    }
    else if (pid > 0)
    {
        Proceso nuevo{ pid, nombre, "suspended" };
        procesos.push_back(nuevo);
        cout << "Proceso creado con PID: " << pid << " y suspendido." << endl;
        kill(pid, SIGSTOP);
        logAction("Proceso creado y suspendido: " + nombre + " PID=" + to_string(pid));
    }
    else
    {
        cerr << "Error al crear el proceso." << endl;
    }
}

void ejecutarProceso()
{
    int index;
    cout << "Selecciona el índice del proceso a ejecutar: ";
    cin >> index;

    if (index >= 0 && index < procesos.size()) {
        pid_t pid = fork();
        if (pid == 0) {
            execlp("ls", "ls", "-l", NULL);
            perror("Error al ejecutar exec");
            exit(1);
        }
        else {
            cout << "Ejecutando comando en hijo con PID: " << pid << endl;
            logAction("Comando ls -l ejecutado en hijo PID=" + to_string(pid));
            waitpid(pid, NULL, 0);
        }
    }
    else {
        cout << "Índice inválido." << endl;
    }
}

void esperarProcesos() {
    for (auto& p : procesos)
    {
        int status;
        waitpid(p.pid, &status, WNOHANG);
        if (WIFEXITED(status))
        {
            p.estado = "terminated";
            cout << "Proceso " << p.nombre << " ha terminado." << endl;
            logAction("Proceso terminado: " + p.nombre);
        }
    }
}

void suspenderProceso()
{
    int index;
    cout << "Índice del proceso a suspender: ";
    cin >> index;
    if (index >= 0 && index < procesos.size())
    {
        kill(procesos[index].pid, SIGSTOP);
        procesos[index].estado = "suspended";
        cout << "Proceso suspendido." << endl;
        logAction("Proceso suspendido: " + procesos[index].nombre);
    }
    else {
        cout << "Índice inválido." << endl;
    }
}

void reanudarProceso()
{
    int index;
    cout << "Índice del proceso a reanudar: ";
    cin >> index;
    if (index >= 0 && index < procesos.size())
    {
        kill(procesos[index].pid, SIGCONT);
        procesos[index].estado = "running";
        cout << "Proceso reanudado." << endl;
        logAction("Proceso reanudado: " + procesos[index].nombre);
    }
    else {
        cout << "Índice inválido." << endl;
    }
}

void terminarProceso()
{
    int index;
    cout << "Índice del proceso a terminar: ";
    cin >> index;
    if (index >= 0 && index < procesos.size())
    {
        kill(procesos[index].pid, SIGKILL);
        procesos[index].estado = "terminated";
        cout << "Proceso terminado." << endl;
        logAction("Proceso terminado manualmente: " + procesos[index].nombre);
    }
    else {
        cout << "Índice inválido." << endl;
    }
}

void mostrarProcesos()
{
    cout << "\nLista de procesos:\n";
    for (int i = 0; i < procesos.size(); i++) {
        cout << i << ". PID: " << procesos[i].pid
            << ", Nombre: " << procesos[i].nombre
            << ", Estado: " << procesos[i].estado << endl;
    }
}

void menuProcesosReales()
{
    int opcion;
    do {
        cout << "\n--- Gestión de Procesos Reales ---\n";
        cout << "1. Crear proceso\n";
        cout << "2. Ejecutar comando (ls -l)\n";
        cout << "3. Esperar procesos\n";
        cout << "4. Suspender proceso\n";
        cout << "5. Reanudar proceso\n";
        cout << "6. Terminar proceso\n";
        cout << "7. Mostrar procesos\n";
        cout << "0. Salir al menú principal\n";
        cout << "Opción: ";
        cin >> opcion;

        switch (opcion)
        {
        case 1: crearProceso(); break;
        case 2: ejecutarProceso(); break;
        case 3: esperarProcesos(); break;
        case 4: suspenderProceso(); break;
        case 5: reanudarProceso(); break;
        case 6: terminarProceso(); break;
        case 7: mostrarProcesos(); break;
        }

    } while (opcion != 0);
}

Usuario login() {
    Usuario u;
    cout << "---- LOGIN ----\n";
    cout << "Usuario: ";
    cin >> u.nombre;
    cout << "Tipo (root/user): ";
    cin >> u.tipo;
    return u;
}

int main() {
    Usuario actual = login();
    int opcion;

    do {
        cout << "\n=== SISTEMA OPERATIVO SIMULADO ===\n";
        cout << "1. FIFO\n2. Round Robin\n3. Sistema de Archivos\n4. gestion de procesos reales\n5. Salir\n";
        cout << "Seleccione una opcion: ";
        cin >> opcion;

        switch (opcion) {
        case 1:
            FIFO();
            break;
        case 2:
            RR();
            break;
        case 3:
            menuArchivos(actual.nombre);
            break;
        case 4:
            menuProcesosReales();
            break;
        case 5:
            cout << "Saliendo del sistema...\n";
            break;
        default:
            cout << "Opcion invalida.\n";
        }

    } while (opcion != 4);

    return 0;
}
