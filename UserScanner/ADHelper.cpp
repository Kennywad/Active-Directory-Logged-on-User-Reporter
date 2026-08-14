#include "ADHelper.h"
#include "LDAPQuery.h"
#include <windows.h>
#include <sddl.h>
#include <iostream>
#include <sstream>
#include <algorithm>

// Constructor
ADHelper::ADHelper(const std::wstring& domain, const std::wstring& domainController)
    : m_domain(domain)
    , m_domainController(domainController)
{
    m_baseDN = DomainToDN(domain);
    std::wcout << L"[INFO] Active Directory Helper initialized" << std::endl;
    std::wcout << L"  Domain: " << m_domain << std::endl;
    std::wcout << L"  DC: " << m_domainController << std::endl;
    std::wcout << L"  Base DN: " << m_baseDN << std::endl;
}

// Creates LDAP connection
std::unique_ptr<LDAPQuery> ADHelper::CreateLDAPConnection() {
    auto ldap = std::make_unique<LDAPQuery>(m_domainController);

    if (!ldap->Initialize()) {
        std::wcerr << L"[ERROR] LDAP initialization failed." << std::endl;
        return nullptr;
    }

    if (!ldap->Bind()) {
        std::wcerr << L"[ERROR] LDAP connection could not be established." << std::endl;
        return nullptr;
    }

    return ldap;
}

// Retrieves all computers
std::vector<ComputerInfo> ADHelper::GetAllComputers() {
    std::vector<ComputerInfo> computers;

    auto ldap = CreateLDAPConnection();
    if (!ldap) {
        return computers;
    }

    // Search for computer objects
    std::wstring filter = L"(&(objectClass=computer))";
    std::vector<std::wstring> attributes = { L"dNSHostName", L"operatingSystem" };

    if (!ldap->Search(m_baseDN, filter, attributes)) {
        std::wcerr << L"[ERROR] Computer search failed." << std::endl;
        return computers;
    }

    // Retrieve results
    auto hostnames = ldap->GetAttributeValues(L"dNSHostName");
    auto osSystems = ldap->GetAttributeValues(L"operatingSystem");

    // Combine information
    for (size_t i = 0; i < hostnames.size(); ++i) {
        ComputerInfo info;
        info.hostname = hostnames[i];
        info.computerName = ExtractComputerName(hostnames[i], m_domain);
        info.operatingSystem = (i < osSystems.size()) ? osSystems[i] : L"Unknown";

        computers.push_back(info);
    }

    std::wcout << L"[INFO] Total " << computers.size() << L" computers found." << std::endl;
    return computers;
}

// Retrieves the operating system of a specified computer
std::wstring ADHelper::GetComputerOS(const std::wstring& computerName) {
    auto ldap = CreateLDAPConnection();
    if (!ldap) {
        return L"";
    }

    // Search for the specific computer
    std::wstring filter = L"(&(objectClass=computer)(cn=" + computerName + L"))";
    std::vector<std::wstring> attributes = { L"operatingSystem" };

    if (!ldap->Search(m_baseDN, filter, attributes)) {
        return L"";
    }

    auto results = ldap->GetAttributeValues(L"operatingSystem");
    if (!results.empty()) {
        return results[0];
    }

    return L"";
}

// Lists active users on a computer
std::vector<UserInfo> ADHelper::GetLoggedOnUsers(const std::wstring& fullyQualifiedName) {
    std::vector<UserInfo> users;

    // Connect to remote computer's registry
    HKEY hRemoteKey = nullptr;
    LONG result = RegConnectRegistryW(fullyQualifiedName.c_str(), HKEY_USERS, &hRemoteKey);

    if (result != ERROR_SUCCESS) {
        // Silent failure - many computers may be inaccessible
        return users;
    }

    // Enumerate subkeys
    DWORD index = 0;
    wchar_t subKeyName[256];
    DWORD subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);

    while (RegEnumKeyExW(hRemoteKey, index, subKeyName, &subKeyNameSize,
        nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {

        // Convert SID to username
        PSID pSid = nullptr;
        if (ConvertStringSidToSidW(subKeyName, &pSid)) {
            wchar_t userName[256] = { 0 };
            DWORD userNameSize = sizeof(userName) / sizeof(wchar_t);
            wchar_t domainName[256] = { 0 };
            DWORD domainNameSize = sizeof(domainName) / sizeof(wchar_t);
            SID_NAME_USE sidType;

            if (LookupAccountSidW(fullyQualifiedName.c_str(), pSid,
                userName, &userNameSize,
                domainName, &domainNameSize,
                &sidType)) {

                UserInfo info(subKeyName, userName, domainName);
                users.push_back(info);
            }

            LocalFree(pSid);
        }

        // Prepare for next key
        index++;
        subKeyNameSize = sizeof(subKeyName) / sizeof(wchar_t);
    }

    RegCloseKey(hRemoteKey);
    return users;
}

// Converts domain name to LDAP DN format
std::wstring ADHelper::DomainToDN(const std::wstring& domain) {
    std::wstring dn;
    std::wstring currentPart;

    for (wchar_t ch : domain) {
        if (ch == L'.') {
            if (!currentPart.empty()) {
                if (!dn.empty()) {
                    dn += L",";
                }
                dn += L"DC=" + currentPart;
                currentPart.clear();
            }
        }
        else {
            currentPart += ch;
        }
    }

    // Add the last part
    if (!currentPart.empty()) {
        if (!dn.empty()) {
            dn += L",";
        }
        dn += L"DC=" + currentPart;
    }

    return dn;
}

// Extracts computer name from FQDN
std::wstring ADHelper::ExtractComputerName(const std::wstring& fqdn, const std::wstring& domain) {
    // Remove domain part
    size_t pos = fqdn.find(L"." + domain);
    if (pos != std::wstring::npos) {
        return fqdn.substr(0, pos);
    }

    // If domain not found, take part before first dot
    pos = fqdn.find(L'.');
    if (pos != std::wstring::npos) {
        return fqdn.substr(0, pos);
    }

    return fqdn;
}
