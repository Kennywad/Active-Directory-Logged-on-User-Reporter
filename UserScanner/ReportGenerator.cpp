#include "ReportGenerator.h"
#include <iostream>
#include <codecvt>
#include <locale>
#include <windows.h>

// Constructor
ReportGenerator::ReportGenerator(const std::wstring& outputPath)
    : m_outputPath(outputPath)
    , m_totalComputers(0)
    , m_computersWithUsers(0)
    , m_totalUsers(0)
{
}

// Destructor
ReportGenerator::~ReportGenerator() {
    Close();
}

// Initializes the report file
bool ReportGenerator::Initialize() {
    // Open file in UTF-8 mode
    m_outputFile.open(m_outputPath, std::ios::out | std::ios::trunc);

    if (!m_outputFile.is_open()) {
        std::wcerr << L"[ERROR] Could not open output file: " << m_outputPath << std::endl;
        return false;
    }

    // Add UTF-8 BOM (required for Excel)
    m_outputFile << "\xEF\xBB\xBF";

    // CSV header row
    m_outputFile << "Computer,OS,SID,Logon User\n";
    m_outputFile.flush();

    std::wcout << L"[INFO] Report file created: " << m_outputPath << std::endl;
    return true;
}

// Adds computer and user information to the report
void ReportGenerator::AddEntry(const ComputerInfo& computer, const std::vector<UserInfo>& users) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_totalComputers++;

    if (users.empty()) {
        // If no users, write only computer info
        std::wstring line = computer.hostname + L"," +
            EscapeCSV(computer.operatingSystem) + L"," +
            L"," +
            L"\n";

        m_outputFile << WStringToUTF8(line);
    }
    else {
        // Each user gets a separate row
        m_computersWithUsers++;

        for (const auto& user : users) {
            m_totalUsers++;

            std::wstring line = computer.hostname + L"," +
                EscapeCSV(computer.operatingSystem) + L"," +
                user.sid + L"," +
                L"\"" + user.GetFullName() + L"\"" +
                L"\n";

            m_outputFile << WStringToUTF8(line);
        }
    }

    m_outputFile.flush();
}

// Closes the report file
void ReportGenerator::Close() {
    if (m_outputFile.is_open()) {
        m_outputFile.close();

        std::wcout << L"\n=== REPORT SUMMARY ===" << std::endl;
        std::wcout << L"Total computers: " << m_totalComputers << std::endl;
        std::wcout << L"Computers with users: " << m_computersWithUsers << std::endl;
        std::wcout << L"Total logons: " << m_totalUsers << std::endl;
        std::wcout << L"Report file: " << m_outputPath << std::endl;
    }
}

// Converts std::wstring to UTF-8 std::string
std::string ReportGenerator::WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }

    // Use Windows API for conversion
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
        nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) {
        return std::string();
    }

    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
        &result[0], sizeNeeded, nullptr, nullptr);

    // Remove trailing null character
    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }

    return result;
}

// Escapes a string for CSV
std::wstring ReportGenerator::EscapeCSV(const std::wstring& str) {
    // If contains comma, quote, or newline, wrap in quotes and double the quotes
    if (str.find(L',') != std::wstring::npos ||
        str.find(L'"') != std::wstring::npos ||
        str.find(L'\n') != std::wstring::npos) {

        std::wstring escaped = L"\"";
        for (wchar_t ch : str) {
            if (ch == L'"') {
                escaped += L"\"\""; // Double the quote
            }
            else {
                escaped += ch;
            }
        }
        escaped += L"\"";
        return escaped;
    }

    return str;
}
