#include "globals.h"
#include "config.h"

#include "communication.h"
#include "files.h"

#pragma prefast(disable                                                        \
                : __WARNING_ENCODE_MEMBER_FUNCTION_POINTER,                    \
                  "Not valid for kernel mode drivers")

//
// Globals
//
static bool gIsFilterLoaded = false;
static bool gImageNotifyRoutineLoaded = false;
static PORT_STATE gServerPortStatus = PORT_STATE::UnInitialized;

extern "C"
{

    //
    //  Prototypes
    //

    DRIVER_INITIALIZE DriverEntry;

    NTSTATUS
    DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
                _In_ PUNICODE_STRING RegistryPath);

    NTSTATUS
    DfUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

    NTSTATUS
    DfInstanceSetup(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
                    _In_ DEVICE_TYPE VolumeDeviceType,
                    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType);

    NTSTATUS
    DfInstanceQueryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                            _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);

    VOID DfInstanceTeardownStart(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                                 _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags);

    VOID DfInstanceTeardownComplete(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                                    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags);

    /* User defined */
    FLT_PREOP_CALLBACK_STATUS
    BackupOnDeleteShield(_Inout_ PFLT_CALLBACK_DATA Data,
                       _In_ PCFLT_RELATED_OBJECTS FltObjects,
                       _Outptr_result_maybenull_ PVOID* CompletionContext);
} // extern C

//
// Function that is called to update the current process name that is trying to
// delete a file.
//

void
OnImageLoadNoitify(_In_ PCUNICODE_STRING FullImageName,
                   HANDLE ProcessId,
                   PIMAGE_INFO ImageInfo);

//
//  Text section assignments for all routines                               //
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DfUnload)
#pragma alloc_text(PAGE, DfInstanceSetup)
#pragma alloc_text(PAGE, DfInstanceQueryTeardown)
#pragma alloc_text(PAGE, DfInstanceTeardownStart)
#pragma alloc_text(PAGE, DfInstanceTeardownComplete)
#pragma alloc_text(PAGE, BackupOnDeleteShield)
#endif

//
// Notify when a process is mapped to virtual memory.
//
void
OnImageLoadNoitify(_In_ PCUNICODE_STRING FullImageName,
                   HANDLE ProcessId,
                   PIMAGE_INFO ImageInfo)
{
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(ImageInfo);

    RtlCopyUnicodeString(&gCurrentProcess, FullImageName);
}

//
//  Operation Registration                                                  //
//
//
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE, 0, BackupOnDeleteShield, NULL },

    // IRP_MJ_SET_INFORMATION is sent whenever file is marked for deletion.
    { IRP_MJ_SET_INFORMATION,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      BackupOnDeleteShield,
      NULL },

    { IRP_MJ_OPERATION_END }

};

//
//  Filter Registration
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof(FLT_REGISTRATION),   //  Size
    FLT_REGISTRATION_VERSION,   //  Version
    0,                          //  Flags
    NULL,                       //  Context
    Callbacks,                  //  Operation callbacks
    DfUnload,                   //  MiniFilterUnload
    DfInstanceSetup,            //  InstanceSetup
    DfInstanceQueryTeardown,    //  InstanceQueryTeardown
    DfInstanceTeardownStart,    //  InstanceTeardownStart
    DfInstanceTeardownComplete, //  InstanceTeardownComplete
    NULL,                       //  GenerateFileName
    NULL,                       //  NormalizeNameComponent
    NULL,                       //  NormalizeContextCleanup

};

/*************************************************************************
 * Helper function
 * ***********************************************************************/

