#include "LDAPQuery.h"
#include <iostream>
#include <sstream>

#pragma comment(lib, "wldap32.lib")

LDAPQuery::LDAPQuery(const std::wstring& domainController)
    : m_ldap(nullptr)
    , m_searchResult(nullptr)
    , m_domainController(domainController)
    , m_lastErrorCode(0)
{
}

LDAPQuery::~LDAPQuery() {
    ClearSearchResults();
    if (m_ldap) {
        ldap_unbind_s(m_ldap);
        m_ldap = nullptr;
    }
}

bool LDAPQuery::Initialize() {
    if (m_ldap) {
        ldap_unbind_s(m_ldap);
        m_ldap = nullptr;
    }

    PWSTR dcString = AllocateWideString(m_domainController);
    if (!dcString) {
        std::wcerr << L"[ERROR] Memory allocation failed." << std::endl;
        return false;
    }

    m_ldap = ldap_initW(dcString, GetLDAPPort());
    delete[] dcString;

    if (!m_ldap) {
        std::wcerr << L"[ERROR] LDAP initialization failed: " << m_domainController << std::endl;
        return false;
    }

    int version = GetLDAPVersion();
    m_lastErrorCode = ldap_set_option(m_ldap, LDAP_OPT_PROTOCOL_VERSION, &version);

    if (m_lastErrorCode != LDAP_SUCCESS) {
        std::wcerr << L"[ERROR] Could not set LDAP options. Error code: "
            << m_lastErrorCode << std::endl;
        ldap_unbind_s(m_ldap);
        m_ldap = nullptr;
        return false;
    }

    return true;
}

bool LDAPQuery::Bind() {
    if (!m_ldap) {
        std::wcerr << L"[ERROR] LDAP not initialized." << std::endl;
        return false;
    }

    m_lastErrorCode = ldap_bind_sW(m_ldap, nullptr, nullptr, LDAP_AUTH_NEGOTIATE);

    if (m_lastErrorCode != LDAP_SUCCESS) {
        std::wcerr << L"[ERROR] LDAP bind failed. Error code: "
            << m_lastErrorCode << std::endl;
        return false;
    }

    std::wcout << L"[INFO] LDAP bind successful: " << m_domainController << std::endl;
    return true;
}

bool LDAPQuery::Search(const std::wstring& baseDN,
    const std::wstring& filter,
    const std::vector<std::wstring>& attributes) {
    if (!m_ldap) {
        std::wcerr << L"[ERROR] LDAP connection not available." << std::endl;
        return false;
    }

    ClearSearchResults();

    PWSTR baseDNStr = AllocateWideString(baseDN);
    PWSTR filterStr = AllocateWideString(filter);

    if (!baseDNStr || !filterStr) {
        delete[] baseDNStr;
        delete[] filterStr;
        std::wcerr << L"[ERROR] String conversion failed." << std::endl;
        return false;
    }

    std::vector<PWSTR> attrArray;
    std::vector<PWSTR> allocatedStrings;

    for (const auto& attr : attributes) {
        PWSTR attrStr = AllocateWideString(attr);
        if (attrStr) {
            attrArray.push_back(attrStr);
            allocatedStrings.push_back(attrStr);
        }
    }
    attrArray.push_back(nullptr);

    m_lastErrorCode = ldap_search_sW(
        m_ldap,
        baseDNStr,
        LDAP_SCOPE_SUBTREE,
        filterStr,
        attrArray.data(),
        0,
        &m_searchResult
    );

    delete[] baseDNStr;
    delete[] filterStr;
    for (PWSTR str : allocatedStrings) {
        delete[] str;
    }

    if (m_lastErrorCode != LDAP_SUCCESS) {
        std::wcerr << L"[ERROR] LDAP search failed. Error code: "
            << m_lastErrorCode << std::endl;
        std::wcerr << L"  BaseDN: " << baseDN << std::endl;
        std::wcerr << L"  Filter: " << filter << std::endl;
        return false;
    }

    int count = ldap_count_entries(m_ldap, m_searchResult);
    std::wcout << L"[INFO] " << count << L" results found." << std::endl;

    return true;
}

std::vector<std::wstring> LDAPQuery::GetAttributeValues(const std::wstring& attributeName) {
    std::vector<std::wstring> results;

    if (!m_ldap || !m_searchResult) {
        std::wcerr << L"[WARNING] No search results available." << std::endl;
        return results;
    }

    PWSTR attrName = AllocateWideString(attributeName);
    if (!attrName) {
        return results;
    }

    for (LDAPMessage* entry = ldap_first_entry(m_ldap, m_searchResult);
        entry != nullptr;
        entry = ldap_next_entry(m_ldap, entry)) {

        PWSTR* values = ldap_get_valuesW(m_ldap, entry, attrName);

        if (values) {
            for (int i = 0; values[i] != nullptr; ++i) {
                results.push_back(values[i]);
            }
            ldap_value_freeW(values);
        }
    }

    delete[] attrName;
    return results;
}

std::wstring LDAPQuery::GetLastError() const {
    if (m_lastErrorCode == LDAP_SUCCESS) {
        return L"No error";
    }

    PWSTR errorMsg = ldap_err2stringW(m_lastErrorCode);
    if (errorMsg) {
        return std::wstring(errorMsg);
    }

    std::wstringstream ss;
    ss << L"Unknown error (code: " << m_lastErrorCode << L")";
    return ss.str();
}

void LDAPQuery::ClearSearchResults() {
    if (m_searchResult) {
        ldap_msgfree(m_searchResult);
        m_searchResult = nullptr;
    }
}

PWSTR LDAPQuery::AllocateWideString(const std::wstring& str) {
    if (str.empty()) {
        return nullptr;
    }

    size_t length = str.length() + 1;
    PWSTR buffer = new (std::nothrow) wchar_t[length];

    if (buffer) {
        wcscpy_s(buffer, length, str.c_str());
    }

    return buffer;
}
