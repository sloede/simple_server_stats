#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <fstream>
#include <sstream>
#include <getopt.h>

// Set sensible default values for network interface and sampling period
const int DEFAULT_ITERATIONS = 0;
const std::string DEFAULT_NETWORK_INTERFACE = "eth0";
const std::string DEFAULT_LOG_FILE = "";
const int DEFAULT_PERIOD = 1;

namespace {
  /// Internal data structure for command line arguments
  struct CommandLineArguments {
    unsigned long long int iterations = DEFAULT_ITERATIONS;
    std::string log_file = DEFAULT_LOG_FILE;
    std::string network_interface = DEFAULT_NETWORK_INTERFACE;
    int period = DEFAULT_PERIOD;
  };

  /// Internal data structure for statistical data sample
  struct Sample {
    // Timestamp values may be signed
    long long int date = 0;
    long long int steady = 0;

    // All other values are defined as unsigned
    double cpu_load_1m = 0.0;
    double cpu_load_5m = 0.0;
    double cpu_load_15m = 0.0;
    unsigned long long int cpu_time_user = 0;
    unsigned long long int cpu_time_nice = 0;
    unsigned long long int cpu_time_system = 0;
    unsigned long long int cpu_time_idle = 0;
    unsigned long long int cpu_time_iowait = 0;
    unsigned long long int cpu_time_irq = 0;
    unsigned long long int cpu_time_softirq = 0;
    unsigned long long int cpu_time_steal = 0;
    unsigned long long int cpu_time_guest = 0;
    unsigned long long int cpu_time_guest_nice = 0;
    unsigned long long int memory_total = 0;
    unsigned long long int memory_used = 0;
    unsigned long long int swap_total = 0;
    unsigned long long int swap_used = 0;
    unsigned long long int network_received = 0;
    unsigned long long int network_sent = 0;
  };
}


/// Print usage information
void print_usage(std::ostream& os = std::cerr) {
  os << "usage: sss-mon [-f] [-h] [-i INTERFACE] [-p PERIOD] [LOGFILE]\n"
     << "\n"
     << "sss-mon gathers information on the current CPU load, memory usage, \n"
     << "and network traffic and writes it to stdout or a log file. Once \n"
     << "started, it runs continuously until killed, unless a maximum number\n"
     << "of iterations is specified on the command line.\n"
     << "\n"
     << "positional arguments:\n"
     << "  LOGFILE               Write data to LOGFILE instead of stdout.\n"
     << "\n"
     << "optional arguments:\n"
     << "  -f, --field-names     Print space-separated list of field names\n"
     << "                        to stdout and exit.\n"
     << "  -h, --help            Show this help message and exit.\n"
     << "  -n, --iterations ITERATIONS\n"
     << "                        Number of samples to gather. Must be a \n"
     << "                        non-negative integer value. If set to zero, \n"
     << "                        sss-mon continues indefinitely until killed \n"
     << "                        (default: " << DEFAULT_ITERATIONS << ").\n"
     << "  -i, --network-interface INTERFACE\n"
     << "                        Gather traffic statistics for the given\n"
     << "                        network interface. INTERFACE must be a valid\n"
     << "                        interface in '/proc/net/dev' (default: "
     << DEFAULT_NETWORK_INTERFACE << ").\n"
     << "  -p, --period PERIOD   Set sampling period (in seconds). Must be a\n"
     << "                        positive integer value (default: "
     << DEFAULT_PERIOD << ").\n"
     << "\n"
     << "For each sample, a space-separated list of the following fields is\n"
     << "written to stdout or a log file and terminated by a newline \n"
     << "character (\\n):\n"
     << "  date                  Unix timestamp in milliseconds.\n"
     << "  steady                A steady timestamp (not necessarily since\n"
     << "                        Unix epoch) for robust time-average \n"
     << "                        calculations in microseconds.\n"
     << "  cpu_load_1m           CPU load average (1 minute average).\n"
     << "  cpu_load_5m           CPU load average (5 minute average).\n"
     << "  cpu_load_15m          CPU load average (15 minute average).\n"
     << "  cpu_time_user         CPU time spent in user mode.\n"
     << "  cpu_time_nice         CPU time spent in user mode with low \n"
     << "                        priority (nice).\n"
     << "  cpu_time_system       CPU time spent in system mode.\n"
     << "  cpu_time_idle         CPU time spent in the idle task.\n"
     << "  cpu_time_iowait       CPU time waiting for I/O to complete.\n"
     << "  cpu_time_irq          CPU time servicing interrupts.\n"
     << "  cpu_time_softirq      CPU time ervicing softirqs.\n"
     << "  cpu_time_steal        CPU stolen time, which is the time spent in\n"
     << "                        other operating systems when running in a \n"
     << "                        virtualized environment.\n"
     << "  cpu_time_guest        CPU time spent running a virtual CPU for \n"
     << "                        guest operating systems under the control of\n"
     << "                        the Linux kernel.\n"
     << "  cpu_time_guest_nice   CPU time spent running a niced guest \n"
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
     << "also 'man proc'). CPU data is from '/proc/loadavg' and '/proc/stat',\n"
     << "memory data is from '/proc/meminfo', and network data is from\n"
     << "'/proc/net/dev'.\n";
  os.flush();
}