//////////////////////////////////////////////////////////////////////////////
//  MiniFilter initialization and unload routines //
//////////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    DFLOG(ERROR,
          "Shield>" __FUNCTION__ ": Compiled on '%wZ' UTC.\n",
          &gCompilationTimestamp);

    //
    // Initialize the current process string. Allocate the buffer now.
    // Free it in DfUnload file.
    //
    gCurrentProcess.MaximumLength = DF_FILENAME_SIZE * sizeof(WCHAR);
    status = DfAllocateUnicodeString(&gCurrentProcess);
    if (!NT_SUCCESS(status))
        goto CleanUp;

    //
    // Initialize the current volume GUID. Free it in DfUnload file.
    //
    gVolumeGUID.MaximumLength = DF_VOLUME_GUID_NAME_SIZE * sizeof(WCHAR);
    status = DfAllocateUnicodeString(&gVolumeGUID);
    if (!NT_SUCCESS(status))
        goto CleanUp;

    //
    //  Support for older version of Win platform.
    //  Default to NonPagedPoolNx for non paged pool allocations where
    //  supported.
    //
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    //
    // register callbacks
    //
    status = PsSetLoadImageNotifyRoutine(
      (PLOAD_IMAGE_NOTIFY_ROUTINE)OnImageLoadNoitify);

    if (!NT_SUCCESS(status)) {
        gImageNotifyRoutineLoaded = false;
        DFLOG(ERROR,
              "Shield>"__FUNCTION__
              ": Failed to register image load callback. CODE=%X.\n",
              status);
    }
    gImageNotifyRoutineLoaded = true;

    //
    //  Register with FltMgr to tell it our callback routines
    //
    status =
      FltRegisterFilter(DriverObject, &FilterRegistration, &gFilterHandle);

    if (!NT_SUCCESS(status)) {
        DFLOG(ERROR,
              "Shield>" __FUNCTION__ " : Driver not started. Error "
                                     "FltRegisterFilter - %X.\n",
              status);
        goto CleanUp;
    }
    gIsFilterLoaded = true;

    //
    // Initialize server port.
    //
    gServerPortStatus = InitializeServerPort(&gServerPortName, gFilterHandle);
    DFLOG(ERROR,
          "Shield>" __FUNCTION__ ": server port status : %d.\n",
          gServerPortStatus);

    //
    //  Start filtering i/o
    //
    status = FltStartFiltering(gFilterHandle);
    if (!NT_SUCCESS(status)) {
        DFLOG(ERROR,
              "Shield>" __FUNCTION__ ": Driver not started. Error "
                                     "FltStartFiltering - %X.\n",
              status);
        goto CleanUp;
    }

    DFLOG(ERROR, "Shield>" __FUNCTION__ ": Driver has started.\n\n");

CleanUp:
    if (!NT_SUCCESS(status))
        if (gIsFilterLoaded)
            FltUnregisterFilter(gFilterHandle);
    return status;
}

NTSTATUS
DfUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags)
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    DFLOG(ERROR, "Shield>" __FUNCTION__ ": Unloading driver...\n");

    if (gImageNotifyRoutineLoaded) {
        status = PsRemoveLoadImageNotifyRoutine(
          (PLOAD_IMAGE_NOTIFY_ROUTINE)OnImageLoadNoitify);

        if (!NT_SUCCESS(status)) {
            DFLOG(
              ERROR,
              "Shield>" __FUNCTION__ ": Failed to unload OnImageLoadNoitify "
                                     "callback - %X\n",
              status);
        }
    }

    if (gServerPortStatus == PORT_STATE::Initialized) {
        DisconnectServerPort();
        DFLOG(ERROR, "Shield>" __FUNCTION__ ": Closed communication port.\n");
    }

    // Clear the unicode allocation holding process name.
    if (&gCurrentProcess)
        DfFreeUnicodeString(&gCurrentProcess);

    // Clear the volume name global.
    if (&gVolumeGUID)
        DfFreeUnicodeString(&gVolumeGUID);

    // Unload the filter.
    if (gFilterHandle && gIsFilterLoaded) {
        DFLOG(ERROR, "Shield>" __FUNCTION__ ": Unloading driver.\n");
        FltUnregisterFilter(gFilterHandle);
        gIsFilterLoaded = false;
        gFilterHandle = nullptr;
    }

    DFLOG(ERROR, "Shield>" __FUNCTION__ ": driver unloaded.\n");

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * Filter Instance Callbacks (Setup/Teardown/QueryTeardown)
 *
 *****************************************************************************/

NTSTATUS
DfInstanceSetup(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
                _In_ DEVICE_TYPE VolumeDeviceType,
                _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isWritable = FALSE;

    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);

    PAGED_CODE();

    status = FltIsVolumeWritable(FltObjects->Volume, &isWritable);

    if (!NT_SUCCESS(status)) {
        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    //  Attaching to read-only volumes is pointless as you should not be
    //  able to delete files on such a volume.
    //

    if (isWritable) {
        switch (VolumeFilesystemType) {
            case FLT_FSTYPE_NTFS:
            case FLT_FSTYPE_REFS:
                status = STATUS_SUCCESS;
                break;

            default:
                return STATUS_FLT_DO_NOT_ATTACH;
        }

    } else {
        DFLOG(ERROR,
              "It is a READONLY volume, I will not attach the minifilter.");
        return STATUS_FLT_DO_NOT_ATTACH;
    }

    //
    // Get the volume name.
    //
    status = FltGetVolumeGuidName(FltObjects->Volume, &gVolumeGUID, NULL);
    if (!NT_SUCCESS(status)) {
        DFLOG(ERROR,
              "Shield>" __FUNCTION__ ": Error FltGetVolumeGuidName - %X\n",
              status);

        // FIXME: Am I sure?
        return STATUS_FLT_DO_NOT_ATTACH;
    }

    return status;
}

