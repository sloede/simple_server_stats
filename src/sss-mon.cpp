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
    // Current point in time as reference
    const auto date = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Steady clock timestamp for robust time-average calculation
    const auto steady = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    // CPU statistics
    unsigned long long int cpu_user = 0;
    unsigned long long int cpu_nice = 0;
    unsigned long long int cpu_system = 0;
    unsigned long long int cpu_idle = 0;
    unsigned long long int cpu_iowait = 0;
    unsigned long long int cpu_irq = 0;
    unsigned long long int cpu_softirq = 0;
    unsigned long long int cpu_steal = 0;
    unsigned long long int cpu_guest = 0;
    unsigned long long int cpu_guest_nice = 0;
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
    unsigned long long int memory_total = 0;
    unsigned long long int memory_used = 0;
    unsigned long long int swap_total = 0;
    unsigned long long int swap_used = 0;
    {
      ifstream in("/proc/meminfo");
      unsigned long long int memory_free = 0;
      unsigned long long int buffers = 0;
      unsigned long long int cached = 0;
      unsigned long long int swap_free = 0;
      for (string line; getline(in, line); ) {
        istringstream l(line);
        string type;
        l >> type;
        if (type == "MemTotal:") {
          l >> memory_total;
        } else if (type == "MemFree:") {
          l >> memory_free;
        } else if (type == "Buffers:") {
          l >> buffers;
        } else if (type == "Cached:") {
          l >> cached;
        } else if (type == "SwapTotal:") {
          l >> swap_total;
        } else if (type == "SwapFree:") {
          l >> swap_free;
        }
      }

      memory_used = memory_total - memory_free - buffers - cached;
      swap_used = swap_total - swap_free;

      memory_total *= 1024;
      memory_used *= 1024;
      swap_total *= 1024;
      swap_used *= 1024;
    }

    // Bytes received/sent on NETWORK_IF
    unsigned long long int network_received = 0;
    unsigned long long int network_sent = 0;
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
          break;
        }
      }
    }

    // Print to stdout
    cout << date
         << " " << steady
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
         << " " << memory_total
         << " " << memory_used
         << " " << swap_total
         << " " << swap_used
         << " " << network_received
         << " " << network_sent
         << endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
