#pragma once
// The actual installation table is the source of truth. This preflight only
// reads import slots, and must run before install_gl_batch writes any of them.
typedef void (*FpsImportProblem)(const char *name,uintptr_t rva,void *actual,void *expected);
static int fps_check_gl_imports(unsigned char *base,HMODULE gl,void *expected_glu,FpsImportProblem report) {
    if(!base || !gl)return 0;
    for(size_t i=0;i<sizeof(arrays_imports)/sizeof(arrays_imports[0]);i++) {
        FpsGlImport *entry=&arrays_imports[i];void *actual=NULL;
        void *expected=(void *)GetProcAddress(gl,entry->name);
        memcpy(&actual,base+entry->rva,sizeof(actual));
        if(!expected || actual!=expected) {
            if(report)report(entry->name,entry->rva,actual,expected);
            return 0;
        }
    }
    void *actual=NULL;memcpy(&actual,base+0x33a000,sizeof(actual));
    if(!expected_glu || actual!=expected_glu) {
        if(report)report("gluBuild2DMipmaps",0x33a000,actual,expected_glu);
        return 0;
    }
    return 1;
}
static void *fps_expected_glu(void) {
    wchar_t path[32768];DWORD length=GetModuleFileNameW(NULL,path,32768);
    if(length && length<32750) {
        wchar_t *slash=wcsrchr(path,L'\\');
        if(slash) {
            wcscpy(slash+1,L"glu32.dll");HMODULE module=GetModuleHandleW(path);
            if(module)return (void *)GetProcAddress(module,"gluBuild2DMipmaps");
        }
    }
    UINT length_system=GetSystemDirectoryW(path,32768);
    if(!length_system || length_system>=32750)return NULL;
    wcscat(path,L"\\glu32.dll");HMODULE module=GetModuleHandleW(path);
    return module?(void *)GetProcAddress(module,"gluBuild2DMipmaps"):NULL;
}
