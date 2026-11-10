// FPSofElysiumSE: rendering-only CoE5SE API 11 plugin, pinned to CoE5 5.43.
// No camera, quality, turn, timer, sorter or worker-pool changes.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jazzy_api.h"

static const unsigned char expected_sha256[32]={
    0x48,0x9d,0x0b,0x82,0xfa,0x79,0x0e,0xc3,0x6e,0x69,0xa2,0x3e,0x23,0x35,0xd0,0xc2,
    0x04,0x8f,0xa5,0x7f,0x40,0x51,0xfe,0x81,0x59,0xc6,0x4f,0x2d,0x33,0x50,0x8e,0x55
};
static const PluginApi *plugin_api;
static unsigned char *detail_base;
static _Thread_local int in_highest;
static long long now_ticks(void) {return 0;}
#include "game_guard.h"
#include "patch_calls.h"
#include "gl_batch_lab.h"
#include "gl_atlas_lab.h"
#include "gl_texture_array_lab.h"
#include "gl_arrays_lab.h"

typedef void (*ModDecorFn)(int,int,uintptr_t,float,float,float,float,float);
static void (*mod_original_highest)(void);
static ModDecorFn mod_original_decor;
static const int *mod_setting_enabled;
static int mod_operational,mod_reported_ready,mod_reported_fallback;
static int mod_reported_setting=-1;

static void mod_log(const char *line) {
    if(plugin_api && plugin_api->logLine)plugin_api->logLine((const void *)mod_log,line);
}
static void mod_decor(int file,int sprite,uintptr_t unused,float x,float y,float width,float height,float skew) {
    decor_scope=in_highest;
    mod_original_decor(file,sprite,unused,x,y,width,height,skew);
    decor_scope=0;
}
static void mod_highest(void) {
    if(!mod_setting_enabled && plugin_api && plugin_api->setting)
        mod_setting_enabled=plugin_api->setting("enabled");
    int enabled=mod_setting_enabled && *mod_setting_enabled;
    if(mod_reported_setting!=enabled) {
        mod_reported_setting=enabled;
        mod_log(enabled?"fps_mod setting enabled=1":"fps_mod setting enabled=0");
    }
    if(!mod_operational || !enabled) {
        mod_original_highest();return;
    }
    atlas_begin_frame();array_texture_begin_frame();arrays_begin_frame();
    in_highest=1;mod_original_highest();arrays_end_frame();batch_flush();in_highest=0;decor_scope=0;
    if(!mod_reported_ready && ta_context_ok && ta_uploads) {
        mod_reported_ready=1;mod_log("fps_mod renderer_ready game=5.43 api=11 original_uv=1 original_mips=1 quality_unchanged=1");
    }
    if(!mod_reported_fallback && ((ta_context_attempted && !ta_context_ok) || ta_rejected || (ta_state_checked && !ta_state_ok))) {
        char line[1200];mod_reported_fallback=1;
        snprintf(line,sizeof(line),"fps_mod renderer_fallback rejected_textures=%lld status=%s",ta_rejected,ta_status);
        mod_log(line);
    }
}

