// Explicit private CoE5SE 3.2.2 integration. This is not the stock loader.
// Admit one exact locally built plugin; keep the verified file locked so it
// cannot be replaced between admission and the loader's normal approval path.
#include "fps_mod_hash.h"
static HANDLE fps_mod_lock=INVALID_HANDLE_VALUE;
static void findExtraPlugins(void) {}
static int isExtraPlugin(const FoundPlugin *p) {
    if(!p || _stricmp(p->name,"FPSofElysiumSE.c5plugin.dll") ||
       _stricmp(p->mod,"FPSofElysiumSE") || _stricmp(p->source,"local"))return 0;
    if(fps_mod_lock!=INVALID_HANDLE_VALUE)return 0;
    HANDLE file=lockDllFile(p->path);char digest[SHA256_HEX_LEN+1];
    if(!hashFileSha256(file,digest) || _stricmp(digest,FPS_MOD_EXPECTED_SHA256)) {unlockDllFile(file);return 0;}
    fps_mod_lock=file;return 1;
}
