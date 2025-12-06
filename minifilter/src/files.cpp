/**
 * Files Operations.
 */

#include "files.h"
#include "communication.h"

/**
 * List of extentions that can not be deleted.
 * TODO: These are currently hardcoded.
 */
#define NUM_PROTECTED_WORDS 2
static UNICODE_STRING ProtectedWords[NUM_PROTECTED_WORDS] = {
    RTL_CONSTANT_STRING(L".protected."),
    RTL_CONSTANT_STRING(L".readonly.")
};

/**
 * List of extentions that needs no backup.
 */
#define NUM_IGNORE_BACKUP_EXTENSIONS 2
static UNICODE_STRING BackupExtentionsIgnore[NUM_IGNORE_BACKUP_EXTENSIONS] = {
    RTL_CONSTANT_STRING(L"tmp"),
    RTL_CONSTANT_STRING(L"log")
};

/**
 * List of words if found in the path disable backup.
 */
#define NUM_IGNORE_SUBDIR_IN_PATH 4
static UNICODE_STRING IgnoreSubdirInPath[NUM_IGNORE_SUBDIR_IN_PATH] = {
    RTL_CONSTANT_STRING(L"\\Temp\\"),
    RTL_CONSTANT_STRING(L"\\AppData\\"),
    RTL_CONSTANT_STRING(L"\\System32\\"),
    RTL_CONSTANT_STRING(L"\\Logs\\")
};