typedef struct {
    uintptr_t site,target;
    void (*hook)(void);
    unsigned char original[5];
} ModCall;
typedef struct { unsigned char *page;SIZE_T size;DWORD original; } ModCodePage;
static ModCall mod_calls[]={
    {0x19d621,0x1ab380,mod_highest,{0}},
    {0x1ab7c1,0x063ec0,(void (*)(void))mod_decor,{0}},
    {0x1ab9f1,0x063ec0,(void (*)(void))mod_decor,{0}}
};
static int mod_verify_calls(void) {
    for(size_t i=0;i<sizeof(mod_calls)/sizeof(mod_calls[0]);i++) {
        ModCall *c=&mod_calls[i];unsigned char *site=detail_base+c->site;int32_t rel=0;
        memcpy(&rel,site+1,4);
        if(site[0]!=0xe8 || site+5+rel!=detail_base+c->target)return 0;
        memcpy(c->original,site,5);
    }
    return 1;
}
static int mod_restore_page_protections(ModCodePage *pages,size_t count) {
    int ok=1;DWORD ignored;
    while(count) {ModCodePage *p=&pages[--count];if(!VirtualProtect(p->page,p->size,p->original,&ignored))ok=0;}
    return ok;
}
static int mod_rollback_calls(ModCodePage *pages,size_t count) {
    int writable=1;DWORD ignored;
    for(size_t i=0;i<count;i++)if(!VirtualProtect(pages[i].page,pages[i].size,PAGE_EXECUTE_READWRITE,&ignored))writable=0;
    if(writable) {
        for(size_t i=0;i<sizeof(mod_calls)/sizeof(mod_calls[0]);i++) {
            unsigned char *site=detail_base+mod_calls[i].site;memcpy(site,mod_calls[i].original,5);
            FlushInstructionCache(GetCurrentProcess(),site,5);
        }
    }
    int restored=mod_restore_page_protections(pages,count);
    if(!writable || !restored)mod_log("fps_mod disabled reason=rollback_or_page_protection_failed module_retained=1");
    return writable;
}
static int mod_install_calls(void) {
    // All three callsites are verified before any page or instruction changes.
    if(!mod_verify_calls())return 0;
    unsigned char *stubs=allocate_near((uintptr_t)(detail_base+mod_calls[0].site));if(!stubs)return 0;
    const size_t count=sizeof(mod_calls)/sizeof(mod_calls[0]);
    for(size_t i=0;i<count;i++) {
        intptr_t delta=(intptr_t)(stubs+i*16)-(intptr_t)(detail_base+mod_calls[i].site+5);
        if(delta<INT32_MIN || delta>INT32_MAX){VirtualFree(stubs,0,MEM_RELEASE);return 0;}
        write_jump(stubs+i*16,mod_calls[i].hook);
    }
    DWORD old,ignored;
    if(!VirtualProtect(stubs,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(stubs,0,MEM_RELEASE);return 0;}
    FlushInstructionCache(GetCurrentProcess(),stubs,count*16);
    SYSTEM_INFO system;GetSystemInfo(&system);
    ModCodePage pages[3];size_t page_count=0,opened=0;
    for(size_t i=0;i<count;i++) {
        unsigned char *page=(unsigned char *)((uintptr_t)(detail_base+mod_calls[i].site)&~((uintptr_t)system.dwPageSize-1));
        size_t j=0;while(j<page_count && pages[j].page!=page)j++;
        if(j==page_count)pages[page_count++]=(ModCodePage){page,system.dwPageSize,0};
    }
    // Acquire every relevant page before writing the first call.
    for(;opened<page_count;opened++)if(!VirtualProtect(pages[opened].page,pages[opened].size,PAGE_EXECUTE_READWRITE,&pages[opened].original))break;
    if(opened!=page_count) {
        if(!mod_restore_page_protections(pages,opened))mod_log("fps_mod disabled reason=page_protection_restore_failed");
        VirtualFree(stubs,0,MEM_RELEASE);return 0;
    }
    int ok=1;
    for(size_t i=0;i<count;i++) {
        unsigned char *site=detail_base+mod_calls[i].site;
        if(!write_relative_call(site,stubs+i*16)){ok=0;break;}
        FlushInstructionCache(GetCurrentProcess(),site,5);
    }
    if(ok && !mod_restore_page_protections(pages,page_count))ok=0;
    if(!ok) {
        // Keep stubs alive if rollback could not remove every possible target.
        if(mod_rollback_calls(pages,page_count))VirtualFree(stubs,0,MEM_RELEASE);
        return 0;
    }
    (void)ignored;return 1;
}
static int mod_initialize_original_gl(void) {
    HMODULE gl=GetModuleHandleA("opengl32.dll");if(!gl)return 0;
#define MOD_GL(var,name) do {*(void **)&var=(void *)GetProcAddress(gl,name);if(!var)return 0;}while(0)
    MOD_GL(real_gl_begin,"glBegin");MOD_GL(real_gl_end,"glEnd");MOD_GL(real_gl_bind,"glBindTexture");
    MOD_GL(real_gl_color,"glColor4f");MOD_GL(real_gl_color3,"glColor3f");MOD_GL(real_gl_enable,"glEnable");
    MOD_GL(real_gl_disable,"glDisable");MOD_GL(real_gl_blend,"glBlendFunc");MOD_GL(real_gl_alpha,"glAlphaFunc");
    MOD_GL(real_gl_depth_mask,"glDepthMask");MOD_GL(real_gl_tex_env,"glTexEnvi");
#undef MOD_GL
    return 1;
}
static void mod_disable(void) {
    mod_operational=0;arrays_enabled=0;texture_array_enabled=0;atlas_enabled=0;
    batch_enabled=0;color_cache_enabled=0;pixel_oracle=0;in_highest=0;decor_scope=0;
}
COE5SE_PLUGIN_LOAD(api) {
    if(!api || api->version!=PLUGIN_API_VERSION || !api->logLine || !api->setting)return 0;
    plugin_api=api;detail_base=(unsigned char *)GetModuleHandleW(NULL);
    mod_original_highest=(void (*)(void))(detail_base+0x1ab380);
    mod_original_decor=(ModDecorFn)(detail_base+0x63ec0);
    // Silence unused aggregate profiler declarations; no timing code is included.
    (void)skipped_sum;(void)end_sum;(void)bind_sum;(void)color_sum;
    if(!game_hash_matches() || !mod_verify_calls()) {mod_disable();mod_log("fps_mod disabled reason=game_hash_or_call_signature_mismatch");return 1;}
    if(!mod_initialize_original_gl()) {mod_disable();mod_log("fps_mod disabled reason=original_gl_exports_unavailable");return 1;}
    if(!install_gl_batch() || !install_gl_atlas() || !install_texture_array() || !install_gl_arrays()) {
        mod_disable();mod_log("fps_mod disabled reason=gl_hook_installation_failed module_retained=1");return 1;
    }
    if(!mod_install_calls()) {
        mod_disable();mod_log("fps_mod disabled reason=call_patch_transaction_failed module_retained=1");return 1;
    }
    mod_operational=1;mod_log("fps_mod initialized game=5.43 api=11 runtime_setting=enabled quality_unchanged=1");return 1;
}
