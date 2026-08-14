#include "ADHelper.h"
#include "ReportGenerator.h"
#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <algorithm>
#include <windows.h>

/**
 * @brief Shows the help message
 */
void ShowHelp() {
    std::wcout << L"\n=== Active Directory User Report ===" << std::endl;
    std::wcout << L"\nUsage:" << std::endl;
    std::wcout << L"  program.exe -d <domain> -dc <domain_controller> -o <output_file> [options]" << std::endl;
    std::wcout << L"\nRequired Parameters:" << std::endl;
    std::wcout << L"  -d,  --domain      Domain name (e.g., example.com)" << std::endl;
    std::wcout << L"  -dc, --dc          Domain controller address (e.g., dc.example.com)" << std::endl;
    std::wcout << L"  -o,  --output      Output CSV file (e.g., report.csv)" << std::endl;
    std::wcout << L"\nOptional Parameters:" << std::endl;
    std::wcout << L"  -t,  --threads     Number of threads (default: 100)" << std::endl;
    std::wcout << L"  -h,  --help        Show this help message" << std::endl;
    std::wcout << L"\nExample:" << std::endl;
    std::wcout << L"  program.exe -d example.com -dc dc.example.com -o report.csv -t 50" << std::endl;
    std::wcout << L"\nNote: The program scans all computers in the domain and saves" << std::endl;
    std::wcout << L"logged-on users in CSV format." << std::endl;
}

/**
 * @brief Parses command-line arguments
 */
struct CommandLineArgs {
    std::wstring domain;
    std::wstring domainController;
    std::wstring outputFile;
    int threadCount = 100;
    bool valid = false;
};

CommandLineArgs ParseArguments(int argc, char* argv[]) {
    CommandLineArgs args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            ShowHelp();
            return args; // valid=false
        }
        else if ((arg == "-d" || arg == "--domain") && i + 1 < argc) {
            std::string value = argv[++i];
            args.domain = std::wstring(value.begin(), value.end());
        }
        else if ((arg == "-dc" || arg == "--dc") && i + 1 < argc) {
            std::string value = argv[++i];
            args.domainController = std::wstring(value.begin(), value.end());
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            std::string value = argv[++i];
            args.outputFile = std::wstring(value.begin(), value.end());
        }
        else if ((arg == "-t" || arg == "--threads") && i + 1 < argc) {
            try {
                args.threadCount = std::stoi(argv[++i]);
                if (args.threadCount < 1) {
                    args.threadCount = 1;
                }
                else if (args.threadCount > 500) {
                    std::wcout << L"[WARNING] Thread count limited to 500." << std::endl;
                    args.threadCount = 500;
                }
            }
            catch (...) {
                std::wcerr << L"[ERROR] Invalid thread count." << std::endl;
                return args;
            }
        }
        else {
            std::wcerr << L"[ERROR] Unknown parameter: " << std::wstring(arg.begin(), arg.end()) << std::endl;
            return args;
        }
    }

    // Check required parameters
    if (args.domain.empty()) {
        std::wcerr << L"[ERROR] Domain name not specified (-d or --domain)" << std::endl;
        return args;
    }
    if (args.domainController.empty()) {
        std::wcerr << L"[ERROR] Domain controller not specified (-dc or --dc)" << std::endl;
        return args;
    }
    if (args.outputFile.empty()) {
        std::wcerr << L"[ERROR] Output file not specified (-o or --output)" << std::endl;
        return args;
    }

    args.valid = true;
    return args;
}

/**
 * @brief Processes a batch of computers
 */
void ProcessComputerBatch(ADHelper& adHelper,
    ReportGenerator& reportGen,
    const std::vector<ComputerInfo>& computers,
    size_t startIdx,
    size_t endIdx) {
    for (size_t i = startIdx; i < endIdx; ++i) {
        const auto& computer = computers[i];

        // Get user information
        auto users = adHelper.GetLoggedOnUsers(computer.hostname);

        // Add to report
        reportGen.AddEntry(computer, users);

        // Show progress
        if ((i - startIdx + 1) % 10 == 0 || i == endIdx - 1) {
            std::wcout << L"[Thread " << std::this_thread::get_id() << L"] "
                << (i - startIdx + 1) << L"/" << (endIdx - startIdx)
                << L" computers processed" << std::endl;
        }
    }
}

/**
 * @brief Main program
 */
int main(int argc, char* argv[]) {
    // Windows console UTF-8 support
    SetConsoleOutputCP(CP_UTF8);
    std::wcout.imbue(std::locale(""));

    std::wcout << L"\n??????????????????????????????????????????????" << std::endl;
    std::wcout << L"?  Active Directory User Scanner            ?" << std::endl;
    std::wcout << L"??????????????????????????????????????????????\n" << std::endl;

    // Parse command-line arguments
    auto args = ParseArguments(argc, argv);
    if (!args.valid) {
        ShowHelp();
        return 1;
    }

    std::wcout << L"[INFO] Settings:" << std::endl;
    std::wcout << L"  Domain: " << args.domain << std::endl;
    std::wcout << L"  DC: " << args.domainController << std::endl;
    std::wcout << L"  Output: " << args.outputFile << std::endl;
    std::wcout << L"  Thread Count: " << args.threadCount << std::endl;
    std::wcout << std::endl;

    try {
        // Initialize Active Directory helper
        ADHelper adHelper(args.domain, args.domainController);

        // Initialize report generator
        ReportGenerator reportGen(args.outputFile);
        if (!reportGen.Initialize()) {
            return 1;
        }

        // Get all computers
        std::wcout << L"[INFO] Scanning computers..." << std::endl;
        auto computers = adHelper.GetAllComputers();

        if (computers.empty()) {
            std::wcout << L"[WARNING] No computers found." << std::endl;
            return 0;
        }

        std::wcout << L"\n[INFO] Collecting user information..." << std::endl;

        // Divide computers among threads
        size_t totalComputers = computers.size();
        size_t computersPerThread = (totalComputers + args.threadCount - 1) / args.threadCount;

        std::vector<std::future<void>> futures;

        for (int t = 0; t < args.threadCount; ++t) {
            size_t startIdx = t * computersPerThread;
            size_t endIdx = std::min<size_t>(startIdx + computersPerThread, totalComputers);

            if (startIdx >= totalComputers) {
                break;
            }

            // Launch thread
            futures.push_back(std::async(std::launch::async,
                ProcessComputerBatch,
                std::ref(adHelper),
                std::ref(reportGen),
                std::cref(computers),
                startIdx,
                endIdx));
        }

        // Wait for all threads to finish
        for (auto& future : futures) {
            future.wait();
        }

        std::wcout << L"\n[SUCCESS] Operation completed!" << std::endl;
        reportGen.Close();

    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::wcerr << L"[ERROR] An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}
