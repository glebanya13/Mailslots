# Testing Guide (Windows/VMs)

This guide explains how to run and validate the Mailslots examples on Windows machines or virtual machines.

## Prerequisites

1. Copy the repository to your Windows machines/VMs
2. Build executables with MSVC (Developer Command Prompt) or Visual Studio (see README)
3. Ensure all machines are on the same network and reachable by name

## Quick Start

### 1 Basic Local Test (single machine)

On the same machine:
```cmd
:: terminal 1
ServerMS.exe

:: terminal 2
ClientMS.exe
```
Expected: the server prints the received "Hello from Maislot-client" message and exits.

### 2 Continuous Receiving Server

On the same machine:
```cmd
:: terminal 1
ServerMS_MultiMessage.exe

:: terminal 2 (run multiple times if needed)
ClientMS.exe
```
Expected: the server keeps printing messages. It stops after a 3‑minute timeout if no messages arrive.

### 3 Remote Server (two machines)

On machine A (server):
```cmd
ServerMS.exe
```
On machine B (client), using the hostname of machine A:
```cmd
ClientMS.exe MACHINE_A
```
Expected: the server on A receives and prints the message.

### 4 Multiple Servers (three machines)

Start a server on each machine:
```cmd
ServerMS.exe   :: on MACHINE_A
ServerMS.exe   :: on MACHINE_B
ServerMS.exe   :: on MACHINE_C
```
From any machine (or a fourth one), run client with multiple targets:
```cmd
ClientMS.exe MACHINE_A MACHINE_B MACHINE_C
```
Expected: each server receives one message.

### 5 Increased Message Size (500 bytes)

Use the 500‑byte server on your machines:
```cmd
ServerMS_500bytes.exe
```
Send messages with the normal client:
```cmd
ClientMS.exe MACHINE_A MACHINE_B MACHINE_C
```
Expected: messages are received correctly with the larger size. Adjust message text in the client if you want to test longer payloads.

### 6 Performance Test (1000 messages)

On server machine:
```cmd
ServerMS_MultiMessage.exe
```
On client machine:
```cmd
ClientMS_Performance.exe MACHINE_A
```
Expected: progress updates during send and a final report with time, messages/sec, and throughput.

## Finding the Machine Name

In Windows Command Prompt:
```cmd
hostname
```
Or:
```cmd
echo %COMPUTERNAME%
```

## Troubleshooting

- "Mailslot not found" / ERROR_FILE_NOT_FOUND
  - Ensure the server is running
  - Verify the machine name with `hostname`
  - Check network connectivity and Windows Firewall

- Timeout (no messages for 3 minutes)
  - Make sure the client is running and targets the correct server name
  - Try the local test first on the same machine

- Missing VCRUNTIME140.dll
  - Build with static runtime `/MT`, or install the "Microsoft Visual C++ Redistributable for Visual Studio 2015–2022" matching your architecture

- Encoding issues (garbled Cyrillic text)
  - Consoles are set to Windows‑1251 in code (SetConsoleCP/SetConsoleOutputCP)
  - Use a font like Consolas in the terminal

## Notes

- Mailslots are a Windows IPC mechanism and work within local networks
- Servers accept raw bytes; clients in these examples send ASCII text
- You cannot create multiple mailslots with the same name on the same machine; use different names if you need parallel servers locally

