#ifndef LDAPQUERY_H
#define LDAPQUERY_H

#include <windows.h>
#include <winldap.h>
#include <string>
#include <vector>
#include <memory>

class LDAPQuery {
public:
    explicit LDAPQuery(const std::wstring& domainController);
    ~LDAPQuery();

    LDAPQuery(const LDAPQuery&) = delete;
    LDAPQuery& operator=(const LDAPQuery&) = delete;
    LDAPQuery(LDAPQuery&&) = delete;
    LDAPQuery& operator=(LDAPQuery&&) = delete;

    bool Initialize();
    bool Bind();
    bool Search(const std::wstring& baseDN,
        const std::wstring& filter,
        const std::vector<std::wstring>& attributes);

    std::vector<std::wstring> GetAttributeValues(const std::wstring& attributeName);
    std::wstring GetLastError() const;

private:
    LDAP* m_ldap;
    LDAPMessage* m_searchResult;
    std::wstring m_domainController;
    ULONG m_lastErrorCode;

    void ClearSearchResults();
    PWSTR AllocateWideString(const std::wstring& str);

    static int GetLDAPVersion() { return LDAP_VERSION3; }
    static int GetLDAPPort() { return LDAP_PORT; }
};

#endif // LDAPQUERY_H
