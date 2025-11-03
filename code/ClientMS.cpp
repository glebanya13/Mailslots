#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

// ------------------------------
// ClientMS: simple Mailslot client
// Opens the server mailslot (as a file) and writes a message to it.
// Supports local and remote servers and multi-target broadcast.
// ------------------------------

// Unified error printing for Mailslot/file operations
void HandleMailslotError(const char* operation) {
    DWORD error = GetLastError();
    std::cerr << "Error in operation '" << operation << "': ";
    switch (error) {
        case ERROR_INVALID_PARAMETER:  std::cerr << "Invalid parameter"; break;
        case ERROR_FILE_NOT_FOUND:     std::cerr << "Mailslot not found"; break;
        case ERROR_PATH_NOT_FOUND:     std::cerr << "Path not found"; break;
        case ERROR_PIPE_BUSY:          std::cerr << "Mailslot is busy"; break;
        case ERROR_BROKEN_PIPE:        std::cerr << "Broken pipe"; break;
        case ERROR_TIMEOUT:            std::cerr << "Timeout"; break;
        case ERROR_INSUFFICIENT_BUFFER:std::cerr << "Insufficient buffer"; break;
        default:                       std::cerr << "Error code: " << error; break;
    }
    std::cerr << std::endl;
}

// Utility: ANSI (Windows-1251) -> UTF‑16
// Required to build a proper wide-string for CreateFileW
static std::wstring to_wstring_from_ansi(const std::string &s) {
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(1251, 0, s.c_str(), (int)s.size(), NULL, 0); // compute length
    std::wstring ws; ws.resize(len);
    MultiByteToWideChar(1251, 0, s.c_str(), (int)s.size(), &ws[0], len);       // convert
    return ws;
}

// Send a single message to a specific server mailslot
// mailslotName — full path like \\.\mailslot\Box or \\HOST\mailslot\Box (wide)
// message      — pointer to payload bytes (server just receives bytes)
bool SendMessageToServer(const std::wstring& mailslotName, const char* message) {
    // CreateFileW opens the server mailslot for writing
    // Parameters:
    // 1) lpFileName   = mailslotName.c_str() — full mailslot path (LPCWSTR)
    // 2) dwDesiredAccess = GENERIC_WRITE — write-only access
    // 3) dwShareMode  = FILE_SHARE_READ — allow shared read
    // 4) lpSecurity   = NULL — default security
    // 5) dwCreationDisposition = OPEN_EXISTING — mailslot must already exist
    // 6) dwFlagsAndAttributes = 0 — no special flags
    // 7) hTemplateFile = NULL — not used
    HANDLE hMailslot = CreateFile(
        mailslotName.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );
    
    if (hMailslot == INVALID_HANDLE_VALUE) {       // If server is not running — ERROR_FILE_NOT_FOUND
        HandleMailslotError("CreateFile");
        std::cerr << "Failed to open Mailslot" << std::endl;
        return false;
    }
    
    // Write payload
    DWORD bytesWritten = 0;
    BOOL writeResult = WriteFile(
        hMailslot,                             // mailslot handle
        message,                               // data pointer
        (DWORD)strlen(message),                // data length (bytes)
        &bytesWritten,                         // actually written
        NULL                                   // synchronous I/O
    );
    
    if (writeResult == FALSE) {                 // Write error
        HandleMailslotError("WriteFile");
        CloseHandle(hMailslot);
        return false;
    }
    
    CloseHandle(hMailslot);                     // Release handle
    std::cout << "Message sent (" << bytesWritten << " bytes): " << message << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    SetConsoleCP(1251);                         // Console input in 1251
    SetConsoleOutputCP(1251);                   // Console output in 1251

    // Build target list. Supported forms:
    // - no args: local server \\.\mailslot\Box
    // - one/more args: machine names for \\HOST\mailslot\Box
    const char* message = "Hello from Maislot-client"; // Payload string
    std::vector<std::wstring> servers;                  // Targets
    
    if (argc > 1) {
        // Arguments present: each becomes a separate target
        for (int i = 1; i < argc; i++) {
            std::string computerNameA = argv[i];        // Host in ANSI
            if (computerNameA == "." || computerNameA == "localhost") {
                servers.push_back(L"\\\\.\\mailslot\\Box"); // Local format
            } else {
                std::wstring computerName = to_wstring_from_ansi(computerNameA); // Convert name
                servers.push_back(L"\\\\" + computerName + L"\\mailslot\\Box"); // Remote format
            }
        }
    } else {
        // No args — use local server
        servers.push_back(L"\\\\.\\mailslot\\Box");
    }
    
    // Send to one or multiple servers
    int successCount = 0;
    for (size_t i = 0; i < servers.size(); i++) {
        std::wcout << L"[" << (i + 1) << L"] Sending attempt" << std::endl; // Wide log line
        if (SendMessageToServer(servers[i], message)) {
            successCount++;
        } else {
            std::wcout << L"Failed to send" << std::endl;
            std::cout << "Make sure ServerMS is running." << std::endl;
        }
    }
    
    std::cout << "\nResult: sent successfully to " << successCount << " of " << servers.size() << " server(s)" << std::endl;
    std::cout << "Client is exiting." << std::endl;
    return (successCount == servers.size()) ? 0 : 1;    // Exit code: 0 — all OK, 1 — partial/failed
}

