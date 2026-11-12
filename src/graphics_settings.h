// CoE5 5.43's native graphics controls. No changes to drawing or swap interval.
#include <shellapi.h>
#include <wchar.h>

static int *fps_gpu_wait;
static int *fps_frame_interval;
static int fps_pending_frame_interval=-1;
static int fps_graphics_settings_active;
static wchar_t fps_graphics_ini[MAX_PATH];
static void (*fps_next_graphics_section_end)(void);
static int (*fps_next_preferences_wait)(int,int);
static int (*fps_next_frame_rate_choice)(int,int,const char *,...);
static void (*fps_next_map_draw)(void);

typedef struct {int interval;const char *label,*tip;} FpsFrameOption;
static const FpsFrameOption fps_frame_options[]={
    {7,"144 Frames per second (approx.)","Approximate target: about 143 FPS before rendering and other limits. Applies immediately and is remembered. Explicit --maxfps launch options take priority on restart."},
    {6,"165 Frames per second (approx.)","Approximate target: about 167 FPS before rendering and other limits. Applies immediately and is remembered. Explicit --maxfps launch options take priority on restart."},
    {4,"240 Frames per second (approx.)","Approximate target: about 250 FPS before rendering and other limits. Applies immediately and is remembered. Explicit --maxfps launch options take priority on restart."},
    {3,"360 Frames per second (approx.)","Approximate target: about 333 FPS before rendering, scheduling and other limits. Actual FPS can be lower. Applies immediately and is remembered. Explicit --maxfps launch options take priority on restart."}
};
static int fps_frame_argument(int argc,wchar_t **argv) {
    for(int i=1;i<argc;i++)if(!wcsncmp(argv[i],L"--maxfps=",9))return 1;
    return 0;
}
static int fps_frame_preference(const wchar_t *value) {
    if(!value[0] || (value[0]==L'0' && value[1]))return -1;
    int result=0;
    for(size_t i=0;value[i];i++) {
        if(value[i]<L'0' || value[i]>L'9')return -1;
        result=result*10+value[i]-L'0';
        if(result>200)return -1;
    }
    return result;
}
static void fps_log_frame_limit(int interval,const char *source,int saved) {
    char line[240];
    snprintf(line,sizeof(line),"fps_mod frame_limit interval_ms=%d unlimited=%d source=%s saved=%d timing=native quality_unchanged=1",interval,interval==0,source,saved);
    mod_log(line);
}
static int fps_bounded_frame_interval(int interval) {return interval>0?interval:3;}
static int fps_store_frame_limit(int interval) {
    wchar_t value[16];swprintf(value,16,L"%d",fps_bounded_frame_interval(interval));
    return fps_graphics_ini[0] && WritePrivateProfileStringW(L"Graphics",L"FrameIntervalMs",value,fps_graphics_ini);
}
static void fps_apply_frame_preference(void) {
    if(!fps_graphics_settings_active)return;
    if(fps_pending_frame_interval>=0) {
        *fps_frame_interval=fps_bounded_frame_interval(fps_pending_frame_interval);
        fps_pending_frame_interval=-1;
        fps_log_frame_limit(*fps_frame_interval,"preference",0);
    } else if(*fps_frame_interval<=0) {
        *fps_frame_interval=3;
        fps_log_frame_limit(3,"legacy_unlimited_migrated",fps_store_frame_limit(3));
    }
}
static void fps_frame_preference_map_draw(void) {fps_apply_frame_preference();fps_next_map_draw();}
static void fps_save_frame_limit(int interval) {
    interval=fps_bounded_frame_interval(interval);
    int saved=fps_store_frame_limit(interval);
    fps_log_frame_limit(interval,"graphics_menu",saved);
}
static int fps_frame_rate_choices(int id,int selected,const char *label) {
    fps_apply_frame_preference();
    int result=fps_next_frame_rate_choice(id,selected,label);
    if(fps_graphics_settings_active) {
        for(size_t i=0;i<sizeof(fps_frame_options)/sizeof(fps_frame_options[0]);i++) {
            const FpsFrameOption *option=&fps_frame_options[i];
            plugin_api->ui->choice(400+option->interval,*fps_frame_interval==option->interval,option->label);
            plugin_api->ui->tooltip(option->tip);
        }
    }
    return result;
}

