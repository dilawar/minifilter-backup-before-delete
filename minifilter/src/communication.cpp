/*
 * Communication port to talk to user mode applications.
 */

#include "communication.h"

UNICODE_STRING gServerPortName = RTL_CONSTANT_STRING(L"\\ShieldPort");

PFLT_PORT gServerPort = nullptr;
PORT_STATE gServerPortState = PORT_STATE::UnInitialized;

PFLT_PORT gClientPort = nullptr;
PORT_STATE gClientPortState = PORT_STATE::UnInitialized;

/**
 * @brief Return true if a client is connected.
 *
 * @return
 */
bool
IsUserAppConnected()
{
    return gClientPortState == PORT_STATE::Connected;
}

NTSTATUS
ShieldMessage(IN PVOID ConnectionCookie,
              IN PVOID InputBuffer,
              IN ULONG InputBufferSize,
              OUT PVOID OutputBuffer OPTIONAL,
              IN ULONG OutputBufferSize,
              OUT PULONG ReturnOutputBufferLength)
{
    UNREFERENCED_PARAMETER(ConnectionCookie);
    UNREFERENCED_PARAMETER(OutputBufferSize);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    if (InputBufferSize < 3) {
        DFLOG(ERROR, "Shield>" __FUNCTION__ ": Message is too short.");
        return STATUS_SUCCESS;
    }

    //
    // First byte is the command code. If it is 0, then I must delete the file.
    // rest of the bytes encode filename.
    //
    PWCH buf = reinterpret_cast<PWCH>(InputBuffer);
    WCHAR CommandCode = buf[0];
    buf = buf + 1; // its wchar. +1 means two bytes.

#if 0
    //
    // NOTE: verifier.exe doesn't like it a bit since we are assigning user
    // defined buffer.
    //
    UNICODE_STRING FileName;
    FileName.Buffer = buf;
    FileName.Length = (USHORT)(InputBufferSize - 2);
    FileName.MaximumLength = FileName.Length;
#else
    //
    // Now convert char* to UNCODE_STRING
    //
    UNICODE_STRING FileName;
    FileName.MaximumLength = (USHORT)InputBufferSize;
    DfAllocateUnicodeString(&FileName);
    RtlUnicodeStringCopyString(&FileName, buf);
#endif

    DFLOG(ERROR,
          "Shield>" __FUNCTION__ ": Recieved message from client of size %lu "
                                 "bytes. "
                                 "CommandCode=%c, FileName='%wZ'.\n",
          InputBufferSize,
          CommandCode,
          &FileName);

    if (CommandCode == CODE_DELETE_FILE) {

        //
        // Delete this file.
        //

        OBJECT_ATTRIBUTES obj;
        InitializeObjectAttributes(&obj,
                                   &FileName,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        status = ZwDeleteFile(&obj);

        if (!NT_SUCCESS(status)) {
            DFLOG(
              ERROR, "Failed to delete (code=%X): '%wZ'\n", status, FileName);
            goto EXIT;
        }

        DFLOG(ERROR,
              "Shield>" __FUNCTION__ ": Successfully deleted. '%wZ'.\n",
              FileName);
    } else {

        DFLOG(ERROR, "Command code %c is not supported.\n", CommandCode);
    }

EXIT:
    DfFreeUnicodeString(&FileName);
    return status;
}

NTSTATUS
ShieldConnect(_In_ PFLT_PORT ClientPort,
              _In_ PVOID ServerPortCookie,
              _In_reads_bytes_(SizeOfContext) PVOID ConnectionContext,
              _In_ ULONG SizeOfContext,
              _Flt_ConnectionCookie_Outptr_ PVOID* ConnectionCookie)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);
    UNREFERENCED_PARAMETER(ConnectionCookie);

    if (ClientPort == NULL) {
        DFLOG(ERROR,
              "Shield>" __FUNCTION__ ": Could not connect client. Error: Null "
                                     "client address.\n");

        gClientPortState = PORT_STATE::UnInitialized;
        return RPC_NT_NULL_REF_POINTER;
    }

    gClientPort = ClientPort;

    //
    // Port is connected.
    //
    gClientPortState = PORT_STATE::Connected;

    DFLOG(ERROR,
          "Shield>" __FUNCTION__ ":  Client connected: port %p.\n",
          gClientPort);

    return STATUS_SUCCESS;
}

void
CloseClientPort()
{
    if (gClientPort != NULL && gFilterHandle != NULL) {
        DFLOG(ERROR,
              "Shield> " __FUNCTION__ ": Client disconnecting port 0x%p.\n",
              gClientPort);

        FltCloseClientPort(gFilterHandle, &gClientPort);

        gClientPortState = PORT_STATE::UnInitialized; // also disconnected
        gClientPort = nullptr;
    }
}

