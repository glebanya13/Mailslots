#include <windows.h>
#include <iostream>
#include <string>

// ------------------------------
// ServerMS_MultiMessage: Mailslot server that receives many messages
// Creates \\.\mailslot\Box, prints every received message, counts them,
// and exits on timeout or when the window is closed.
// ------------------------------

// Error helper for Mailslot/WinAPI operations
void HandleMailslotError(const char* operation) {
    DWORD error = GetLastError();
    std::cerr << "Error in operation '" << operation << "': ";
    switch (error) {
        case ERROR_INVALID_PARAMETER: std::cerr << "Invalid parameter"; break;
        case ERROR_ALREADY_EXISTS:    std::cerr << "Mailslot already exists"; break;
        case ERROR_PIPE_BUSY:         std::cerr << "Mailslot is busy"; break;
        case ERROR_BROKEN_PIPE:       std::cerr << "Broken pipe"; break;
        case ERROR_TIMEOUT:           std::cerr << "Timeout"; break;
        case ERROR_INSUFFICIENT_BUFFER: std::cerr << "Insufficient buffer"; break;
        default: std::cerr << "Error code: " << error; break;
    }
    std::cerr << std::endl;
}

int main() {
    SetConsoleCP(1251);                // Console input code page
    SetConsoleOutputCP(1251);          // Console output code page

    // Block 1: Create Mailslot (local \\. prefix)
    LPCWSTR mailslotName = L"\\\\.\\mailslot\\Box";
    HANDLE hMailslot = NULL;
    
    // 3‑minute timeout allows graceful exit when no client is present
    hMailslot = CreateMailslot(
        mailslotName,  // Full mailslot name
        300,           // Max incoming message size (bytes)
        180000,        // Read timeout in ms (3 minutes)
        NULL           // Default security attributes
    );
    if (hMailslot == INVALID_HANDLE_VALUE) { HandleMailslotError("CreateMailslot"); return 1; }

    std::cout << "Mailslot created" << std::endl;
    std::cout << "Waiting for client messages..." << std::endl;
    std::cout << "Press Ctrl+C or close the window to exit" << std::endl;
    
    // Block 2: Loop reading multiple messages
    char buffer[512];                   // Read buffer (margin above 300 bytes)
    DWORD bytesRead = 0;                // Actual read size
    int messageCount = 0;               // Message counter
    
    while (true) {
        // ReadFile blocks until data is available or timeout occurs
        BOOL readResult = ReadFile(
            hMailslot,                  // server handle
            buffer,                     // destination buffer
            sizeof(buffer) - 1,         // leave space for trailing '\0'
            &bytesRead,                 // bytes read
            NULL                        // synchronous I/O
        );
        
        if (readResult == FALSE) {
            DWORD error = GetLastError();
            if (error == ERROR_TIMEOUT) {        // Exit when no messages for 3 minutes
                std::cout << "\nMessage wait timeout (3 minutes)" << std::endl;
                std::cout << "Total messages received: " << messageCount << std::endl;
                break;
            } else {
                HandleMailslotError("ReadFile");
                break;
            }
        }
        
        // Block 3: Process each message
        messageCount++;
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            std::cout << "[" << messageCount << "] Received (" << bytesRead << " bytes): " << buffer << std::endl;
        } else {
            std::cout << "[" << messageCount << "] Empty message" << std::endl;
        }
        
        // Progress every 100 messages
        if (messageCount % 100 == 0) {
            std::cout << "Processed messages: " << messageCount << std::endl;
        }
    }
    
    CloseHandle(hMailslot);             // Release server handle
    std::cout << "\nServer shutting down. Total messages: " << messageCount << std::endl;
    return 0;
}

