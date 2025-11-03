#include <windows.h>
#include <iostream>
#include <string>

// ------------------------------
// ServerMS_500bytes: Mailslot server for the 500-byte message size variant
// Same as the basic server, but configured to accept up to 500 bytes.
// ------------------------------

// Error helper for readable console output
void HandleMailslotError(const char* operation) {
    DWORD error = GetLastError();
    std::cerr << "Error in operation '" << operation << "': ";
    switch (error) {
        case ERROR_INVALID_PARAMETER:  std::cerr << "Invalid parameter"; break;
        case ERROR_ALREADY_EXISTS:     std::cerr << "Mailslot already exists"; break;
        case ERROR_PIPE_BUSY:          std::cerr << "Mailslot is busy"; break;
        case ERROR_BROKEN_PIPE:        std::cerr << "Broken pipe"; break;
        case ERROR_TIMEOUT:            std::cerr << "Timeout"; break;
        case ERROR_INSUFFICIENT_BUFFER:std::cerr << "Insufficient buffer"; break;
        default:                       std::cerr << "Error code: " << error; break;
    }
    std::cerr << std::endl;
}

int main() {
    SetConsoleCP(1251);               // Console input (Windows-1251)
    SetConsoleOutputCP(1251);         // Console output (Windows-1251)

    // Block 1: Create Mailslot
    // Local name: \\.\mailslot\Box
    LPCWSTR mailslotName = L"\\\\.\\mailslot\\Box";
    HANDLE hMailslot = NULL;          // Server handle
    
    // Key difference — nMaxMessage = 500
    hMailslot = CreateMailslot(
        mailslotName,      // Full name (LPCWSTR)
        500,               // Max message size (bytes)
        180000,            // Read timeout: 3 minutes
        NULL               // Default security
    );
    if (hMailslot == INVALID_HANDLE_VALUE) { HandleMailslotError("CreateMailslot"); return 1; }

    std::cout << "Mailslot created" << std::endl;
    std::cout << "Max message size: 500 bytes" << std::endl;
    std::cout << "Waiting for client message..." << std::endl;
    
    // Block 2: Read one message
    char buffer[512];                 // Buffer (margin above 500)
    DWORD bytesRead = 0;              // Actual read size
    BOOL readResult = ReadFile(
        hMailslot,                    // mailslot handle
        buffer,                       // buffer
        sizeof(buffer) - 1,           // leave space for '\0'
        &bytesRead,                   // bytes read
        NULL                          // synchronous I/O
    );
    if (readResult == FALSE) {
        DWORD error = GetLastError();
        if (error == ERROR_TIMEOUT) {
            std::cout << "Message wait timeout (3 minutes)" << std::endl;
        } else {
            HandleMailslotError("ReadFile");
        }
        CloseHandle(hMailslot);
        return 1;
    }
    
    // Block 3: Print result
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::cout << "Received message (" << bytesRead << " bytes):" << std::endl;
        std::cout << buffer << std::endl;
    } else {
        std::cout << "Empty message received" << std::endl;
    }
    
    CloseHandle(hMailslot);          // Close server handle
    std::cout << "Server shutting down." << std::endl;
    return 0;
}

