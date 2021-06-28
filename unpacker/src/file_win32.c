#include <windows.h>

#include <stdio.h>
#include <string.h>

#include "rucksack/unpacker/unpacker.h"

void rucksack_unpacker_create_directory(const uint16_t *name, uint32_t attributes) {
    size_t name_length = wcslen(name);
    wchar_t *temporary_buffer = malloc((name_length + 1) * sizeof(wchar_t));
    wcscpy_s(temporary_buffer, name_length + 1, name);

    if(temporary_buffer[name_length - 1] == L'/') {
        temporary_buffer[name_length - 1] = L'\0';
    }

    for(wchar_t *search = temporary_buffer + 1; *search; search++) {
        if(*search == L'/') {
            *search = L'\0';
            if(!CreateDirectoryW(temporary_buffer, NULL)) {
                DWORD err = GetLastError();

                if(err != ERROR_ALREADY_EXISTS) {
                    fprintf(stderr, "CreateDirectoryW failed\n");
                    exit(1);
                }
            }
            *search = L'/';
        }
    }

    if(!CreateDirectoryW(temporary_buffer, NULL)) {
        DWORD err = GetLastError();

        if(err != ERROR_ALREADY_EXISTS) {
            fprintf(stderr, "CreateDirectoryW failed\n");
            exit(1);
        }
    }

    if((attributes & 0xF0000000) != 0) {
        attributes &= 0x3FFF;
    }

    SetFileAttributesW(temporary_buffer, attributes);

    free(temporary_buffer);
}

void rucksack_unpacker_write_file(const uint16_t *name, const void *data, size_t size, uint32_t attributes) {
    HANDLE opened_file = CreateFileW(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if(opened_file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "CreateFileW failed\n");
        exit(1);
    }

    DWORD num_written;
    BOOL has_been_written = WriteFile(opened_file, data, size, &num_written, NULL);
    CloseHandle(opened_file);

    if(!has_been_written) {
        fprintf(stderr, "WriteFile failed\n");
        exit(1);
    } else if(num_written != size) {
        fprintf(stderr, "Expected to write %lu bytes, but only wrote %zu bytes\n", num_written, size);
        exit(1);
    }

    if((attributes & 0xF0000000) != 0) {
        attributes &= 0x3FFF;
    }

    SetFileAttributesW(name, attributes);
}

void rucksack_unpacker_execute_file(const char *name, const char *working_dir) {
    STARTUPINFOA startup_info;
    ZeroMemory(&startup_info, sizeof(STARTUPINFOA));
    startup_info.cb = sizeof(STARTUPINFOA);

    PROCESS_INFORMATION process_information;
    ZeroMemory(&process_information, sizeof(PROCESS_INFORMATION));

    BOOL ok = CreateProcessA(name, NULL, NULL, NULL, FALSE, 0, NULL, working_dir, &startup_info, &process_information);
    if(!ok) {
        fprintf(stderr, "CreateProcessA failed\n");
        exit(1);
    }

    WaitForSingleObject(process_information.hProcess, INFINITE);
    CloseHandle(process_information.hThread);
    CloseHandle(process_information.hProcess);
}
