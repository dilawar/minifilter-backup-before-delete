/**
 * Files related operations.
 */

#ifndef FILES_H_2URCQHKU
#define FILES_H_2URCQHKU

#include "globals.h"

const enum class RENAME_STATUS {
    SUCCESS,                     // rename was a success
    FAILED,                      // Rename was a failure
    FAILED_FILE_BEING_BACKED_UP, // rename failed because this file is being
                                 // backed up
};

BOOLEAN
IsRenameAllowed(_In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo);

BOOLEAN
IsDeletionAllowed(_In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo);

BOOLEAN
IsBackupAllowed(_In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo);

BOOLEAN
HideFile(_Inout_ PFLT_CALLBACK_DATA Data);

RENAME_STATUS
RenameOnDelete(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                     _In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo,
                     BOOLEAN IsDirectory);

BOOLEAN
DeleteFile(_In_ PCFLT_RELATED_OBJECTS FltObjects,
                 _In_ PFLT_FILE_NAME_INFORMATION pFileNameInfo);

#endif /* end of include guard: FILES_H_2URCQHKU */
