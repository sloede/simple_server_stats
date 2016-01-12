#include <array>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <fstream>
#include <sstream>
#include <getopt.h>
#include <sys/statvfs.h>

// Define types for statistic data - use 64 bit values (signed) for maximum
// range and ease-of-use
using Int = long long int;
using Float = double;

// Set sensible default values for network interface and sampling period
constexpr const Int DEFAULT_ITERATIONS = 0;
const std::string DEFAULT_NETWORK_INTERFACE = "eth0";
const std::string DEFAULT_LOG_FILE = "";
constexpr const Int DEFAULT_PERIOD = 1;
const std::string DEFAULT_STAT_PATH = ".";

namespace {
  /// Maximum file name length for time-encoded log files
  constexpr const Int max_log_file_name_length = 512;

  /// Internal data structure for command line arguments
  struct CommandLineArguments {
    Int iterations = DEFAULT_ITERATIONS;
    std::string log_file = DEFAULT_LOG_FILE;
    std::string network_interface = DEFAULT_NETWORK_INTERFACE;
    Int period = DEFAULT_PERIOD;
    std::string stat_path = DEFAULT_STAT_PATH;
  };

  /// Internal data structure for statistical data sample
  struct Sample {
    // Timestamp values may be signed
    Int date = 0;
    Int steady = 0;

    // Load values may be decimal numbers
    Float cpu_load_1m = 0.0;
    Float cpu_load_5m = 0.0;
    Float cpu_load_15m = 0.0;

    // All other values are counters that cannot be negative
    Int cpu_time_user = 0;
    Int cpu_time_nice = 0;
    Int cpu_time_system = 0;
    Int cpu_time_idle = 0;
    Int cpu_time_iowait = 0;
    Int cpu_time_irq = 0;
    Int cpu_time_softirq = 0;
    Int cpu_time_steal = 0;
    Int cpu_time_guest = 0;
    Int cpu_time_guest_nice = 0;
    Int memory_total = 0;
    Int memory_used = 0;
    Int swap_total = 0;
    Int swap_used = 0;
    Int disk_total = 0;
    Int disk_used = 0;
    Int disk_available = 0;
    Int network_received = 0;
    Int network_sent = 0;
  };
}


/// Print usage information
static void print_usage(std::ostream& os = std::cerr) {
  os << "usage: sss-mon [-f] [-h] [-i INTERFACE] [-p PERIOD] [LOGFILE]\n"
     << "\n"
     << "sss-mon gathers information on the current CPU load, memory usage, \n"
     << "and network traffic and writes it to stdout or a log file. Once \n"
     << "started, it runs continuously until killed, unless a maximum number\n"
     << "of iterations is specified on the command line.\n"
     << "\n"
     << "positional arguments:\n"
     << "  LOGFILE               Write data to LOGFILE instead of stdout. The\n"
     << "                        file name may contain time format strings as\n"
     << "                        defined in std::strftime, and the actual\n"
     << "                        name will be re-determined before each\n"
     << "                        write. The overall file name length (after\n"
     << "                        optional time formats have been applied)\n"
     << "                        must be less than " << max_log_file_name_length
     << ".\n"
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
     << "  -s, --stat-file       Path to file/directory on the file system\n"
     << "                        that should be used to gather disk usage\n"
     << "                        statistics (default: " << DEFAULT_STAT_PATH
     << ").\n"
     << "\n"
     << "For each sample, a space-separated list of the following fields is\n"
     << "written to stdout or a log file and terminated by a newline \n"
     << "character (\\n):\n"
     << "  date                  Unix timestamp (in milliseconds).\n"
     << "  time_delta            Time since last sample was recorded (in \n"
     << "                        milliseconds). A value of zero indicates\n"
     << "                        that this is the first sample since\n"
     << "                        (re-)starting sss-mon.\n"
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
     << "  disk_total            Total usable disk space (in bytes).\n"
     << "  disk_used             Disk space currently in use (in bytes).\n"
     << "  disk_available        Disk space available for non-privileged\n"
     << "                        users (in bytes).\n"
     << "  network_received      Total bytes received (in bytes).\n"
     << "  network_sent          Total bytes sent (in bytes).\n"
     << "\n"
     << "Most of the information is gathered from the 'proc' filesystem (see\n"
     << "also 'man proc'). CPU data is from '/proc/loadavg' and '/proc/stat',\n"
     << "memory data is from '/proc/meminfo', and network data is from\n"
     << "'/proc/net/dev'. Disk usage statistics are obtained through the\n"
     << "system call 'statvfs()'.\n";
  os.flush();
}


/// Parse command line options
static CommandLineArguments parse_arguments(int argc, char* argv[]) {
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
      {"stat-path", required_argument, nullptr, 's'},
      {nullptr, 0, nullptr, 0}
    };

    // Get next argument
    const auto c = getopt_long(
        argc, argv, "fhn:l:i:p:s:", long_options, nullptr);

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
                    << " time_delta"
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
                    << " disk_total"
                    << " disk_used"
                    << " disk_available"
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

      // Set stat path
      case 's':
        {
          args.stat_path = optarg;

          // Make sure that the path exists and can be used
          struct statvfs sb;
          if (statvfs(args.stat_path.c_str(), &sb) != 0) {
            std::cerr << "error: stat path (" << args.stat_path << ") does"
                      << " not exit or cannot be used" << std::endl;
            std::exit(2);
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
    if (args.log_file.empty()) {
      std::cerr << "error: log file name is empty" << std::endl;
      exit(2);
    }
    if (args.log_file.length() > max_log_file_name_length) {
      std::cerr << "error: log file name is too long (must be < "
                << max_log_file_name_length << ")" << std::endl;
      exit(2);
    }
  }

  return args;
}