static int fps_gpu_wait_argument(int argc,wchar_t **argv) {
    int value=-1;
    for(int i=1;i<argc;i++) {
        if(!wcscmp(argv[i],L"--glfinish"))value=1;
        if(!wcscmp(argv[i],L"--noglfinish"))value=0;
    }
    return value;
}
static int fps_gpu_wait_preference(const wchar_t *value) {
    if(!wcscmp(value,L"0"))return 0;
    if(!wcscmp(value,L"1"))return 1;
    return -1;
}
static void fps_log_gpu_wait(const char *source,int saved) {
    char line[240];
    snprintf(line,sizeof(line),"fps_mod gpu_wait glfinish=%d source=%s saved=%d quality_unchanged=1 vsync_unchanged=1",*fps_gpu_wait,source,saved);
    mod_log(line);
}
static void fps_save_gpu_wait(void) {
    int saved=fps_graphics_ini[0] && WritePrivateProfileStringW(L"Graphics",L"WaitForGpu",*fps_gpu_wait?L"1":L"0",fps_graphics_ini);
    fps_log_gpu_wait("graphics_menu",saved);
}
static void fps_graphics_section_end(void) {
    if(fps_graphics_settings_active) {
        plugin_api->ui->checkbox(1,fps_gpu_wait,"Wait for GPU each frame");
        plugin_api->ui->tooltip("Extra GPU synchronization (glFinish). Off = --noglfinish; may improve performance. Graphics quality is unchanged. This is not VSync. Applies immediately and is remembered; explicit launch options take priority on restart.");
    }
    fps_next_graphics_section_end();
}
static int fps_preferences_wait(int a,int b) {
    int before=fps_graphics_settings_active?*fps_gpu_wait:0;
    int result=fps_next_preferences_wait(a,b);
    if(fps_graphics_settings_active && *fps_gpu_wait!=before)fps_save_gpu_wait();
    // The native preferences handler applies IDs 400..499 as interval=id-400
    // after this wait returns. Persist that selection without consuming the ID.
    if(fps_graphics_settings_active && result==400)result=403;
    if(fps_graphics_settings_active && result>=400 && result<500)fps_save_frame_limit(result-400);
    return result;
}
static void fps_initialize_graphics_settings(void) {
    if(!plugin_api->ui || !plugin_api->ui->checkbox || !plugin_api->ui->tooltip || !plugin_api->ui->choice) {
        mod_log("fps_mod graphics_menu unavailable reason=ui_api_missing");return;
    }
    fps_gpu_wait=(int *)(detail_base+0x58243ac);
    fps_frame_interval=(int *)(detail_base+0x1a5324c);
    if(*fps_gpu_wait!=0 && *fps_gpu_wait!=1) {
        mod_log("fps_mod graphics_menu unavailable reason=unexpected_glfinish_value");return;
    }
    HMODULE self=NULL;wchar_t path[MAX_PATH];
    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)(uintptr_t)fps_initialize_graphics_settings,&self)) {
        DWORD length=GetModuleFileNameW(self,path,MAX_PATH);
        if(length && length<MAX_PATH) {
            wchar_t *slash=wcsrchr(path,L'\\');
            if(slash && (size_t)(slash-path)+1+wcslen(L"FPSofElysiumSE.ini")<MAX_PATH) {
                slash[1]=0;wcscat(path,L"FPSofElysiumSE.ini");wcscpy(fps_graphics_ini,path);
            }
        }
    }
    int argc=0;wchar_t **argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    // Do not replace an explicit launch option with a remembered preference.
    int explicit_value=argv?fps_gpu_wait_argument(argc,argv):-1;
    int explicit_frame_limit=argv?fps_frame_argument(argc,argv):0;
    int arguments_available=argv!=NULL;
    if(argv)LocalFree(argv);
    wchar_t value[32]=L"";
    if(fps_graphics_ini[0])GetPrivateProfileStringW(L"Graphics",L"WaitForGpu",L"",value,32,fps_graphics_ini);
    int preference=fps_gpu_wait_preference(value);
    const char *source="game_default";
    if(explicit_value>=0){*fps_gpu_wait=explicit_value;source="launch_option";}
    else if(arguments_available && preference>=0){*fps_gpu_wait=preference;source="preference";}
    else if(value[0])mod_log("fps_mod graphics_menu ignored reason=invalid_saved_glfinish_value");
    fps_graphics_settings_active=1;fps_log_gpu_wait(source,0);
    value[0]=0;
    if(fps_graphics_ini[0])GetPrivateProfileStringW(L"Graphics",L"FrameIntervalMs",L"",value,32,fps_graphics_ini);
    preference=fps_frame_preference(value);source="game_default";
    if(preference==0) {
        preference=3;
        fps_log_frame_limit(3,"legacy_unlimited_migrated",fps_store_frame_limit(3));
    }
    if(explicit_frame_limit)source="launch_option";
    else if(arguments_available && preference>=0){fps_pending_frame_interval=preference;source="preference_pending";}
    else if(value[0])mod_log("fps_mod graphics_menu ignored reason=invalid_saved_frame_interval");
    fps_log_frame_limit(fps_pending_frame_interval>=0?fps_pending_frame_interval:*fps_frame_interval,source,0);
    mod_log("fps_mod graphics_menu ready page=Graphics_2 label=Wait_for_GPU_each_frame default_preserved=1");
    mod_log("fps_mod graphics_menu ready page=Graphics_1 frame_targets=144,165,240,360 timing=native_approximate");
}