/// Parse command line options
CommandLineArguments parse_arguments(int argc, char* argv[]) {
  CommandLineArguments args;

  // Parse option arguments
  while (true) {
    // Create structure with long options
    static struct option long_options[] = {
      {"field-names", no_argument, nullptr, 'f'},
      {"help", no_argument, nullptr, 'h'},
      {"iterations", required_argument, nullptr, 'n'},
      {"network-interface", required_argument, nullptr, 'i'},
      {"period", required_argument, nullptr, 'p'},
      {nullptr, 0, nullptr, 0}
    };

    // Get next argument
    const int c = getopt_long(argc, argv, "fhn:l:i:p:", long_options, nullptr);

    // Exit loop if end of options is reached
    if (c == -1) {
      break;
    }

    // Handle argument
    switch (c) {
      // Print field names and quit
      case 'f':
        {
          std::cout << "date"
                    << " steady"
                    << " cpu_load_1m"
                    << " cpu_load_5m"
                    << " cpu_load_15m"
                    << " cpu_time_user"
                    << " cpu_time_nice"
                    << " cpu_time_system"
                    << " cpu_time_idle"
                    << " cpu_time_iowait"
                    << " cpu_time_irq"
                    << " cpu_time_softirq"
                    << " cpu_time_steal"
                    << " cpu_time_guest"
                    << " cpu_time_guest_nice"
                    << " memory_total"
                    << " memory_used"
                    << " swap_total"
                    << " swap_used"
                    << " network_received"
                    << " network_sent"
                    << std::endl;
          exit(0);
        }

      // Show usage information and quit
      case 'h':
        {
          print_usage(std::cout);
          exit(0);
        }

      // Set number of iterations
      case 'n':
        {
          // Try to parse argument as integer
          std::istringstream arg(optarg);
          arg >> args.iterations;

          // Handle bad arguments
          if (arg.fail()
              || arg.get() != std::istringstream::traits_type::eof()) {
            std::cerr << "error: argument to '-n|--iterations' (" << optarg
                      << ") is not an integer" << std::endl;
            exit(2);
          }
          break;
        }

      // Set network interface
      case 'i':
        {
          args.network_interface = optarg;
          break;
        }

      // Set sampling period
      case 'p':
        {
          // Try to parse argument as integer
          std::istringstream arg(optarg);
          arg >> args.period;

          // Handle bad arguments
          if (arg.fail()
              || arg.get() != std::istringstream::traits_type::eof()) {
            std::cerr << "error: argument to '-p|--period' (" << optarg
                      << ") is not an integer" << std::endl;
            exit(2);
          } else if (args.period < 1) {
            std::cerr << "error: argument to '-p|--period' (" << optarg
                      << ") is less than one" << std::endl;
            exit(2);
          }
          break;
        }

      // If an unknown/bad argument was encountered, show usage and quit
      case '?':
        {
          print_usage();
          exit(2);
          break;
        }

      // The default should never be reached and signifies an unknown problem
      default:
        {
          std::cerr << "error: unknown error while parsing command line "
                    << "arguments" << std::endl;
          exit(1);
        }
    }
  }

  // Parse for optional log file
  if (optind < argc) {
    args.log_file = argv[optind];
    if (args.log_file == "") {
      std::cerr << "error: log file name is empty" << std::endl;
      exit(2);
    }
  }

  return args;
}


