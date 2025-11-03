#include <windows.h>
#include <iostream>
#include <string>

// ------------------------------
// ServerMS: basic Mailslot server
// Creates local mailslot \\.\mailslot\Box, waits for a single
// incoming message and prints it to the console.
// ------------------------------

// Error helper for Mailslot/WinAPI operations
// operation — a short name of the API call for readable logs
void HandleMailslotError(const char* operation) {
    DWORD error = GetLastError();                  // Last error code in current thread
    std::cerr << "Error in operation '" << operation << "': ";
    
    // Decode common error codes for clarity
    switch (error) {
        case ERROR_INVALID_PARAMETER:
            std::cerr << "Invalid parameter"; break;
        case ERROR_ALREADY_EXISTS:
            std::cerr << "Mailslot already exists"; break;
        case ERROR_PIPE_BUSY:
            std::cerr << "Mailslot is busy"; break;
        case ERROR_BROKEN_PIPE:
            std::cerr << "Broken pipe"; break;
        case ERROR_TIMEOUT:
            std::cerr << "Timeout"; break;
        case ERROR_INSUFFICIENT_BUFFER:
            std::cerr << "Insufficient buffer"; break;
        default:
            std::cerr << "Error code: " << error; break;
    }
    std::cerr << std::endl;
}

int main() {
    SetConsoleCP(1251);                            // Console input code page: Windows-1251
    SetConsoleOutputCP(1251);                      // Console output code page: Windows-1251

    // Block 1: Create Mailslot
    // Local name format — \\.\mailslot\<Name>
    // Here <Name> = "Box". The mailslot is visible on the current machine.
    LPCWSTR mailslotName = L"\\\\.\\mailslot\\Box"; // Wide string (UNICODE)
    HANDLE hMailslot = NULL;                       // Server handle
    
    // CreateMailslot creates a server (receiving) endpoint.
    // Parameters:
    // 1) lpName        = mailslotName (LPCWSTR) — full mailslot name
    // 2) nMaxMessage   = 300 — maximum incoming message size in bytes
    // 3) lReadTimeout  = 180000 — ReadFile wait timeout in ms (3 minutes)
    //                     (MAILSLOT_WAIT_FOREVER == 0 means infinite wait)
    // 4) lpSecurity    = NULL — default security attributes
    hMailslot = CreateMailslot(
        mailslotName,      // Full local mailslot name
        300,               // Max message size (bytes)
        180000,            // Read timeout: 3 minutes (180000 ms)
        NULL               // Default security
    );
    
    if (hMailslot == INVALID_HANDLE_VALUE) {       // Check creation result
        HandleMailslotError("CreateMailslot");
        return 1;
    }
    
    std::cout << "Mailslot created" << std::endl;
    std::cout << "Waiting for client message..." << std::endl;
    
    // Block 2: Read client message
    // ReadFile blocks until data arrives or until the timeout set in CreateMailslot.
    char buffer[512];                               // Read buffer (with margin above 300)
    DWORD bytesRead = 0;                            // Actual bytes read
    BOOL readResult = FALSE;                        // ReadFile result
    
    // ReadFile parameters:
    // 1) hFile        = hMailslot — mailslot handle
    // 2) lpBuffer     = buffer — destination buffer
    // 3) nNumberOfBytesToRead = sizeof(buffer) - 1 — available buffer size
    // 4) lpNumberOfBytesRead  = &bytesRead — actual read size
    // 5) lpOverlapped = NULL — synchronous I/O
    readResult = ReadFile(
        hMailslot,        // Mailslot handle
        buffer,           // Buffer
        sizeof(buffer) - 1, // Reserve 1 byte for trailing '\0'
        &bytesRead,       // Bytes read
        NULL              // Synchronous
    );
    
    if (readResult == FALSE) {                      // Handle read errors
        DWORD error = GetLastError();
        if (error == ERROR_TIMEOUT) {               // Special case: read timeout
            std::cout << "Message wait timeout (3 minutes)" << std::endl;
        } else {
            HandleMailslotError("ReadFile");
        }
        CloseHandle(hMailslot);                     // Free resource
        return 1;
    }
    
    // Block 3: Process the message
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';                  // Ensure C-string termination
        std::cout << "Received message (" << bytesRead << " bytes):" << std::endl;
        std::cout << buffer << std::endl;          // Print payload
    } else {
        std::cout << "Empty message received" << std::endl;
    }
    
    CloseHandle(hMailslot);                         // Close server handle
    std::cout << "Server shutting down." << std::endl;
    
    return 0;                                       // Success
}

