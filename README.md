# Mailslots

Minimal Windows IPC examples using the Mailslot API.

This repository contains:
- ServerMS: basic server creating a local mailslot `\\.\mailslot\Box`, receives one message and exits
- ServerMS_MultiMessage: server that continuously receives messages (until timeout)
- ServerMS_500bytes: server variant with 500‑byte max message size
- ClientMS: client that can send to local or remote servers, including multiple hosts
- ClientMS_Performance: client that sends 1000 messages and reports throughput

Console I/O is configured for Windows‑1251 to display Russian text correctly; logic is locale‑agnostic. Binaries can be built as static (/MT) to avoid MSVC runtime redistribution.
