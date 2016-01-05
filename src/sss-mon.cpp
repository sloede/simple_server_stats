#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <fstream>
#include <sstream>

const std::string NETWORK_IF = "eth0";

using namespace std;


int main() {
  while (true) {
    // Get current point in time as reference
    const auto now = std::chrono::steady_clock::now();
    const auto date = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();

    // CPU statistics
    long long int cpu_user = 0;
    long long int cpu_nice = 0;
    long long int cpu_system = 0;
    long long int cpu_idle = 0;
    long long int cpu_iowait = 0;
    long long int cpu_irq = 0;
    long long int cpu_softirq = 0;
    long long int cpu_steal = 0;
    long long int cpu_guest = 0;
    long long int cpu_guest_nice = 0;
    {
      ifstream in("/proc/stat");
      string line;
      getline(in, line);
      istringstream l(line);
      string dump;
      l >> dump;
      l >> cpu_user;
      l >> cpu_nice;
      l >> cpu_system;
      l >> cpu_idle;
      l >> cpu_iowait;
      l >> cpu_irq;
      l >> cpu_softirq;
      l >> cpu_steal;
      l >> cpu_guest;
      l >> cpu_guest_nice;
    }

    // Memory usage
    long long int memory_available;
    long long int memory_used;
    long long int swap_available;
    long long int swap_used;
    // {
    //   ifstream in("/proc/net/dev");
    //   for (string line; getline(in, line); ) {
    //     istringstream l(line);
    //     string interface;
    //     l >> interface;
    //     if (interface == NETWORK_IF + ":") {
    //       l >> bytes_recv;
    //       for (int i = 0; i < 8; i++) {
    //         l >> bytes_send;
    //       }
    //     }
    //   }
    // }

    // Bytes received/sent on NETWORK_IF
    long long int network_received = 0;
    long long int network_sent = 0;
    {
      ifstream in("/proc/net/dev");
      for (string line; getline(in, line); ) {
        istringstream l(line);
        string interface;
        l >> interface;
        if (interface == NETWORK_IF + ":") {
          l >> network_received;
          for (int i = 0; i < 8; i++) {
            l >> network_sent;
          }
        }
      }
    }

    cout << date
         << " " << network_received
         << " " << network_sent
         << " " << cpu_user
         << " " << cpu_nice
         << " " << cpu_system
         << " " << cpu_idle
         << " " << cpu_iowait
         << " " << cpu_irq
         << " " << cpu_softirq
         << " " << cpu_steal
         << " " << cpu_guest
         << " " << cpu_guest_nice
         << endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
