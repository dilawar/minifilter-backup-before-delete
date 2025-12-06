/*
 * Common functions and macros.
 */

#ifndef COMMON_H_EP6FTA5H
#define COMMON_H_EP6FTA5H

//
// Global headers.
//
#include <dontuse.h>
#include <fltKernel.h>
#include <suppress.h>
#include <Ntstrsafe.h>

//
// Global macros.
//
#define ERROR                        0x00000001
#define DFDBG_TRACE_ROUTINES         0x00000002
#define DFDBG_TRACE_OPERATION_STATUS 0x00000004

#define DF_VOLUME_GUID_NAME_SIZE 48
#define DF_FILENAME_SIZE         260

#define DF_INSTANCE_CONTEXT_POOL_TAG    'skly'
#define DF_STREAM_CONTEXT_POOL_TAG      'skta'
#define DF_TRANSACTION_CONTEXT_POOL_TAG 'm0ss'
#define DF_ERESOURCE_POOL_TAG           'mzzx'
#define DF_DELETE_NOTIFY_POOL_TAG       'quio'
#define DF_STRING_POOL_TAG              'xolw'

#define DF_CONTEXT_POOL_TYPE PagedPool

//
// Globals
//

// Prefix and suffix when renaming a file.
#if 0
const UNICODE_STRING gSUBCOM_RENAME_PREFIX = RTL_CONSTANT_STRING(L".πππ.");
const UNICODE_STRING gSUBCOM_RENAME_SUFFIX = RTL_CONSTANT_STRING(L".πππ");
#else
const UNICODE_STRING gSUBCOM_RENAME_PREFIX = RTL_CONSTANT_STRING(L".^^^.");
const UNICODE_STRING gSUBCOM_RENAME_SUFFIX = RTL_CONSTANT_STRING(L".^^^");
#endif

// Handles
extern PFLT_FILTER gFilterHandle;

// Current process name.
extern UNICODE_STRING gCurrentProcess;

// Volume GUI
extern UNICODE_STRING gVolumeGUID;

// Debug level.
extern ULONG gTraceFlags;

//////////////////////////////////////////////////////////////////////////////
//  Macros                                                                  //
//////////////////////////////////////////////////////////////////////////////

#define DF_PRINT(...)                                                          \
    DbgPrintEx(DPFLTR_FLTMGR_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)

#define DFLOG(_dbgLevel, ...)                                                  \
    (FlagOn(gTraceFlags, (_dbgLevel)) ? DF_PRINT(__VA_ARGS__) : (0))

#define FlagOnAll(F, T) (FlagOn(F, T) == T)

extern "C"
{

    //
    // GLOBALS
    //
    extern PFLT_FILTER gFilterHandle;
    extern UNICODE_STRING gCurrentProcess;

    extern ULONG gTraceFlags;

    //
    // Other
    //

    extern NTSTATUS
    DfAllocateUnicodeString(_Inout_ PUNICODE_STRING String);

    extern VOID
    DfFreeUnicodeString(_Inout_ PUNICODE_STRING String);

} // extern C

#pragma alloc_text(PAGE, DfAllocateUnicodeString)
#pragma alloc_text(PAGE, DfFreeUnicodeString)

BOOLEAN
FindSubString(IN PUNICODE_STRING String, IN PUNICODE_STRING SubString);

#endif /* end of include guard: COMMON_H_EP6FTA5H */

