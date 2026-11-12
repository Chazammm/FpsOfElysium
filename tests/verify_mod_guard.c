#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "jazzy_api.h"
static int mismatch,settings_called;
static void log_probe(const void *owner,const char *line) {(void)owner;if(strstr(line,"unsupported_game_hash"))mismatch++;}
static const int *setting_probe(const char *key) {(void)key;settings_called++;return NULL;}
int main(void) {
    HMODULE library=LoadLibraryW(L"FPSofElysiumSE.c5plugin.dll");
    if(!library)return 1;
    int (*load)(const PluginApi *)=(int (*)(const PluginApi *))GetProcAddress(library,"coe5sePluginLoad");
    const int *version=(const int *)GetProcAddress(library,"coe5sePluginApiVersion");
    if(!load || !version || *version!=11)return 2;
    PluginApi api={0};api.version=11;api.logLine=log_probe;api.setting=setting_probe;
    if(load(NULL)!=0)return 3;
    api.version=10;if(load(&api)!=0 || mismatch)return 4;
    api.version=11;
    if(load(&api)!=1 || mismatch!=1 || settings_called)return 5;
    puts("PASS: API/null guards and foreign-executable refusal; no setting or rendering initialization reached.");
    FreeLibrary(library);return 0;
}