/// Gather data sample
static Sample sample(const std::string& network_interface,
                     const std::string& stat_path) {
  // Create sample object
  Sample s{};

  // Current point in time as reference
  s.date = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

  // Steady clock timestamp for robust time-average calculation
  s.steady = std::chrono::duration_cast<std::chrono::milliseconds>(
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
    Int memory_free = 0;
    Int buffers = 0;
    Int cached = 0;
    Int swap_free = 0;
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

  // Disk usage
  {
    // Call statvfs to get information on file system
    struct statvfs sb;
    statvfs(stat_path.c_str(), &sb);

    // All values of interest from statvfs are given in blocks, thus to obtain
    // the byte value they have to be multiplied by frsize
    // Total = disk capacity
    s.disk_total = sb.f_blocks * sb.f_frsize;
    // Used = capacity minus what kernel/root may use
    s.disk_used = (sb.f_blocks - sb.f_bfree) * sb.f_frsize;
    // Available = remaining capacity that normal users may use
    s.disk_available = sb.f_bavail * sb.f_frsize;
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
        for (Int i = 0; i < 8; i++) {
          l >> s.network_sent;
        }
        break;
      }
    }
  }

  return s;
}


/// Parse string through std::strftime using the current system time
static std::string time_formatted(const std::string& s) {
  // Return early if string is empty
  if (s.empty()) {
    return std::string();
  }

  // Parse string through strftime
  std::array<char, max_log_file_name_length + 1> buffer;
  const auto now = std::time(nullptr);
  const auto status = std::strftime(
      &buffer[0], max_log_file_name_length + 1, s.c_str(),
      std::localtime(&now));

  // If there was an error, return empty string
  if (status == 0) {
    return std::string();
  }

  return std::string(&buffer[0]);
}


int main(int argc, char* argv[]) {
  // Parse command line arguments
  const auto args = parse_arguments(argc, argv);

  // Check if system clock uses Unix epoch, otherwise quit with an error
  {
    const auto epoch = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::time_point());
    std::array<char, 16> buffer;
    const auto status =
        std::strftime(&buffer[0], 16, "%Y%m%d%H%M%S", std::gmtime(&epoch));
    if (status == 0 || std::string(&buffer[0]) != "19700101000000") {
      std::cerr << "error: system clock does not use Unix epoch" << std::endl;
      std::exit(1);
    }
  }

  // Check if log file is specified and whether its name is time-encoded
  const bool use_log_file = (!args.log_file.empty());
  const bool has_time_in_log_file_name =
      (time_formatted(args.log_file) != args.log_file);

  // Set output stream to use
  std::ofstream log_file;
  std::string log_file_name;
  std::ostream os(use_log_file ? log_file.rdbuf() : std::cout.rdbuf());

  // Begin main loop
  const auto sleep_time = std::chrono::seconds(args.period);
  Int previous_steady = 0;
  for (Int iteration = 0;;) {
    // Obtain sample
    const Sample s = sample(args.network_interface, args.stat_path);

    // Check if log file needs to be (re-)opened
    if (use_log_file) {
      // Determine name for next log file
      const auto new_name =
          has_time_in_log_file_name
          ? time_formatted(args.log_file)
          : args.log_file;

      // An empty new name implies an error in the time_formatted function
      if (new_name.empty()) {
        std::cerr << "error: log file name after applying time format is too "
                  << "long (must be < " << max_log_file_name_length << ")"
                  << std::endl;
        std::exit(1);
      }

      // Check if log file needs to be (re-)opened
      if (new_name != log_file_name) {
        // Close log file if it was already open
        if (log_file.is_open()) {
          log_file.close();
          log_file.clear();
        }

        log_file_name = new_name;
        log_file.open(log_file_name, std::ios::out | std::ios::app);
        if (!log_file.good()) {
          std::cerr << "error: could not open log file '" << log_file_name
                    << "' for writing" << std::endl;
          std::exit(1);
        }
        std::cout << "Writing to '" << log_file_name << "'..." << std::endl;
      }
    }

    // Calculate time delta since last sample
    const Int time_delta = (iteration == 0) ? 0 : s.steady - previous_steady;

    // Print all data to stdout
    os << s.date
       << " " << time_delta
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
       << " " << s.disk_total
       << " " << s.disk_used
       << " " << s.disk_available
       << " " << s.network_received
       << " " << s.network_sent
       << std::endl;

    // Increment interation counter
    iteration++;

    // Store current value of steady clock for reference
    previous_steady = s.steady;

    // Exit main loop if maximum number of iterations is reached
    if (args.iterations > 0 && iteration >= args.iterations) {
      break;
    }

    // Sleep until next sample time
    const auto sleep_until = std::chrono::steady_clock::time_point(
        std::chrono::milliseconds(s.steady) + sleep_time);
    std::this_thread::sleep_until(sleep_until);
  }
}
