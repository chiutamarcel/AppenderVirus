/**
*	resource copier.
*	copies first 32 icons, icon group and version
*	information from target executable to destination executable
*	
*	(c) vzlomshikzloy 
*
**/

#include <windows.h>
#include <stdio.h>

#define MAX_RESOURCES 512

typedef enum {
    CODE_NULL,
    CODE_RT_ICON,
    CODE_RT_GROUP_ICON,
    CODE_RT_VERSION,
} TYPE_CODES;

typedef struct {
    int index;
    int type;
    DWORD dwSize;
    BYTE* resRaw;
} RESDATA;

RESDATA resources[MAX_RESOURCES];
int rightResources = 0;

void RestoreResources(LPSTR targetFile);

void pushResources(LPVOID lpResData, int index, int type, DWORD dwSize) {
    if (rightResources >= MAX_RESOURCES) {
        return;
    }

    RESDATA resData;
    resData.index = index;
    resData.type = type;
    resData.resRaw = (BYTE*)malloc(dwSize);
    memcpy(resData.resRaw, lpResData, dwSize);
    resData.dwSize = dwSize;

    resources[rightResources++] = resData;
}

void SaveOldResources(LPSTR targetFile, DWORD Check) {
    HMODULE hExe;
    HRSRC hRes;
    DWORD i,rCount;


    hExe = LoadLibraryExA(targetFile,0,LOAD_LIBRARY_AS_DATAFILE);
    if(hExe == NULL)
    {	
        return;
    }
    
    for(i=0;i<Check;i++){
        hRes = FindResourceA(hExe, MAKEINTRESOURCEA(i), RT_GROUP_ICON);
        if(hRes!=NULL){
            pushResources(LockResource(LoadResource(hExe, hRes)), i, CODE_RT_GROUP_ICON, SizeofResource(hExe, hRes));
        }
    }
    hRes=NULL;


    for(i=0;i<Check;i++){		
        if(FindResourceA(hExe, MAKEINTRESOURCEA(i), RT_ICON)!=NULL)
        { 
            i--;
            for(rCount=i;rCount<(i+32);rCount++)
            {
                hRes = FindResourceA(hExe,MAKEINTRESOURCEA(rCount),RT_ICON);
                if(hRes!=NULL)
                {
                    pushResources(LockResource(LoadResource(hExe, hRes)), rCount, CODE_RT_ICON, SizeofResource(hExe, hRes));
                }
            }
        break;
        }

    }hRes=NULL;

    for(i=0;i<Check;i++){
    hRes = FindResourceA(hExe,MAKEINTRESOURCEA(i),RT_VERSION);
        if(hRes!=NULL){
            pushResources(LockResource(LoadResource(hExe, hRes)), i, CODE_RT_VERSION, SizeofResource(hExe, hRes));
            break;
        }
    }

    FreeLibrary(hExe);
}

void RestoreResources(LPSTR targetFile) {
    HMODULE hExe;
    HANDLE hUpdateRes;

    hExe = LoadLibraryExA(targetFile,0,LOAD_LIBRARY_AS_DATAFILE);
    if(hExe == NULL)
    {	
        return;
    }

    hUpdateRes = BeginUpdateResourceA(targetFile, FALSE);
    if(hUpdateRes == NULL)
    {
        FreeLibrary(hExe);
        return;
    }

    DWORD i,rCount;


    for(i=0;i<rightResources;i++){
        // printf("i: %d\n", i);
        RESDATA resData = resources[i];

        switch(resData.type) {
            case CODE_RT_GROUP_ICON:
                if(!UpdateResourceA(hUpdateRes,RT_GROUP_ICON,MAKEINTRESOURCEA(resData.index),MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),resData.resRaw,resData.dwSize)
                ) {
                    fprintf(stderr, "Error on UpdateResourceA: %d", GetLastError());
                    exit(EXIT_FAILURE);
                }
                break;
            case CODE_RT_ICON:
                if(!UpdateResourceA(hUpdateRes,RT_ICON,MAKEINTRESOURCEA(resData.index),MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),resData.resRaw,resData.dwSize)
                ) {
                    fprintf(stderr, "Error on UpdateResourceA: %d", GetLastError());
                    exit(EXIT_FAILURE);
                }
                break;
            case CODE_RT_VERSION:
                if(!UpdateResourceA(hUpdateRes,RT_VERSION,MAKEINTRESOURCEA(resData.index),MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),resData.resRaw,resData.dwSize)
                ) {
                    fprintf(stderr, "Error on UpdateResourceA: %d", GetLastError());
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Invalid resource type");
                exit(EXIT_FAILURE);
                break;
        }

    }


    EndUpdateResourceA(hUpdateRes, FALSE);

    FreeLibrary(hExe);
}