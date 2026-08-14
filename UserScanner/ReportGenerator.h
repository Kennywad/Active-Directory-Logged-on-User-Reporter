#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include "ADHelper.h"
#include <string>
#include <fstream>
#include <mutex>

/**
 * @class ReportGenerator
 * @brief Creates and manages a CSV report
 */
class ReportGenerator {
public:
    /**
     * @brief Constructs the report generator
     * @param outputPath Path of the output file
     */
    explicit ReportGenerator(const std::wstring& outputPath);

    ~ReportGenerator();

    /**
     * @brief Opens the report file and writes the header row
     * @return True if successful
     */
    bool Initialize();

    /**
     * @brief Adds computer and user information to the report (thread-safe)
     * @param computer Computer information
     * @param users List of users
     */
    void AddEntry(const ComputerInfo& computer, const std::vector<UserInfo>& users);

    /**
     * @brief Returns statistics
     */
    int GetTotalComputers() const { return m_totalComputers; }
    int GetComputersWithUsers() const { return m_computersWithUsers; }
    int GetTotalUsers() const { return m_totalUsers; }

    /**
     * @brief Closes the report file
     */
    void Close();

private:
    std::wstring m_outputPath;      // Output file path
    std::ofstream m_outputFile;     // Output file stream
    std::mutex m_mutex;             // Mutex for thread safety
    int m_totalComputers;           // Total number of computers
    int m_computersWithUsers;       // Number of computers with users
    int m_totalUsers;               // Total number of users

    /**
     * @brief Converts a wstring to a UTF-8 string
     * @param wstr Wide-character string
     * @return UTF-8 encoded string
     */
    static std::string WStringToUTF8(const std::wstring& wstr);

    /**
     * @brief Escapes a string for CSV (handles commas and quotes)
     * @param str Input string
     * @return Escaped string
     */
    static std::wstring EscapeCSV(const std::wstring& str);
};

#endif // REPORTGENERATOR_H
