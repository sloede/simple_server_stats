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
  struct CommandLineArguments {
    string network_interface = DEFAULT_NETWORK_INTERFACE;
    int period = DEFAULT_PERIOD;
  };
}

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
     << "                        non-negative integer value (default: "
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

int main(int argc, char* argv[]) {
  // Parse command line options
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
          usage();
          exit(0);
        }

      case 'i':
        {
          args.network_interface = optarg;
          break;
        }

      case 'p':
        {
          stringstream ss(optarg);
          cout << "jo" << endl;
          ss >> args.period;
          cout << args.period << endl;
          cout << "man" << endl;
        }

      case '?':
        {
          usage();
          cout << "?" << endl;
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
  cout << "Network interface: " << args.network_interface << endl;
  cout << "Period:            " << args.period << endl;
  return 0;

  int c, verbose_flag;

  while (1)
    {
      static struct option long_options[] =
        {
          /* These options set a flag. */
          {"verbose", no_argument,       &verbose_flag, 1},
          {"brief",   no_argument,       &verbose_flag, 0},
          /* These options don’t set a flag.
             We distinguish them by their indices. */
          {"add",     no_argument,       0, 'a'},
          {"append",  no_argument,       0, 'b'},
          {"delete",  required_argument, 0, 'd'},
          {"create",  required_argument, 0, 'c'},
          {"file",    required_argument, 0, 'f'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "abc:d:f:",
                       long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 0:
          /* If this option set a flag, do nothing else now. */
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;

        case 'a':
          puts ("option -a\n");
          break;

        case 'b':
          puts ("option -b\n");
          break;

        case 'c':
          printf ("option -c with value `%s'\n", optarg);
          break;

        case 'd':
          printf ("option -d with value `%s'\n", optarg);
          break;

        case 'f':
          printf ("option -f with value `%s'\n", optarg);
          break;

        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
          abort ();
        }
    }

  /* Instead of reporting ‘--verbose’
     and ‘--brief’ as they are encountered,
     we report the final status resulting from them. */
  if (verbose_flag)
    puts ("verbose flag is set");

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }
  return 0;

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
        if (interface == DEFAULT_NETWORK_INTERFACE + ":") {
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
