# IPC (Inter-Process Communication)

## Shared Memory

> This may be the fastest solution to IPC.

> The two processes must be on the same PC.

- Win32: [https://docs.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory]
	- CreateFileMapping, MapViewOfFile, UnMapViewOfFile, CloseHandle.
- Posix: shmget, shmat, shmdt, shmctl API calls.

## UDP / TCP Sockets

> For both Win32 and Posix the API is basically the same.

> This is networking so works across computer.

- Win32: WinSock
- Posix: Berkely API

### Ports

- They are 16bit so the 'full' range is 0 - 65535
- Use Port 0 for the OS to choose.
- Ports 0 - 1000 are reserved for services.
  - Ex: HTTP: 80, HTTPS: 443, SSH: 22
- Ports we can use (1,000 to 65,535)

## Named Pipes

> Pipes work on the same PC or across the network.

> There are also anon pipes, they are one way (parent -> child).

- Win32:
	- Pipe name format server: "\\.\pipe\PipeName", client: "\\ComputerName\pipe\PipeName" (remeber to correctly escape backslashes)
	- CreateNamedPipe, ConnectNamedPipe, etc..
- Posix: mkfifo, mknod, normal file IO functions.


## WM_COPY_DATA / SendMessage (Win32)

> Handle the 'WM_COPY_DATA' message in the WinProc function.

> Server calls 'SendMessage' with some data.

> Must two processes be on the same PC.

## Mailslot (Win32)

> Works only for really small data sizes.

> One way, client -> server.

> Works across network.
