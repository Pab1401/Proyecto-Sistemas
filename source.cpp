#include <iostream>
#include <fstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <vector>
#include <ctime>

using namespace std;

// ===================== LOG DEL SISTEMA =====================
ofstream logFile;

void iniciarLog() {
    string logPath = "log.txt";

    if (!filesystem::exists(logPath)) {
        logFile.open(logPath, ios::out);
        logFile << "=== LOG DEL SISTEMA INICIADO ===\n";
    }
    else {
        logFile.open(logPath, ios::app);
        logFile << "\n=== NUEVA EJECUCIÓN DEL SISTEMA ===\n";
    }

    if (!logFile.is_open()) {
        cerr << "Error al abrir el archivo de log.\n";
    }
}

void logEvento(const string& evento) {
    if (logFile.is_open()) {
        time_t now = time(0);
        char* dt = ctime(&now);
        dt[strlen(dt) - 1] = '\0'; // Quitar salto de línea
        logFile << "[" << dt << "] " << evento << "\n";
        logFile.flush();
    }
}
// ===========================================================

struct Usuario {
    string nombre;
    string contrasena;
    bool esRoot;
};

vector<Usuario> usuarios = {
    {"root", "root123", true},
    {"usuario1", "user123", false}
};

Usuario* usuarioActual = nullptr;

bool login() {
    string nombre, contrasena;
    cout << "Login:\nUsuario: ";
    cin >> nombre;
    cout << "Contraseña: ";
    cin >> contrasena;

    for (auto& usuario : usuarios) {
        if (usuario.nombre == nombre && usuario.contrasena == contrasena) {
            usuarioActual = &usuario;
            cout << "Bienvenido, " << usuario.nombre << endl;
            logEvento("Usuario '" + usuario.nombre + "' inició sesión.");
            return true;
        }
    }

    logEvento("Intento de inicio de sesión fallido para el usuario '" + nombre + "'.");
    cout << "Login fallido\n";
    return false;
}

void ejecutarProcesoHijo() {
    execl("/bin/ls", "ls", nullptr);
    perror("Error al ejecutar exec");
    logEvento("Error: exec falló en proceso hijo.");
    exit(1);
}

void simularProcesos() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error al crear el proceso hijo");
        logEvento("Error: no se pudo crear el proceso hijo.");
    }
    else if (pid == 0) {
        logEvento("Proceso hijo creado.");
        ejecutarProcesoHijo();
    }
    else {
        logEvento("Proceso padre esperando al hijo.");
        wait(nullptr);
        logEvento("Proceso hijo finalizado.");
    }
}

void simularFIFO() {
    vector<string> procesos = { "P1", "P2", "P3" };
    for (const auto& proceso : procesos) {
        cout << "Ejecutando " << proceso << endl;
        logEvento("Planificador FIFO ejecuta proceso: " + proceso);
        sleep(1);
    }
}

void simularRoundRobin() {
    vector<string> procesos = { "P1", "P2", "P3" };
    int quantum = 1;
    for (int i = 0; i < 3; ++i) {
        for (const auto& proceso : procesos) {
            cout << "Ejecutando " << proceso << " por " << quantum << " segundos\n";
            logEvento("Planificador Round Robin ejecuta proceso: " + proceso);
            sleep(quantum);
        }
    }
}

void simularSistemaDeArchivos() {
    string nombreArchivo = "archivo.txt";
    ofstream archivo(nombreArchivo);
    archivo << "Contenido del archivo\n";
    archivo.close();
    logEvento("Archivo '" + nombreArchivo + "' creado y escrito.");

    ifstream archivoLectura(nombreArchivo);
    string linea;
    while (getline(archivoLectura, linea)) {
        cout << linea << endl;
    }
    archivoLectura.close();
    logEvento("Archivo '" + nombreArchivo + "' leído.");
}

void verificarPermisoYAccederArchivo(const string& archivo, bool escritura) {
    if (usuarioActual == nullptr) return;

    if (usuarioActual->esRoot) {
        logEvento("Acceso permitido a '" + archivo + "' para root.");
        cout << "Acceso concedido a '" << archivo << "' como root.\n";
    }
    else {
        if (escritura) {
            logEvento("Permiso denegado: el usuario intentó escribir en '" + archivo + "'.");
            cout << "Permiso denegado para escribir en '" << archivo << "'.\n";
        }
        else {
            logEvento("Acceso de solo lectura a '" + archivo + "' concedido.");
            cout << "Lectura de '" << archivo << "' permitida.\n";
        }
    }
}

int main() {
    iniciarLog();

    if (!login()) {
        logEvento("El sistema se cerró por fallo de autenticación.");
        logFile.close();
        return 1;
    }

    simularProcesos();
    simularFIFO();
    simularRoundRobin();
    simularSistemaDeArchivos();

    verificarPermisoYAccederArchivo("archivo.txt", true);

    logEvento("Sistema finalizado.");
    logFile.close();
    return 0;
}
