#ifndef COMMUNICATION_H_UKNNGCR4
#define COMMUNICATION_H_UKNNGCR4

#include "globals.h"

#define CODE_HEARTBEAT   L"0" // This means heartbeat.
#define CODE_BACKUP_FILE L"B" // Create backup of this file.
#define CODE_DONT_BACKUP L"b" // Do not create backup of this file.
#define HEARTBEAT_MSG    L"0HeartBeat"

#define CODE_DELETE_FILE L'D' // Delete this file.

#define MAX_CLIENTS 3 // Maximum number of clients that can be connected.

/**
 * @brief Various states.
 */
enum class PORT_STATE
{
    UnInitializing,
    UnInitialized,
    Initializing,
    Initialized,
    UnConncted,
    Connected,
};

extern PFLT_PORT gServerPort;
extern PORT_STATE gServerPortState;

extern PFLT_PORT gClientPort;
extern PORT_STATE gClientPortState;

extern UNICODE_STRING gServerPortName;

bool
IsUserAppConnected();

void
CloseClientPort();

void
SendHeartBeatToClient();

NTSTATUS
ShieldMessage(_In_ PVOID ConnectionCookie,
              _In_reads_bytes_opt_(InputBufferSize) PVOID InputBuffer,
              _In_ ULONG InputBufferSize,
              _Out_writes_bytes_to_opt_(OutputBufferSize,
                                        *ReturnOutputBufferLength)
                PVOID OutputBuffer,
              _In_ ULONG OutputBufferSize,
              _Out_ PULONG ReturnOutputBufferLength);

NTSTATUS
ShieldConnect(_In_ PFLT_PORT ClientPort,
              _In_ PVOID ServerPortCookie,
              _In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
              _In_ ULONG SizeOfContext,
              _Flt_ConnectionCookie_Outptr_ PVOID* ConnectionCookie);

VOID
ShieldDisconnect(_In_opt_ PVOID ConnectionCookie);

PORT_STATE
InitializeServerPort(IN PUNICODE_STRING PortName, IN PFLT_FILTER Filter);

void
DisconnectServerPort();

bool
SendMessageToClient(PWSTR buffer, ULONG size);

void
SendReanmeInfoToClient(IN PCUNICODE_STRING Volume,
                       IN PCUNICODE_STRING RootDir,
                       IN PCUNICODE_STRING NewName);

#endif /* end of include guard: COMMUNICATION_H_UKNNGCR4 */
