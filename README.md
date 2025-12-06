# Active Directory User Scanner

**Active Directory User Scanner** is a C++ application that scans all computers in a Windows domain and collects logged-on users. The output is saved in a CSV file compatible with Excel. The tool supports multithreading for faster scanning in large environments.

## Features

- Scan all computers in an Active Directory domain
- List logged-on users for each computer
- Save results as UTF-8 BOM CSV (Excel compatible)
- Multithreading support for faster processing
- Registry-based SID-to-username resolution
- Error handling and reporting


## Requirements

- Windows operating system
- Visual Studio 2022
- Windows SDK
- `wldap32.lib` (for LDAP operations)


## File Structure

| File | Description |
|------|-------------|
| `main.cpp` | Entry point of the program. Handles command-line arguments and starts the AD scanning process. |
| `ADHelper.h/.cpp` | Helper class for simplifying Active Directory operations, including fetching computers and users. |
| `LDAPQuery.h/.cpp` | Low-level LDAP query class. Manages LDAP connection, bind, and search operations. |
| `ReportGenerator.h/.cpp` | Responsible for generating the CSV report, writing data, and tracking statistics. |


## Compilation

- Download the project to your computer.
- Extract the Project to a Folder.
- Download Visual Studio to your computer
- Open the solution file (.sln).
- Select **Build Solution** from the **Build** menu.

## Usage

Run the program from the command line:

```bash
program.exe -d <domain> -dc <domain_controller> -o <output_file> [options]
````

### Required Parameters

* `-d, --domain` → Domain name (e.g., `example.com`)
* `-dc, --dc` → Domain Controller address (e.g., `dc.example.com`)
* `-o, --output` → Output CSV file (e.g., `report.csv`)

### Optional Parameters

* `-t, --threads` → Number of threads (default: 100, maximum: 500)
* `-h, --help` → Show help message

### Example

```bash
program.exe -d example.com -dc dc.example.com -o report.csv -t 50
```


## Classes and Functions

### `ReportGenerator`

**Purpose:** Writes scanning results to a CSV file and keeps statistics.

**Key Functions:**

* `Initialize()` → Creates the CSV file and writes the header row
* `AddEntry(const ComputerInfo&, const std::vector<UserInfo>&)` → Adds computer and user information (thread-safe)
* `Close()` → Closes the CSV file and prints a summary
* `GetTotalComputers()`, `GetComputersWithUsers()`, `GetTotalUsers()` → Retrieve statistics

**Helper Functions:**

* `WStringToUTF8` → Converts `std::wstring` to UTF-8 `std::string`
* `EscapeCSV` → Escapes commas, quotes, and newlines for CSV compatibility

### `LDAPQuery`

**Purpose:** Handles low-level LDAP operations using Windows LDAP API.

**Key Functions:**

* `Initialize()` → Initialize LDAP connection
* `Bind()` → Perform LDAP bind using negotiated authentication
* `Search(baseDN, filter, attributes)` → Execute an LDAP search
* `GetAttributeValues(attributeName)` → Retrieve values for a specific attribute
* `GetLastError()` → Retrieve the last LDAP error


### `ADHelper`

**Purpose:** Simplifies LDAP queries and Active Directory operations.

**Key Functions:**

* `GetAllComputers()` → Retrieve all computers in the domain
* `GetComputerOS(computerName)` → Retrieve the operating system of a computer
* `GetLoggedOnUsers(fqdn)` → Retrieve users logged in on a specific computer
* `DomainToDN(domain)` → Convert a domain name to LDAP DN format
* `ExtractComputerName(fqdn, domain)` → Extract the computer name from its FQDN


### `main.cpp` Workflow

1. Parse command-line arguments (`ParseArguments`)
2. Initialize `ADHelper` and `ReportGenerator`
3. Retrieve all computers from Active Directory (`GetAllComputers`)
4. Divide computers among threads for multithreaded scanning (`ProcessComputerBatch`)
5. Add logged-on users to CSV file
6. Print a summary when scanning is complete


## Example CSV Output

```csv
Computer,OS,SID,Logon User
PC1,Windows 10,, 
PC2,Windows Server 2019,S-1-5-21-1234567890-1234567890-1234567890-1001,"DOMAIN\User1"
PC2,Windows Server 2019,S-1-5-21-1234567890-1234567890-1234567890-1002,"DOMAIN\User2"
```

* Computers without users only display computer info
* Each user gets a separate row


## Multithreading

* Uses `std::async` and `std::future`
* Computer list is divided across threads
* Each thread processes a batch of computers
* `ReportGenerator::AddEntry` ensures thread-safe writes to CSV


## Error Handling

* LDAP and registry errors are logged
* Inaccessible computers are skipped silently
* Error messages are written to `std::wcerr`


## License

This project is licensed under the MIT License. For more information, see the [LICENSE file](LICENSE).