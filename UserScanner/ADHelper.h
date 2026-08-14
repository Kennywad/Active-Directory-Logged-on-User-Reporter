#ifndef ADHELPER_H
#define ADHELPER_H

#include <string>
#include <vector>
#include <memory>

// Forward declaration
class LDAPQuery;

/**
 * @struct ComputerInfo
 * @brief Holds information about a computer
 */
struct ComputerInfo {
    std::wstring hostname;          // Fully Qualified Computer Name (FQDN)
    std::wstring computerName;      // Short computer name
    std::wstring operatingSystem;   // Operating system

    ComputerInfo() = default;
    ComputerInfo(const std::wstring& host, const std::wstring& name, const std::wstring& os)
        : hostname(host), computerName(name), operatingSystem(os) {
    }
};

/**
 * @struct UserInfo
 * @brief Holds user session information
 */
struct UserInfo {
    std::wstring sid;           // Security Identifier
    std::wstring userName;      // User name
    std::wstring domainName;    // Domain name

    UserInfo() = default;
    UserInfo(const std::wstring& s, const std::wstring& user, const std::wstring& domain)
        : sid(s), userName(user), domainName(domain) {
    }

    // Full user name (DOMAIN\Username)
    std::wstring GetFullName() const {
        return domainName + L"\\" + userName;
    }
};

/**
 * @class ADHelper
 * @brief Helper class for simplifying Active Directory operations
 */
class ADHelper {
public:
    /**
     * @brief Constructs the helper class with domain information
     * @param domain Domain name (e.g., "example.com")
     * @param domainController Domain controller address (e.g., "dc.example.com")
     */
    ADHelper(const std::wstring& domain, const std::wstring& domainController);

    ~ADHelper() = default;

    /**
     * @brief Retrieves all computers in the domain
     * @return List of computer information
     */
    std::vector<ComputerInfo> GetAllComputers();

    /**
     * @brief Retrieves the operating system of the specified computer
     * @param computerName Computer name
     * @return Operating system information
     */
    std::wstring GetComputerOS(const std::wstring& computerName);

    /**
     * @brief Lists active users on a computer (via registry)
     * @param fullyQualifiedName Fully Qualified Computer Name (FQDN)
     * @return List of user information
     */
    std::vector<UserInfo> GetLoggedOnUsers(const std::wstring& fullyQualifiedName);

    /**
     * @brief Converts a domain name to LDAP DN format
     * @param domain Domain name (e.g., "example.com")
     * @return LDAP DN (e.g., "DC=example,DC=com")
     */
    static std::wstring DomainToDN(const std::wstring& domain);

    /**
     * @brief Extracts the computer name from a FQDN
     * @param fqdn Fully Qualified Computer Name (e.g., "PC1.example.com")
     * @param domain Domain name (e.g., "example.com")
     * @return Computer name (e.g., "PC1")
     */
    static std::wstring ExtractComputerName(const std::wstring& fqdn, const std::wstring& domain);

private:
    std::wstring m_domain;              // Domain name
    std::wstring m_domainController;    // Domain controller
    std::wstring m_baseDN;              // LDAP base DN

    /**
     * @brief Creates a connection for LDAP queries
     * @return LDAPQuery object (nullptr if failed)
     */
    std::unique_ptr<LDAPQuery> CreateLDAPConnection();
};

#endif // ADHELPER_H
