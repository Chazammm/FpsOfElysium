#define FPS_SE_MOD
#include "fps_mod.c"
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int problems,lines;
static char last_line[2400];
static int menu_calls;
static int fake_menu_wait(int a,int b){menu_calls++;return a*3+b;}
static int section_calls,checkbox_calls,tip_calls,wait_toggle,frame_wait_id,choice_calls,choice_selected,choice_bad,original_choice_calls;
static void fake_section_end(void){section_calls++;}
static int map_forward_calls;
static void fake_map_draw(void){map_forward_calls++;}
static void fake_checkbox(int id,int *value,const char *label){if(id==1 && value==fps_gpu_wait && !strcmp(label,"Wait for GPU each frame"))checkbox_calls++;}
static void fake_tooltip(const char *text){if(strstr(text,"not VSync"))tip_calls++;}
static int fake_preferences_wait(int a,int b){if(wait_toggle)*fps_gpu_wait=!*fps_gpu_wait;return frame_wait_id?frame_wait_id:a*5+b;}
static int fake_original_choice(int id,int selected,const char *label,...){if(id==408 && selected==0 && !strcmp(label,"120 Frames per second"))original_choice_calls++;return 731;}
static void fake_choice(int id,int selected,const char *label){
    size_t i=(size_t)choice_calls++;
    if(i>=sizeof(fps_frame_options)/sizeof(fps_frame_options[0])){choice_bad++;return;}
    if(id!=400+fps_frame_options[i].interval || strcmp(label,fps_frame_options[i].label))choice_bad++;
    choice_selected+=selected;
    if(selected && fps_frame_options[i].interval!=*fps_frame_interval)choice_bad++;
}
static void problem(const char *name,uintptr_t rva,void *actual,void *expected) {(void)name;(void)rva;(void)actual;(void)expected;problems++;}
static void capture(const void *self,const char *line) {(void)self;lines++;snprintf(last_line,sizeof(last_line),"%s",line);}
int main(void) {
    HMODULE gl=LoadLibraryA("opengl32.dll");CHECK(gl);
    wchar_t system[32768];UINT n=GetSystemDirectoryW(system,32768);CHECK(n && n<32750);
    wcscat(system,L"\\glu32.dll");HMODULE glu=LoadLibraryW(system);CHECK(glu);
    void *mip=(void *)GetProcAddress(glu,"gluBuild2DMipmaps");CHECK(mip && fps_expected_glu()==mip);
    size_t bytes=0x33b000;unsigned char *base=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,bytes),*snapshot=HeapAlloc(GetProcessHeap(),0,bytes);CHECK(base && snapshot);
    size_t count=sizeof(arrays_imports)/sizeof(arrays_imports[0]);
    for(size_t i=0;i<count;i++){void *native=(void *)GetProcAddress(gl,arrays_imports[i].name);CHECK(native);memcpy(base+arrays_imports[i].rva,&native,sizeof(native));}
    memcpy(base+0x33a000,&mip,sizeof(mip));
    CHECK(fps_check_gl_imports(base,gl,mip,problem) && !problems);
    for(size_t i=0;i<=count;i++) {
        uintptr_t rva=i<count?arrays_imports[i].rva:0x33a000;
        void *original=NULL,*foreign=(void *)(uintptr_t)0x12345678;
        memcpy(&original,base+rva,sizeof(original));memcpy(base+rva,&foreign,sizeof(foreign));memcpy(snapshot,base,bytes);
        problems=0;CHECK(!fps_check_gl_imports(base,gl,mip,problem) && problems==1);CHECK(!memcmp(snapshot,base,bytes));
        memcpy(base+rva,&original,sizeof(original));CHECK(fps_check_gl_imports(base,gl,mip,NULL));
    }
    PluginApi api={0};api.version=PLUGIN_API_VERSION;api.logLine=capture;plugin_api=&api;
    mod_import_problem("glBegin",0x33a3e0,(void *)(uintptr_t)1,(void *)GetProcAddress(gl,"glBegin"));
    CHECK(strstr(last_line,"graphics_import_conflict") && strstr(last_line,"glBegin") && strstr(last_line,"imports_unchanged=1"));
    detail_base=base;
    for(size_t i=0;i<sizeof(mod_calls)/sizeof(mod_calls[0]);i++) {
        int32_t relative=(int32_t)(mod_calls[i].target-mod_calls[i].site-5);
        base[mod_calls[i].site]=0xe8;memcpy(base+mod_calls[i].site+1,&relative,4);
    }
    CHECK(mod_verify_calls());
    for(size_t i=0;i<sizeof(mod_calls)/sizeof(mod_calls[0]);i++) {
        unsigned char *site=base+mod_calls[i].site;site[0]=0xe9;memcpy(snapshot,base,bytes);
        CHECK(!mod_verify_calls() && !memcmp(snapshot,base,bytes));CHECK(strstr(last_line,"game_callsite_changed"));
        site[0]=0xe8;site[1]^=1;CHECK(!mod_verify_calls());site[1]^=1;CHECK(mod_verify_calls());
    }
    lines=0;fps_fallback_reasons=0;mod_reported_reasons=0;
    fps_note_fallback(FPS_TEXTURE_SIZE);fps_note_fallback(FPS_VAO_BOUND);mod_log_fallbacks();CHECK(lines==2);
    mod_log_fallbacks();CHECK(lines==2);
    fps_note_fallback(FPS_ALLOCATION);mod_log_fallbacks();CHECK(lines==3 && strstr(last_line,"texture_allocation_failed"));
    for(unsigned i=0;i<FPS_REASON_COUNT;i++)CHECK(strcmp(fps_fallback_name((FpsFallbackReason)i),"unknown"));
    mod_next_main_menu_wait=fake_menu_wait;mod_reported_graphics=1;
    CHECK(mod_main_menu_wait(7,11)==32 && menu_calls==1);
    wchar_t *arguments[]={L"CoE5.exe",L"--maxfps=240",L"--glfinish",L"--noglfinish",L"--glfinish"};
    CHECK(fps_gpu_wait_argument(2,arguments)==-1 && fps_gpu_wait_argument(3,arguments)==1);
    CHECK(fps_gpu_wait_argument(4,arguments)==0 && fps_gpu_wait_argument(5,arguments)==1);
    CHECK(fps_gpu_wait_preference(L"0")==0 && fps_gpu_wait_preference(L"1")==1);
    CHECK(fps_gpu_wait_preference(L"")==-1 && fps_gpu_wait_preference(L"2")==-1 && fps_gpu_wait_preference(L"01")==-1);
    CHECK(fps_frame_argument(2,arguments) && !fps_frame_argument(1,arguments));
    wchar_t *unrelated[]={L"CoE5.exe",L"--maxfps",L"prefix--maxfps=240"};CHECK(!fps_frame_argument(3,unrelated));
    CHECK(fps_frame_preference(L"0")==0 && fps_frame_preference(L"4")==4 && fps_frame_preference(L"200")==200);
    CHECK(fps_frame_preference(L"")==-1 && fps_frame_preference(L"01")==-1 && fps_frame_preference(L"201")==-1);
    CHECK(fps_frame_preference(L"-1")==-1 && fps_frame_preference(L"4x")==-1 && fps_frame_preference(L"999999999999999999999")==-1);
    UiApi ui={0};ui.checkbox=fake_checkbox;ui.tooltip=fake_tooltip;ui.choice=fake_choice;api.ui=&ui;
    int gpu_wait=0;fps_gpu_wait=&gpu_wait;fps_graphics_settings_active=1;
    int frame_interval=4;fps_frame_interval=&frame_interval;
    fps_next_map_draw=fake_map_draw;fps_pending_frame_interval=0;
    fps_frame_preference_map_draw();CHECK(frame_interval==3 && fps_pending_frame_interval==-1 && map_forward_calls==1);
    frame_interval=4;fps_frame_preference_map_draw();CHECK(frame_interval==4 && map_forward_calls==2);
    fps_next_frame_rate_choice=fake_original_choice;
    CHECK(fps_frame_rate_choices(408,0,"120 Frames per second")==731);
    CHECK(original_choice_calls==1 && choice_calls==4 && choice_selected==1 && !choice_bad);
    for(size_t i=0;i<sizeof(fps_frame_options)/sizeof(fps_frame_options[0]);i++)CHECK(fps_frame_options[i].interval>=3);
    fps_next_graphics_section_end=fake_section_end;fps_next_preferences_wait=fake_preferences_wait;
    fps_graphics_section_end();CHECK(section_calls==1 && checkbox_calls==1 && tip_calls==1 && gpu_wait==0);
    wchar_t temporary[MAX_PATH],readback[8];CHECK(GetTempFileNameW(L".",L"fps",0,temporary));
    CHECK(GetFullPathNameW(temporary,MAX_PATH,fps_graphics_ini,NULL));
    frame_interval=0;fps_apply_frame_preference();CHECK(frame_interval==3);
    CHECK(GetPrivateProfileStringW(L"Graphics",L"FrameIntervalMs",L"",readback,8,fps_graphics_ini)==1 && !wcscmp(readback,L"3"));
    CHECK(strstr(last_line,"legacy_unlimited_migrated"));
    frame_interval=0;fps_apply_frame_preference();CHECK(frame_interval==3);
    frame_interval=4;
    lines=0;CHECK(fps_preferences_wait(7,11)==46 && lines==0 && gpu_wait==0);
    wait_toggle=1;CHECK(fps_preferences_wait(7,11)==46 && gpu_wait==1 && lines==1);
    CHECK(GetPrivateProfileStringW(L"Graphics",L"WaitForGpu",L"",readback,8,fps_graphics_ini)==1 && !wcscmp(readback,L"1"));
    CHECK(strstr(last_line,"saved=1"));
    CHECK(fps_preferences_wait(2,3)==13 && gpu_wait==0 && lines==2);
    CHECK(GetPrivateProfileStringW(L"Graphics",L"WaitForGpu",L"",readback,8,fps_graphics_ini)==1 && !wcscmp(readback,L"0"));
    wait_toggle=0;
    for(size_t i=0;i<sizeof(fps_frame_options)/sizeof(fps_frame_options[0]);i++) {
        int interval=fps_frame_options[i].interval;frame_wait_id=400+interval;
        CHECK(fps_preferences_wait(7,11)==frame_wait_id && frame_interval==4);
        CHECK(GetPrivateProfileStringW(L"Graphics",L"FrameIntervalMs",L"",readback,8,fps_graphics_ini));
        CHECK(fps_frame_preference(readback)==interval && strstr(last_line,"saved=1"));
    }
    frame_wait_id=416;CHECK(fps_preferences_wait(7,11)==416);
    CHECK(GetPrivateProfileStringW(L"Graphics",L"FrameIntervalMs",L"",readback,8,fps_graphics_ini)==2 && !wcscmp(readback,L"16"));
    int previous_lines=lines;frame_wait_id=500;CHECK(fps_preferences_wait(7,11)==500 && lines==previous_lines);
    frame_wait_id=400;CHECK(fps_preferences_wait(7,11)==403);
    CHECK(GetPrivateProfileStringW(L"Graphics",L"FrameIntervalMs",L"",readback,8,fps_graphics_ini)==1 && !wcscmp(readback,L"3"));
    CHECK(DeleteFileW(fps_graphics_ini));fps_graphics_ini[0]=0;
    fps_graphics_settings_active=0;fps_graphics_section_end();CHECK(section_calls==2 && checkbox_calls==1);
    CHECK(fps_frame_rate_choices(408,0,"120 Frames per second")==731 && choice_calls==4 && original_choice_calls==2);
    ta_forbidden=0;ta_texture_on=1;ta_modulate=1;
    const unsigned tracked_caps[]={0x0b50,0x0b57,0x0b60,0x0de0,0x806f,0x8513,0x8458,0x3000,0x3001,0x3002,0x3003,0x3004,0x3005,0x0c60,0x0c61,0x0c62,0x0c63};
    CHECK(ta_switches_allowed());
    for(size_t i=0;i<sizeof(tracked_caps)/sizeof(tracked_caps[0]);i++) {
        ta_note_cap(tracked_caps[i],1);CHECK(!ta_switches_allowed());
        ta_note_cap(tracked_caps[i],0);CHECK(ta_switches_allowed());
    }
    ta_note_cap(0x0de1,0);CHECK(!ta_switches_allowed());ta_note_cap(0x0de1,1);CHECK(ta_switches_allowed());
    ta_track_state=1;ta_state_checked=1;arrays_matrix_mode=0x1702;arrays_matrix_changed();CHECK(!ta_state_checked);
    ta_state_checked=1;arrays_matrix_mode=0x1700;arrays_matrix_changed();CHECK(ta_state_checked);
    HeapFree(GetProcessHeap(),0,snapshot);HeapFree(GetProcessHeap(),0,base);FreeLibrary(glu);FreeLibrary(gl);
    printf("PASS: %u GL/GLU import conflicts and 6 changed callsites rejected without writes; bounded frame choices, uncapped-setting migration, persistence, legacy selection and argument validation.\n",(unsigned)count+1);
    return 0;
}
