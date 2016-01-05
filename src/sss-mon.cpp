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

    // Read CPU load information
    long long int user = 0;
    long long int nice = 0;
    long long int system = 0;
    long long int idle = 0;
    long long int iowait = 0;
    long long int irq = 0;
    long long int softirq = 0;
    long long int steal = 0;
    long long int guest = 0;
    long long int guest_nice = 0;
    {
      ifstream in("/proc/stat");
      string line;
      getline(in, line);
      istringstream l(line);
      string dump;
      l >> dump;
      l >> user;
      l >> nice;
      l >> system;
      l >> idle;
      l >> iowait;
      l >> irq;
      l >> softirq;
      l >> steal;
      l >> guest;
      l >> guest_nice;
    }

    // Bytes received/sent on NETWORK_IF
    long long int bytes_recv = 0;
    long long int bytes_send = 0;
    {
      ifstream in("/proc/net/dev");
      for (string line; getline(in, line); ) {
        istringstream l(line);
        string interface;
        l >> interface;
        if (interface == NETWORK_IF + ":") {
          l >> bytes_recv;
          for (int i = 0; i < 8; i++) {
            l >> bytes_send;
          }
        }
      }
    }

    cout << date
         << " " << bytes_recv << " " << bytes_send
         << " " << user << " " << nice << " " << system << " " << idle
         << " " << iowait << " " << irq << " " << softirq
         << " " << steal << " " << guest << " " << guest_nice
         << endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}
