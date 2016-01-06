#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <fstream>
#include <sstream>
#include <getopt.h>

const std::string DEFAULT_NETWORK_INTERFACE = "eth0";
const int DEFAULT_PERIOD = 1;

using namespace std;

namespace {
  // Internal data structure for command line arguments
  struct CommandLineArguments {
    string network_interface = DEFAULT_NETWORK_INTERFACE;
    int period = DEFAULT_PERIOD;
  };
}


// Print usage information
void usage(std::ostream& os = std::cerr) {
  os << "usage: sss-mon [-f] [-h] [-i INTERFACE] [-p PERIOD]\n"
     << "\n"
     << "sss-mon gathers information on the current CPU load, memory usage, \n"
     << "and network traffic and prints it to stdout. Once started, it runs \n"
     << "continuously until killed.\n"
     << "\n"
     << "optional arguments:\n"
     << "  -f, --field-names     Print space-separated list of field names\n"
     << "                        and exit.\n"
     << "  -h, --help            Show this help message and exit.\n"
     << "  -i, --network-interface INTERFACE\n"
     << "                        Gather traffic statistics for the given\n"
     << "                        network interface. INTERFACE must be a valid\n"
     << "                        interface in '/proc/net/dev' (default: "
     << DEFAULT_NETWORK_INTERFACE << ").\n"
     << "  -p, --period PERIOD   Set sampling period (in seconds). Must be a\n"
     << "                        positive integer value (default: "
     << DEFAULT_PERIOD << ").\n"
     << "\n"
     << "For each sample, a space-seperated list of the following fields is\n"
     << "printed to stdout and terminated by a newline character (\\n):\n"
     << "  date                  Unix timestamp in milliseconds.\n"
     << "  steady                A steady timestamp (not necessarily since\n"
     << "                        Unix epoch) for robust time-average \n"
     << "                        calculations in microseconds.\n"
     << "  cpu_user              CPU time spent in user mode.\n"
     << "  cpu_nice              CPU time spent in user mode with low \n"
     << "                        priority (nice).\n"
     << "  cpu_system            CPU time spent in system mode.\n"
     << "  cpu_idle              CPU time spent in the idle task.\n"
     << "  cpu_iowait            CPU time waiting for I/O to complete.\n"
     << "  cpu_irq               CPU time servicing interrupts.\n"
     << "  cpu_softirq           CPU time ervicing softirqs.\n"
     << "  cpu_steal             CPU stolen time, which is the time spent in\n"
     << "                        other operating systems when running in a \n"
     << "                        virtualized environment.\n"
     << "  cpu_guest             CPU time spent running a virtual CPU for \n"
     << "                        guest operating systems under the control of\n"
     << "                        the Linux kernel.\n"
     << "  cpu_guest_nice        CPU time spent running a niced guest \n"
     << "                        (virtual CPU for guest operating systems\n"
     << "                        under the control of the Linux kernel).\n"
     << "  memory_total          Total usable RAM (in bytes).\n"
     << "  memory_used           Memory currently in use (in bytes).\n"
     << "  swap_total            Total amount of swap space (in bytes).\n"
     << "  swap_used             Swap space currently in use (in bytes).\n"
     << "  network_received      Total bytes received (in bytes).\n"
     << "  network_sent          Total bytes sent (in bytes).\n"
     << "\n"
     << "The recorded information is gathered from the 'proc' filesystem (see\n"
     << "also 'man proc'). CPU data is from '/proc/stat', memory data is from\n"
     << "'/proc/meminfo', and network data is from '/proc/net/dev'.\n";
  os.flush();
}


// Parse command line options
CommandLineArguments parse_arguments(int argc, char* argv[]) {
  CommandLineArguments args;
  while (true) {
    // Create structure with long options
    static struct option long_options[] = {
      {"field-names", no_argument, nullptr, 'f'},
      {"help", no_argument, nullptr, 'h'},
      {"network-interface", required_argument, nullptr, 'i'},
      {"period", required_argument, nullptr, 'p'},
      {nullptr, 0, nullptr, 0}
    };

    // Get next argument
    const int c = getopt_long(argc, argv, "hl:i:p:", long_options, nullptr);

    // Break if end of options is reached
    if (c == -1) {
      break;
    }

    // Parse arguments
    switch (c) {
      case 'f':
        {
          exit(0);
        }

      case 'h':
        {
          usage(cout);
          exit(0);
        }

      case 'i':
        {
          args.network_interface = optarg;
          break;
        }

      case 'p':
        {
          istringstream arg(optarg);
          arg >> args.period;
          if (arg.fail() || arg.get() != istringstream::traits_type::eof()) {
            cerr << "error: argument to '-p|--period' (" << optarg
                 << ") is not an integer" << endl;
            exit(2);
          } else if (args.period < 1) {
            cerr << "error: argument to '-p|--period' (" << optarg
                 << ") is less than one" << endl;
            exit(2);
          }
          break;
        }

      case '?':
        {
          usage();
          exit(2);
          break;
        }

      default:
        {
          cerr << "error: unknown error while parsing command line arguments"
               << endl;
          exit(1);
        }
    }
  }

  return args;
}


int main(int argc, char* argv[]) {
  // Parse command line arguments
  const auto args = parse_arguments(argc, argv);

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

    // Bytes received/sent on network interface
    unsigned long long int network_received = 0;
    unsigned long long int network_sent = 0;
    {
      ifstream in("/proc/net/dev");
      for (string line; getline(in, line); ) {
        istringstream l(line);
        string interface;
        l >> interface;
        if (interface == args.network_interface + ":") {
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

    // Sleep until next sample time
    std::this_thread::sleep_for(std::chrono::seconds(args.period));
  }
}