BOOLEAN
IsRenameAllowed(_In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo)
{
    // If a protected word is found in the name of the file, do not allow
    // renaming.
    for (unsigned int i = 0; i < NUM_PROTECTED_WORDS; i++) {
        if (TRUE ==
            FindSubString(&pFileNameInfo->FinalComponent, &ProtectedWords[i])) {
            DFLOG(
              ERROR,
              "Shield>" __FUNCTION__ ": Rename not allowed because a "
                                     "protected word '%wZ' found in the name "
                                     "'%wZ'\n.",
              &ProtectedWords[i],
              &pFileNameInfo->FinalComponent);
            return FALSE;
        }
    }

    // If the name has SUBCOM_RENAME_PREFIX then don't allow moving this
    // directory.
    if (RtlPrefixUnicodeString(
          &gSUBCOM_RENAME_PREFIX, &pFileNameInfo->FinalComponent, FALSE)) {
        DFLOG(
          ERROR,
          "Shield>" __FUNCTION__ ": Rename is not allowed because this file "
                                 "was created by the Shield and has not been "
                                 "backed-up yet.\n");
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
IsDeletionAllowed(_In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo)
{
    for (unsigned int i = 0; i < NUM_PROTECTED_WORDS; i++) {
        if (TRUE ==
            FindSubString(&pFileNameInfo->FinalComponent, &ProtectedWords[i])) {
            DFLOG(
              ERROR,
              "Shield>" __FUNCTION__ ": Delete not allowed because a "
                                     "protected word '%wZ' found in the name "
                                     "'%wZ'\n.",
              &ProtectedWords[i],
              &pFileNameInfo->FinalComponent);
            return FALSE;
        }
    }
    return TRUE;
}

BOOLEAN
IsBackupAllowed(_In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo)
{
    if (pFileNameInfo->Extension.Length == 0 ||
        pFileNameInfo->FinalComponent.Length == 0) {
        DFLOG(DFDBG_TRACE_ROUTINES,
              "Empty extentions. Won't backup: '%wZ'.\n",
              &pFileNameInfo->Name);
        return FALSE;
    }

    // Don't try backup if  FinalComponent is NULL or its Length is
    // 0 Files with very small names gets no protection.
    if (pFileNameInfo->FinalComponent.Length <= 2) {
        DFLOG(DFDBG_TRACE_ROUTINES,
              "No backup. Name of file is <2 chars: '%wZ'.\n",
              &pFileNameInfo->Name);
        return FALSE;
    }

    // If the name ernds with ~ and other special
    // characters(TODO), don't protect.
    // NB: Lenght is number of bytes and not the number of
    // characters.
    USHORT LastIndex =
      (pFileNameInfo->FinalComponent.Length / sizeof(WCHAR)) - 1;
    if (L'~' == pFileNameInfo->FinalComponent.Buffer[LastIndex]) {
        DFLOG(DFDBG_TRACE_ROUTINES,
              "Ends with ~. No backup: "
              "'%wZ'.\n",
              &pFileNameInfo->Name);
        return FALSE;
    }

    // Ignore these file extensions.
    for (unsigned int i = 0; i < NUM_IGNORE_BACKUP_EXTENSIONS; i++) {
        if (RtlCompareUnicodeString(&pFileNameInfo->Extension,
                                    &BackupExtentionsIgnore[i],
                                    TRUE) == 0) {
            DFLOG(DFDBG_TRACE_ROUTINES,
                  "Ignore ext. No backup: '%wZ'.\n",
                  &pFileNameInfo->Name);
            return FALSE;
        }
    }

    // ignore if these subdirectories are found in the filepath.
    for (unsigned int i = 0; i < NUM_IGNORE_SUBDIR_IN_PATH; i++) {
        if (FindSubString(&pFileNameInfo->Name, &IgnoreSubdirInPath[i]) ==
            TRUE) {
            DFLOG(DFDBG_TRACE_ROUTINES,
                  "'%wZ' found: Not creating backup: '%wZ'.\n",
                  &IgnoreSubdirInPath[i],
                  &pFileNameInfo->Name);
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Make this file hidden.
 */
BOOLEAN
HideFile(_Inout_ PFLT_CALLBACK_DATA Data)
{
    FILE_BASIC_INFORMATION fileBasicInfo = { 0 };
    fileBasicInfo.FileAttributes = FILE_ATTRIBUTE_HIDDEN;

    auto status = FltSetInformationFile(Data->Iopb->TargetInstance,
                                        Data->Iopb->TargetFileObject,
                                        &fileBasicInfo,
                                        sizeof(FILE_BASIC_INFORMATION),
                                        FileBasicInformation);

    if (!NT_SUCCESS(status)) {
        DFLOG(ERROR,
              "Shield>" __FUNCTION__ ": '%wZ' - CODE=%X'.\n",
              &Data->Iopb->TargetFileObject->FileName,
              status);
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Rename this file.
 *
 * # Useful links.
 *
 * https://community.osr.com/discussion/153128/how-to-simple-rename-a-file-in-minifilter
 *
 */
RENAME_STATUS
RenameOnDelete(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                     _In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo,
                     BOOLEAN IsDirectory)
{
    // Requires DELETE access.
    NTSTATUS status = STATUS_SUCCESS;
    auto ret = RENAME_STATUS::SUCCESS;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(status);
    UNREFERENCED_PARAMETER(IsDirectory);

    UNICODE_STRING outFileName = { 0 };
    FILE_RENAME_INFORMATION* fileRenameInfo = nullptr;

    PAGED_CODE();

    //
    // If this path already have SUBCOM_RENAME_PREFIX at the begining then we
    // are trying to delete a file that is being backedup. No not process
    // further and return a code that caller knows about. The caller must not
    // allow deletion of this file.
    //
    if (RtlPrefixUnicodeString(
          &gSUBCOM_RENAME_PREFIX, &pFileNameInfo->FinalComponent, FALSE)) {
        ret = RENAME_STATUS::FAILED_FILE_BEING_BACKED_UP;
        goto RENAME_EXIT;
    }

    //
    // Prepare of new name of the file.
    //
    outFileName.MaximumLength = sizeof(WCHAR) * DF_FILENAME_SIZE;
    status = DfAllocateUnicodeString(&outFileName);
    ASSERT(NT_SUCCESS(status));

    RtlAppendUnicodeStringToString(&outFileName, &gSUBCOM_RENAME_PREFIX);
    RtlAppendUnicodeStringToString(&outFileName,
                                   &pFileNameInfo->FinalComponent);
    RtlAppendUnicodeStringToString(&outFileName, &gSUBCOM_RENAME_SUFFIX);

    //
    // Read this thread carefully:
    // https://community.osr.com/discussion/130644/zwsetinformationfile-bsod-error-when-trying-to-rename-a-file
    // Always allocate this structure on heap.
    //

    ULONG fileRenameInfoLength =
      sizeof(FILE_RENAME_INFORMATION) + outFileName.Length;

    fileRenameInfo =
      static_cast<FILE_RENAME_INFORMATION*>(ExAllocatePoolWithTag(
        PagedPool, fileRenameInfoLength, DF_ERESOURCE_POOL_TAG));

    if (fileRenameInfo == NULL) {
        ret = RENAME_STATUS::FAILED;
        goto RENAME_EXIT;
    }

    fileRenameInfo->ReplaceIfExists = TRUE;
    fileRenameInfo->RootDirectory = NULL;
    fileRenameInfo->FileNameLength = outFileName.Length;

    RtlCopyMemory(
      fileRenameInfo->FileName, outFileName.Buffer, outFileName.Length);

    status = FltSetInformationFile(FltObjects->Instance,
                                   FltObjects->FileObject,
                                   fileRenameInfo,
                                   fileRenameInfoLength,
                                   FileRenameInformation);

    if (!NT_SUCCESS(status)) {
        DFLOG(
          ERROR,
          "Shield> " __FUNCTION__ ": Failed '%wZ' -> '%wZ'. Error. CODE=%X'.\n",
          &pFileNameInfo->Name,
          &outFileName,
          status);
        ret = RENAME_STATUS::FAILED;
        goto RENAME_EXIT;
    }

    //
    // TODO: Call when pFileNameInfo->Format == FLT_FILE_NAME_NORMALIZED
    // rename successful. Let the client know.
    //
    SendReanmeInfoToClient(
      &gVolumeGUID, &pFileNameInfo->ParentDir, &outFileName);

RENAME_EXIT:
    if (fileRenameInfo != NULL)
        ExFreePoolWithTag(fileRenameInfo, DF_ERESOURCE_POOL_TAG);

    if (&outFileName)
        DfFreeUnicodeString(&outFileName);

    return ret;
}

BOOLEAN
DeleteFile(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                 _In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo)
{
    // Requires DELETE access.
    NTSTATUS status = STATUS_SUCCESS;
    bool ret = TRUE;

    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(status);

    FILE_DISPOSITION_INFORMATION DispositionInformation = { 0 };
    DispositionInformation.DeleteFile = TRUE;

    PAGED_CODE();

    status = FltSetInformationFile(FltObjects->Instance,
                                   FltObjects->FileObject,
                                   &DispositionInformation,
                                   sizeof(DispositionInformation),
                                   FileDispositionInformation);

    if (!NT_SUCCESS(status)) {
        DFLOG(
          ERROR,
          "Shield> " __FUNCTION__ ": Failed to delete '%wZ' Error. CODE=%X'.\n",
          &pFileNameInfo->Name,
          status);
        ret = FALSE;
        goto DELETE_EXIT;
    }

DELETE_EXIT:
    return ret;
}