VOID
ShieldDisconnect(_In_opt_ PVOID ConnectionCookie)
{
    UNREFERENCED_PARAMETER(ConnectionCookie);
    PAGED_CODE();
    DFLOG(ERROR, "Shield> " __FUNCTION__ ": Client is disconnected.\n");
    CloseClientPort();
}

/**
 * @brief  Initialize the server port.
 *
 * @param PortName
 * @param Filter
 */
PORT_STATE
InitializeServerPort(IN PUNICODE_STRING CommunicationPortName,
                     IN PFLT_FILTER Filter)
{
    NTSTATUS status = STATUS_SUCCESS;

    gServerPortState = PORT_STATE::UnInitialized;

    PSECURITY_DESCRIPTOR SecurityDescriptor;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    DFLOG(ERROR,
          "Shield>" __FUNCTION__ ": Trying opening port: '%wZ'.\n",
          CommunicationPortName);

    //
    // Create communication descriptor.
    //
    status = FltBuildDefaultSecurityDescriptor(&SecurityDescriptor,
                                               FLT_PORT_ALL_ACCESS);

    if (!NT_SUCCESS(status)) {
        DFLOG(
          ERROR,
          "Shield>" __FUNCTION__ " : Port is not initialized. Error "
                                 "FltBuildDefaultSecurityDescriptor - %X.\n",
          status);
        gServerPortState = PORT_STATE::UnInitialized;
        goto CleanUp;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               CommunicationPortName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               SecurityDescriptor);

    status = FltCreateCommunicationPort(
      Filter,
      &gServerPort,
      &ObjectAttributes,
      NULL,
      ShieldConnect,    // Connect notify callback
      ShieldDisconnect, // Disconnect notify callback.
      ShieldMessage,    // message notify callback.
      MAX_CLIENTS);

    if (!NT_SUCCESS(status)) {
        DFLOG(ERROR,
              "Shield>" __FUNCTION__ " : Port is not initialized. Error "
                                     "FltCreateCommunicationPort - %X.\n",
              status);
        gServerPortState = PORT_STATE::UnInitialized;
        goto CleanUp;
    }

    DFLOG(ERROR,
          "Shield>" __FUNCTION__ ": opened server port handle 0x%p.\n",
          gServerPort);

    gServerPortState = PORT_STATE::Initialized;

CleanUp:
    if (SecurityDescriptor)
        FltFreeSecurityDescriptor(SecurityDescriptor);

    return gServerPortState;
}

void
DisconnectServerPort()
{
    DFLOG(ERROR, "Shield>" __FUNCTION__ ": Closing server port...\n");

    CloseClientPort();

    FltCloseCommunicationPort(gServerPort);
    gServerPort = nullptr;
    gServerPortState = PORT_STATE::UnInitialized;
}

bool
SendMessageToClient(PWSTR buffer, ULONG size)
{
    if (gClientPortState != PORT_STATE::Connected)
        return false;

    // Set timeout 1000 (x100 ns)
    LARGE_INTEGER timeout;
    timeout.QuadPart = 1000;

    NTSTATUS status = FltSendMessage(
      gFilterHandle, &gClientPort, buffer, size, NULL, NULL, &timeout);

    if (!NT_SUCCESS(status)) {
        DFLOG(ERROR,
              "Shield>" __FUNCTION__ ": Failed to send message - code %X.\n",
              status);
        return false;
    }
    return true;
}

void
SendHeartBeatToClient()
{
    UNICODE_STRING HeartBeat = RTL_CONSTANT_STRING(HEARTBEAT_MSG);
    DFLOG(ERROR, "Shield>" __FUNCTION__ ". Sending heartbeat.\n");
    SendMessageToClient(HeartBeat.Buffer, HeartBeat.Length);
}

void
SendReanmeInfoToClient(IN PCUNICODE_STRING Volume,
                       IN PCUNICODE_STRING RootPath,
                       IN PCUNICODE_STRING NewName)
{
    //
    // The client only need to know the new client.
    //
    UNICODE_STRING NewPath;
    NewPath.MaximumLength = DF_FILENAME_SIZE * sizeof(WCHAR);
    DfAllocateUnicodeString(&NewPath);

    RtlAppendUnicodeToString(&NewPath, CODE_BACKUP_FILE);
    RtlAppendUnicodeStringToString(&NewPath, Volume);
    RtlAppendUnicodeStringToString(&NewPath, RootPath);
    RtlAppendUnicodeStringToString(&NewPath, NewName);

    if (SendMessageToClient(NewPath.Buffer, NewPath.Length))
        DFLOG(ERROR, "Shield>" __FUNCTION__ ": To backup '%wZ'\n", &NewPath);

    DfFreeUnicodeString(&NewPath);
    return;
}
