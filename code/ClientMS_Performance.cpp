#include <windows.h>
#include <iostream>
#include <string>
#include <iomanip>

// ------------------------------
// ClientMS_Performance: sends 1000 messages to the server Mailslot
// Measures elapsed time, messages/sec and throughput.
// ------------------------------

// Unified error printing
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

int main(int argc, char* argv[]) {
    SetConsoleCP(1251);                       // Console input in 1251
    SetConsoleOutputCP(1251);                 // Console output in 1251

    const int MESSAGE_COUNT = 1000;           // How many messages to send
    const char* message = "Hello from Maislot-client"; // Payload
    size_t messageLen = strlen(message);      // Payload size in bytes
    
    // Local lambda: ANSI (1251) -> UTF‑16
    auto to_wstring_from_ansi = [](const std::string &s) -> std::wstring {
        if (s.empty()) return std::wstring();
        int len = MultiByteToWideChar(1251, 0, s.c_str(), (int)s.size(), NULL, 0);
        std::wstring ws; ws.resize(len);
        MultiByteToWideChar(1251, 0, s.c_str(), (int)s.size(), &ws[0], len);
        return ws;
    };

    // Default target: local \\.\mailslot\Box
    std::wstring mailslotNameW = L"\\\\.\\mailslot\\Box";
    // If an argument is provided, treat it as a remote machine name
    if (argc > 1) {
        std::string computerNameA = argv[1];
        std::wstring computerName = to_wstring_from_ansi(computerNameA);
        mailslotNameW = L"\\\\" + computerName + L"\\mailslot\\Box";
    }
    
    std::cout << "Performance test: sending " << MESSAGE_COUNT << " messages" << std::endl;
    std::wcout << L"Server: " << mailslotNameW << std::endl;
    std::cout << "Message size: " << messageLen << " bytes" << std::endl;
    
    // Open server mailslot once
    HANDLE hMailslot = CreateFile(
        mailslotNameW.c_str(),   // LPCWSTR name
        GENERIC_WRITE,           // write access
        FILE_SHARE_READ,         // allow shared read
        NULL,                    // default security
        OPEN_EXISTING,           // mailslot must exist
        0,                       // flags/attributes
        NULL                     // template not used
    );
    if (hMailslot == INVALID_HANDLE_VALUE) {
        HandleMailslotError("CreateFile");
        std::cout << "Failed to open Mailslot" << std::endl;
        std::cout << "Make sure ServerMS is running." << std::endl;
        return 1;
    }
    std::cout << "Mailslot opened. Starting to send..." << std::endl;
    
    // High-resolution timers
    LARGE_INTEGER frequency, startTime, endTime;
    QueryPerformanceFrequency(&frequency);     // ticks per second
    QueryPerformanceCounter(&startTime);       // start
    
    int successCount = 0;                      // Successfully sent
    int errorCount = 0;                        // Failed writes
    
    for (int i = 0; i < MESSAGE_COUNT; i++) {
        DWORD bytesWritten = 0;
        BOOL writeResult = WriteFile(
            hMailslot,               // mailslot handle
            message,                 // data
            (DWORD)messageLen,       // size
            &bytesWritten,           // actually written
            NULL                     // synchronous I/O
        );
        if (writeResult == FALSE) {
            errorCount++;
            if (errorCount <= 5) {   // avoid spamming the console
                HandleMailslotError("WriteFile");
            }
        } else {
            successCount++;
        }
        if ((i + 1) % 100 == 0) {    // progress every 100 messages
            std::cout << "Sent: " << (i + 1) << " / " << MESSAGE_COUNT << std::endl;
        }
    }
    
    QueryPerformanceCounter(&endTime);         // end
    CloseHandle(hMailslot);
    
    // Metrics
    double elapsedSeconds = (double)(endTime.QuadPart - startTime.QuadPart) / frequency.QuadPart; // t = dTicks / Hz
    double messagesPerSecond = (double)successCount / elapsedSeconds;                              // msg/s
    double totalBytes = (double)successCount * messageLen;                                         // total bytes
    double bytesPerSecond = totalBytes / elapsedSeconds;                                           // B/s
    
    // Report
    std::cout << "\n========== MEASUREMENT RESULTS ==========" << std::endl;
    std::cout << "Messages: " << MESSAGE_COUNT << std::endl;
    std::cout << "Succeeded: " << successCount << std::endl;
    std::cout << "Errors: " << errorCount << std::endl;
    std::cout << "Elapsed: " << std::fixed << std::setprecision(3) << elapsedSeconds << " s" << std::endl;
    std::cout << "Rate:    " << std::fixed << std::setprecision(2) << messagesPerSecond << " msg/s" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << bytesPerSecond << " B/s" << std::endl;
    std::cout << "Throughput: " << std::fixed << std::setprecision(2) << (bytesPerSecond / 1024.0) << " KB/s" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    std::cout << "\nClient is exiting." << std::endl;
    return (errorCount == 0) ? 0 : 1;          // Exit code: 0 — no write errors
}

