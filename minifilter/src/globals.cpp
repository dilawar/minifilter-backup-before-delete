/**
 * Global variables.
 */

#include "globals.h"

//
// Handles
PFLT_FILTER gFilterHandle = nullptr;

// Current process name.
UNICODE_STRING gCurrentProcess = { 0 };

// Volume GUID.
UNICODE_STRING gVolumeGUID = { 0 };

//
// Trace flags.
#if 0
ULONG gTraceFlags = DFDBG_TRACE_ROUTINES;
#else
ULONG gTraceFlags = ERROR;
#endif

extern "C"
{
    NTSTATUS
    DfAllocateUnicodeString(_Inout_ PUNICODE_STRING String)
    {
        PAGED_CODE();

        ASSERT(NULL != String);
        ASSERT(0 != String->MaximumLength);

        String->Length = 0;

        String->Buffer = static_cast<PWSTR>(ExAllocatePoolWithTag(
          DF_CONTEXT_POOL_TYPE, String->MaximumLength, DF_STRING_POOL_TAG));

        if (NULL == String->Buffer) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        return STATUS_SUCCESS;
    }

    VOID
    DfFreeUnicodeString(_Inout_ PUNICODE_STRING String)
    {
        PAGED_CODE();

        if (NULL == String)
            return;

        if (0 == String->MaximumLength)
            return;

        String->Length = 0;

        if (NULL != String->Buffer) {

            String->MaximumLength = 0;
            ExFreePoolWithTag(String->Buffer, DF_STRING_POOL_TAG);
            String->Buffer = NULL;
        }
    }
} // extern C

/*++

  Routine Description:
  This routine looks to see if SubString is a substring of String.

  Arguments:
  String - the string to search in
  SubString - the substring to find in String

  Return Value:
  Returns TRUE if the substring is found in string and FALSE
  otherwise.

  --*/
BOOLEAN
FindSubString(IN PUNICODE_STRING String, IN PUNICODE_STRING SubString)
{
    ULONG index;

    //
    // First, check to see if the strings are equal.
    //

    if (RtlEqualUnicodeString(String, SubString, TRUE)) {
        return TRUE;
    }

    //
    // String and SubString aren't equal, so now see if SubString
    // is in String any where.
    //

    for (index = 0; index + SubString->Length / sizeof(WCHAR) <=
                    String->Length / sizeof(WCHAR);
         index++) {
        if (_wcsnicmp(&String->Buffer[index],
                      SubString->Buffer,
                      SubString->Length / sizeof(WCHAR)) == 0) {
            //
            // SubString is found in String, so return TRUE.
            //
            return TRUE;
        }
    }

    return FALSE;
}

/* Global functions */