NTSTATUS
DfInstanceQueryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                        _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    PAGED_CODE();
    DFLOG(ERROR, "Shied>" __FUNCTION__ ": exit.\n");
    return STATUS_SUCCESS;
}

VOID
DfInstanceTeardownStart(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                        _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    PAGED_CODE();
    DFLOG(ERROR, "Shied>" __FUNCTION__ ": exit.\n");
}

VOID
DfInstanceTeardownComplete(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                           _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);
    PAGED_CODE();
    DFLOG(ERROR, "Shied>" __FUNCTION__ ": exit.\n");
}

//////////////////////////////////////////////////////////////////////////////
//  MiniFilter Operation Callback Routines //
//////////////////////////////////////////////////////////////////////////////

/**
 * @brief
 *
 *  1. Disallow delete or rename on protected files.
 *  2. Create backup before deletion removes the file from the disk.
 *
 * @param Data
 * @param FltObjects
 * @param CompletionContext
 */
FLT_PREOP_CALLBACK_STATUS
BackupOnDeleteShield(_Inout_ PFLT_CALLBACK_DATA Data,
                   _In_ PCFLT_RELATED_OBJECTS FltObjects,
                   _Outptr_result_maybenull_ PVOID* CompletionContext)
{
    // PDF_STREAM_CONTEXT streamContext;

    NTSTATUS status;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    BOOLEAN IsDirectory = FALSE;
    PFLT_FILE_NAME_INFORMATION pFileNameInfo = nullptr;

    // By default, there is not need to call the post-operation routine
    // since there is no need for it.
    FLT_PREOP_CALLBACK_STATUS ret = FLT_PREOP_SUCCESS_NO_CALLBACK;

    PAGED_CODE();

    if (Data->RequestorMode == KernelMode) {
        // if the requestor is kernel mode, pass the request on uninterrupted
        ret = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto CleanUp;
    }

    //
    // If no client is connected to create backups then there is no point using
    // the shield.
    //
    if (!IsUserAppConnected()) {
        DFLOG(ERROR, "Shield>" __FUNCTION__ ": No backup client connected.\n");
        ret = FLT_PREOP_SUCCESS_NO_CALLBACK;
        goto CleanUp;
    }

    //
    // Check if the given file is a directory. We may use this information
    // in functions later.
    //
    status = FltIsDirectory(
      FltObjects->FileObject, FltObjects->Instance, &IsDirectory);

    //
    // We don't want anything that doesn't have the DELETE_ON_CLOSE flag.
    //
    if (Data->Iopb->MajorFunction == IRP_MJ_CREATE) {
        if (!FlagOn(Data->Iopb->Parameters.Create.Options,
                    FILE_DELETE_ON_CLOSE)) {
            // We are here because a file was created but FILE_DELETE_ON_CLOSE
            // was not set. That means, we don't want this file.
            goto CleanUp;
        }
    }

    //
    // We are interested in DELETE files or RENAME file operations.
    //

    bool IsRename = false;
    const auto fileInfoClass =
      Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

    if (Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION) {

        switch (fileInfoClass) {
            case FileRenameInformation:
                IsRename = true;
            case FileRenameInformationEx:
                IsRename = true;
            case FileDispositionInformation:
            case FileDispositionInformationEx:
            case FileRenameInformationBypassAccessCheck:
            case FileRenameInformationExBypassAccessCheck:
                break;
            default:
                goto CleanUp;
        }
    }
    status = FltGetFileNameInformation(Data,
                                       FLT_FILE_NAME_NORMALIZED |
                                         FLT_FILE_NAME_QUERY_DEFAULT,
                                       &pFileNameInfo);
    if (!NT_SUCCESS(status)) {
        DFLOG(ERROR,
              "Shield>" __FUNCTION__ ": Error FltGetFileNameInformation - Code "
                                     "%X.\n",
              status);
        goto CleanUp;
    }

    status = FltParseFileNameInformation(pFileNameInfo);

    if (!NT_SUCCESS(status)) {
        DFLOG(
          ERROR,
          "Shield>" __FUNCTION__ ": Error FltParseFileNameInformation - Code "
                                 "%X.\n",
          status);
        goto CleanUp;
    }

    //
    // TODO: Allow some processes to delete/rename without backup.
    // Get the name of the process that is trying to delete the
    // file. The callback OnImageLoadNoitify set the process image
    // name to gCurrentProcess that can be used here to allow or
    // disallow deletion.
    //
    // DFLOG(ERROR, "Shield>" __FUNCTION__ ": process: %wZ.\n",
    // &gCurrentProcess);

    if (IsRename) {

        //
        // The file/directory is being renamed. We only protect rename operation
        // on the files that are protected or created by the shield. For rest of
        // the files, rename can continue.
        //

        UNICODE_STRING FileName;
        FileName.MaximumLength = DF_FILENAME_SIZE * sizeof(WCHAR);

        status = DfAllocateUnicodeString(&FileName);
        ASSERT(NT_SUCCESS(status));

        if (TRUE == IsRenameAllowed(pFileNameInfo)) {

            //
            // Rename is allowed, continue with rename. No need to
            // call post callback. This should be the case for most of the
            // time.
            //
            ret = FLT_PREOP_SUCCESS_NO_CALLBACK;
            return ret;
        }

        //
        // Rename is not allowed. Deny access.
        //
        //

        DFLOG(ERROR,
              "Shield>"__FUNCTION__
              ": Rename operation not allowed on '%wZ'.\n",
              pFileNameInfo->Name);

        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;

        //
        // Complete the I/O request and send it back up.
        //
        ret = FLT_PREOP_COMPLETE;
        goto CleanUp;
    }

    //
    // DELETE
    //

    /**
     * This section protect a few extensions. These extentions
     * cannot be deleted.
     */
    if (FALSE == IsDeletionAllowed(pFileNameInfo)) {
        DFLOG(ERROR,
              "Shield>" __FUNCTION__ ": Protecting file "
                                     "deletion/rename '%wZ'.\n",
              &pFileNameInfo->Name);

        // String match, deny access!
        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;

        // Complete the I/O request and send it back up.
        ret = FLT_PREOP_COMPLETE;
        goto CleanUp;
    }

    if (FALSE == IsBackupAllowed(pFileNameInfo)) {
        goto CleanUp;
    }

    //
    // If we are here, then we mark the file hidden and rename it.
    // TODO: Send the new name to service that would perform the backup and
    // delete the renamed file.
    //

    HideFile(Data);

    //
    // Try renaming file.
    //

    const auto RenameStatus =
      RenameOnDelete(FltObjects, pFileNameInfo, IsDirectory);

    //
    // If file is generated by a previous rename operation then the rename
    // operation will fail with code RENAME_STATUS::FAILED_PROTECTED_FILE code.
    // In this situation, don't rename file and don't delete it. Only the
    // backup service can delete such file after backing it up.
    //

    if (RenameStatus == RENAME_STATUS::FAILED_FILE_BEING_BACKED_UP) {

        DFLOG(ERROR,
              "Shield>" __FUNCTION__ ": Error - '%wZ' was created by "
                                     "shield.\n",
              pFileNameInfo->Name);

        // Don't allow further IO.
        Data->IoStatus.Status = STATUS_ACCESS_DENIED;
        Data->IoStatus.Information = 0;

        ret = FLT_PREOP_COMPLETE;
        goto CleanUp;
    }

    // On a general failure, delete the file.
    // TODO: Think about if this is allowed.
    if (RenameStatus == RENAME_STATUS::SUCCESS) {

        //
        // If we are here, we have renamed the file. DO NOT
        // ALLOW DELETE of original file here because the
        // original file will be deleted. because renamed file
        // will also be deleted.
        //
        // TODO: Notify the backup service.
        //
        DFLOG(ERROR, "Shield>" __FUNCTION__ ": Rename is successful.\n");

        Data->IoStatus.Status = STATUS_SUCCESS;
        ret = FLT_PREOP_COMPLETE;
        goto CleanUp;
    } else {

        //
        // Rename operation failed. We can't do much here. Continue as if
        // nothing happened.
        //
        Data->IoStatus.Status = STATUS_SUCCESS;
        ret = FLT_PREOP_COMPLETE;
        goto CleanUp;
    }

CleanUp:
    if (pFileNameInfo)
        FltReleaseFileNameInformation(pFileNameInfo);
    return ret;
}
