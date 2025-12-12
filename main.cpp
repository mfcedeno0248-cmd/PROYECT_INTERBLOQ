// TAREA6.01.cpp
#include <bits/stdc++.h>
#include <mutex>
using namespace std;

struct SystemState {
    // Número de procesos y recursos
    size_t P, R;

    // Recursos disponibles
    vector<int> available;            // tamaño R

    // Asignación actual por proceso y recurso
    vector<vector<int>> allocation;   // P x R

    // Solicitud pendiente por proceso y recurso
    vector<vector<int>> request;      // P x R
};

class ResourceManager {
    SystemState s;
    mutable std::mutex mtx;

    bool leq(const vector<int>& a, const vector<int>& b) const {
        for (size_t i = 0; i < a.size(); ++i) if (a[i] > b[i]) return false;
        return true;
    }

public:
    ResourceManager(size_t P, size_t R, const vector<int>& initialAvailable)
    {
        s.P = P; s.R = R;
        s.available = initialAvailable;
        s.allocation.assign(P, vector<int>(R, 0));
        s.request.assign(P, vector<int>(R, 0));
    }

    // Registrar una nueva solicitud pendiente del proceso pid
    bool addRequest(size_t pid, const vector<int>& req) {
        lock_guard<mutex> lock(mtx);
        if (pid >= s.P || req.size() != s.R) return false;
        for (size_t j = 0; j < s.R; ++j) {
            s.request[pid][j] += req[j];
        }
        return true;
    }

    // Intentar asignar inmediatamente si hay disponibilidad
    bool tryAllocate(size_t pid) {
        lock_guard<mutex> lock(mtx);
        if (pid >= s.P) return false;
        if (!leq(s.request[pid], s.available)) return false; // no hay suficiente

        // Asignar
        for (size_t j = 0; j < s.R; ++j) {
            s.available[j] -= s.request[pid][j];
            s.allocation[pid][j] += s.request[pid][j];
            s.request[pid][j] = 0; // solicitud satisfecha
        }
        return true;
    }

    // Liberar recursos del proceso pid
    bool release(size_t pid, const vector<int>& rel) {
        lock_guard<mutex> lock(mtx);
        if (pid >= s.P || rel.size() != s.R) return false;
        for (size_t j = 0; j < s.R; ++j) {
            int give = min(rel[j], s.allocation[pid][j]);
            s.allocation[pid][j] -= give;
            s.available[j] += give;
        }
        return true;
    }

    // Detección de interbloqueo
    // Devuelve el conjunto de procesos potencialmente en interbloqueo
    vector<size_t> detectDeadlock() const {
        lock_guard<mutex> lock(mtx);

        vector<int> work = s.available;
        vector<bool> finish(s.P, false);

        // Progreso mientras existan procesos cuyos request <= work
        bool progress = true;
        while (progress) {
            progress = false;
            for (size_t i = 0; i < s.P; ++i) {
                if (finish[i]) continue;
                if (leq(s.request[i], work)) {
                    // Simular finalización del proceso i
                    for (size_t j = 0; j < s.R; ++j) {
                        work[j] += s.allocation[i][j];
                    }
                    finish[i] = true;
                    progress = true;
                }
            }
        }

        // Procesos no finalizados que tienen recursos asignados están potencialmente en interbloqueo
        vector<size_t> deadlocked;
        for (size_t i = 0; i < s.P; ++i) {
            int allocatedSum = accumulate(s.allocation[i].begin(), s.allocation[i].end(), 0);
            if (!finish[i] && allocatedSum > 0) {
                deadlocked.push_back(i);
            }
        }
        return deadlocked;
    }

    // Estado para impresión
    void printState() const {
        lock_guard<mutex> lock(mtx);
        cout << "\n== Estado del sistema ==\n";
        cout << "Disponibles: ";
        for (auto v : s.available) cout << v << " ";
        cout << "\nAsignacion:\n";
        for (size_t i = 0; i < s.P; ++i) {
            cout << " P" << i << ": ";
            for (auto v : s.allocation[i]) cout << v << " ";
            cout << "\n";
        }
        cout << "Solicitud pendiente:\n";
        for (size_t i = 0; i < s.P; ++i) {
            cout << " P" << i << ": ";
            for (auto v : s.request[i]) cout << v << " ";
            cout << "\n";
        }
        cout << "========================\n";
    }
};

int main() {
    ios::sync_with_stdio(false);

    // Ejemplo: 3 procesos, 2 recursos [Memoria, CPU]
    // Memoria=500, CPU=4
    ResourceManager rm(3, 2, {500, 4});

    cout << "--- Simulacion de asignacion y deteccion de interbloqueos ---\n";
    rm.printState();

    // Escenario de solicitudes
    cout << "\n[Accion] P0 solicita 50MB y 1CPU\n";
    rm.addRequest(0, {50, 1});
    cout << (rm.tryAllocate(0) ? " -> Asignado\n" : " -> No asignado\n");
    rm.printState();

    cout << "\n[Accion] P1 solicita 80MB y 2CPU\n";
    rm.addRequest(1, {80, 2});
    cout << (rm.tryAllocate(1) ? " -> Asignado\n" : " -> No asignado\n");
    rm.printState();

    cout << "\n[Accion] P2 solicita 300MB y 2CPU\n";
    rm.addRequest(2, {300, 2});
    cout << (rm.tryAllocate(2) ? " -> Asignado\n" : " -> No asignado (pendiente)\n");
    rm.printState();

    // Intentar detectar interbloqueo
    auto dead = rm.detectDeadlock();
    if (dead.empty()) {
        cout << "\n[Deteccion] No hay interbloqueo\n";
    } else {
        cout << "\n[Deteccion] Interbloqueo detectado. Procesos involucrados: ";
        for (auto p : dead) cout << "P" << p << " ";
        cout << "\n";
    }

    // Liberacion para demostrar salida del bloqueo
    cout << "\n[Accion] Liberar recursos de P1: 80MB y 2CPU\n";
    rm.release(1, {80, 2});
    rm.printState();

    // Reintento de asignacion de P2
    cout << "\n[Accion] Reintento de P2\n";
    cout << (rm.tryAllocate(2) ? " -> Asignado\n" : " -> Sigue pendiente\n");
    rm.printState();

    cout << "\n--- Fin de la simulacion ---\n";
    cout << "Presione Enter para continuar...";
    cin.get();
    return 0;
}