/// Gather data sample
Sample sample(const std::string& network_interface) {
  // Create sample object
  Sample s{};

  // Current point in time as reference
  s.date = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

  // Steady clock timestamp for robust time-average calculation
  s.steady = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();

  // CPU load averages
  {
    // Open file
    std::ifstream in("/proc/loadavg");

    // Read line
    std::string line;
    std::getline(in, line);
    std::istringstream l(line);

    // Read and convert CPU load averages
    l >> s.cpu_load_1m;
    l >> s.cpu_load_5m;
    l >> s.cpu_load_15m;
  }

  // CPU statistics
  {
    // Open file
    std::ifstream in("/proc/stat");

    // Read first line with cumulated values
    std::string line;
    std::getline(in, line);
    std::istringstream l(line);

    // Read first word and throw it away (will probably be 'cpu')
    std::string dump;
    l >> dump;

    // Read and convert CPU statistics
    l >> s.cpu_time_user;
    l >> s.cpu_time_nice;
    l >> s.cpu_time_system;
    l >> s.cpu_time_idle;
    l >> s.cpu_time_iowait;
    l >> s.cpu_time_irq;
    l >> s.cpu_time_softirq;
    l >> s.cpu_time_steal;
    l >> s.cpu_time_guest;
    l >> s.cpu_time_guest_nice;
  }

  // Memory usage
  {
    // Open file
    std::ifstream in("/proc/meminfo");

    // Read file line by line and parse each line for relevant data
    unsigned long long int memory_free = 0;
    unsigned long long int buffers = 0;
    unsigned long long int cached = 0;
    unsigned long long int swap_free = 0;
    for (std::string line; std::getline(in, line); ) {
      // Extract data type
      std::istringstream l(line);
      std::string type;
      l >> type;

      // Store data depending on type
      if (type == "MemTotal:") {
        l >> s.memory_total;
      } else if (type == "MemFree:") {
        l >> memory_free;
      } else if (type == "Buffers:") {
        l >> buffers;
      } else if (type == "Cached:") {
        l >> cached;
      } else if (type == "SwapTotal:") {
        l >> s.swap_total;
      } else if (type == "SwapFree:") {
        l >> swap_free;
      }
    }

    // Calculate used memory and used swap space
    s.memory_used = s.memory_total - memory_free - buffers - cached;
    s.swap_used = s.swap_total - swap_free;

    // Convert values from kibibytes to bytes
    s.memory_total *= 1024;
    s.memory_used *= 1024;
    s.swap_total *= 1024;
    s.swap_used *= 1024;
  }

  // Bytes received/sent on network interface
  {
    // Open file
    std::ifstream in("/proc/net/dev");

    // Read file line by line until selected interface is found
    for (std::string line; std::getline(in, line); ) {
      // Extract interface name
      std::istringstream l(line);
      std::string interface;
      l >> interface;

      // If specified interface is found, read data and exit loop
      if (interface == network_interface + ":") {
        // Bytes received is the 2nd value
        l >> s.network_received;

        // Bytes sent is the 10th value
        for (int i = 0; i < 8; i++) {
          l >> s.network_sent;
        }
        break;
      }
    }
  }

  return s;
}


int main(int argc, char* argv[]) {
  // Parse command line arguments
  const auto args = parse_arguments(argc, argv);

  // Open log file if specified
  std::ofstream log_file;
  if (args.log_file != "") {
    log_file.open(args.log_file, std::ios::out | std::ios::app);
    if (!log_file.good()) {
      std::cerr << "error: could not open log file '" << args.log_file
                << "' for writing" << std::endl;
      std::exit(1);
    }
    std::cout << "Writing data to '" << args.log_file << "'..." << std::endl;
  }

  // Set output stream to use
  std::ostream os(args.log_file == "" ? std::cout.rdbuf() : log_file.rdbuf());

  // Begin main loop
  const auto sleep_time = std::chrono::microseconds(1000000 * args.period);
  for (unsigned long long int iteration = 0;;) {
    // Obtain sample
    const Sample s = sample(args.network_interface);

    // Print all data to stdout
    os << s.date
       << " " << s.steady
       << " " << s.cpu_load_1m
       << " " << s.cpu_load_5m
       << " " << s.cpu_load_15m
       << " " << s.cpu_time_user
       << " " << s.cpu_time_nice
       << " " << s.cpu_time_system
       << " " << s.cpu_time_idle
       << " " << s.cpu_time_iowait
       << " " << s.cpu_time_irq
       << " " << s.cpu_time_softirq
       << " " << s.cpu_time_steal
       << " " << s.cpu_time_guest
       << " " << s.cpu_time_guest_nice
       << " " << s.memory_total
       << " " << s.memory_used
       << " " << s.swap_total
       << " " << s.swap_used
       << " " << s.network_received
       << " " << s.network_sent
       << std::endl;

    // Increment interation counter
    iteration++;

    // Exit main loop if maximum number of iterations is reached
    if (args.iterations > 0 && iteration >= args.iterations) {
      break;
    }

    // Sleep until next sample time
    std::this_thread::sleep_for(sleep_time);
  }
}
