// Optional presentation pacing for the exact supported executable. The game's
// GetTickCount values remain real; only the two native limiter waits are replaced.
#pragma once
static int fps_precise_setting,fps_pacing_enabled,fps_pacing_installed;
static int fps_pacing_launch_target,fps_pacing_last_target;
static HANDLE fps_pacing_timer;
static LARGE_INTEGER fps_pacing_frequency;
static long long fps_pacing_deadline;
static uintptr_t fps_pacing_route;
static DWORD fps_pacing_thread;
static _Thread_local int fps_pacing_owns_wait;
static DWORD (WINAPI *fps_pacing_original_clock)(void);
static void (WINAPI *fps_pacing_original_sleep)(DWORD);
static int fps_pacing_interval_target(int interval){
    switch(interval){case 50:return 20;case 33:return 30;case 16:return 60;case 8:return 120;case 7:return 144;case 6:return 165;case 4:return 240;case 3:return 360;}
    int target=interval>0?1000/interval:360;
    return target<5?5:(target>1000?1000:target);
}
static int fps_pacing_argument(int argc,wchar_t **argv){
    int result=0;
    for(int i=1;i<argc;i++)if(!wcsncmp(argv[i],L"--maxfps=",9)){
        unsigned value=0;const wchar_t *p=argv[i]+9;
        if(!*p){result=0;continue;}
        for(;*p>=L'0' && *p<=L'9';p++){if(value<10000)value=value*10+(unsigned)(*p-L'0');}
        result=*p?0:(value<5?5:(value>1000?1000:(int)value));
    }
    return result;
}
static int fps_pacing_check(unsigned char *base,void *clock,void *sleep){
    if(!base || !clock || !sleep)return 0;
    void *found=NULL;memcpy(&found,base+0x33a268,sizeof(found));if(found!=clock)return 0;
    memcpy(&found,base+0x33a278,sizeof(found));if(found!=sleep)return 0;
    const uintptr_t sites[]={0x23f23f,0x2406ec,0x23f27a,0x240727};
    for(size_t i=0;i<4;i++){
        const unsigned char *site=base+sites[i];int32_t relative=0;
        if(site[0]!=0xff || site[1]!=0x15)return 0;
        memcpy(&relative,site+2,4);
        if((uintptr_t)site+6+(intptr_t)relative!=(uintptr_t)base+(i<2?0x33a268:0x33a278))return 0;
    }
    return 1;
}
static long long fps_pacing_tick(void){LARGE_INTEGER value;QueryPerformanceCounter(&value);return value.QuadPart;}
static void fps_pacing_reset(void){fps_pacing_deadline=0;fps_pacing_last_target=0;fps_pacing_route=0;fps_pacing_thread=0;fps_pacing_owns_wait=0;}
static void fps_pacing_fail(const char *reason){
    fps_pacing_enabled=fps_precise_setting=0;fps_pacing_reset();
    char line[260];snprintf(line,sizeof(line),"fps_mod precise_frame_limit enabled=0 reason=%s native_wait_restored=1",reason);mod_log(line);
}
static int fps_pacing_wait(long long deadline){
    for(int retry=0;retry<4;retry++){
        long long remaining=deadline-fps_pacing_tick();if(remaining<=0)return 1;
        LARGE_INTEGER due;due.QuadPart=-(remaining*10000000+fps_pacing_frequency.QuadPart-1)/fps_pacing_frequency.QuadPart;
        if(!SetWaitableTimer(fps_pacing_timer,&due,0,NULL,NULL,FALSE))return 0;
        DWORD timeout=(DWORD)(remaining*1000/fps_pacing_frequency.QuadPart)+50;
        if(WaitForSingleObject(fps_pacing_timer,timeout)!=WAIT_OBJECT_0)return 0;
    }
    return 0;
}
static void fps_pacing_frame(uintptr_t route){
    int target=fps_pacing_launch_target?fps_pacing_launch_target:fps_pacing_interval_target(*fps_frame_interval);
    DWORD owner=0;HWND foreground=GetForegroundWindow();
    if(!foreground || !GetWindowThreadProcessId(foreground,&owner) || owner!=GetCurrentProcessId())if(target>30)target=30;
    DWORD thread=GetCurrentThreadId();
    long long now=fps_pacing_tick(),period=(fps_pacing_frequency.QuadPart+target/2)/target;
    if(fps_pacing_route!=route || fps_pacing_thread!=thread || fps_pacing_last_target!=target || now-fps_pacing_deadline>fps_pacing_frequency.QuadPart){
        if(fps_pacing_last_target!=target){char line[180];snprintf(line,sizeof(line),"fps_mod precise_frame_limit active_target=%d foreground=%d busy_wait=0",target,owner==GetCurrentProcessId());mod_log(line);}
        fps_pacing_deadline=0;fps_pacing_route=route;fps_pacing_thread=thread;fps_pacing_last_target=target;
    }
    fps_pacing_owns_wait=0;
    if(fps_pacing_deadline && !fps_pacing_wait(fps_pacing_deadline)){fps_pacing_fail("timer_wait_failed");return;}
    now=fps_pacing_tick();
    fps_pacing_deadline=fps_pacing_deadline?fps_pacing_deadline+period:now+period;
    if(fps_pacing_deadline<=now)fps_pacing_deadline=now+period;
    fps_pacing_owns_wait=1;
}
static DWORD WINAPI fps_pacing_clock(void){
    uintptr_t route=(uintptr_t)__builtin_return_address(0)-(uintptr_t)detail_base;
    if(fps_pacing_enabled && (route==0x2406f2 || route==0x23f245))fps_pacing_frame(route);
    return fps_pacing_original_clock();
}
static void WINAPI fps_pacing_sleep(DWORD milliseconds){
    uintptr_t route=(uintptr_t)__builtin_return_address(0)-(uintptr_t)detail_base;
    if(fps_pacing_enabled && fps_pacing_owns_wait && (route==0x24072d || route==0x23f280))return;
    fps_pacing_original_sleep(milliseconds);
}
static int fps_pacing_enable(int enabled){
    if(!enabled){fps_pacing_enabled=0;fps_pacing_reset();return 1;}
    if(!fps_pacing_installed){
        HMODULE kernel=GetModuleHandleW(L"kernel32.dll");
        fps_pacing_original_clock=(DWORD (WINAPI *)(void))GetProcAddress(kernel,"GetTickCount");
        fps_pacing_original_sleep=(void (WINAPI *)(DWORD))GetProcAddress(kernel,"Sleep");
        if(!fps_pacing_check(detail_base,(void *)fps_pacing_original_clock,(void *)fps_pacing_original_sleep)){fps_pacing_fail("timing_hook_conflict");return 0;}
        if(!QueryPerformanceFrequency(&fps_pacing_frequency)){fps_pacing_fail("clock_unavailable");return 0;}
        if(!fps_pacing_timer)fps_pacing_timer=CreateWaitableTimerExW(NULL,NULL,0x00000002,TIMER_MODIFY_STATE|SYNCHRONIZE);
        if(!fps_pacing_timer){fps_pacing_fail("high_resolution_timer_unavailable");return 0;}
        void **clock=(void **)(detail_base+0x33a268),**sleep=(void **)(detail_base+0x33a278);DWORD old,unused;
        if(!VirtualProtect(clock,24,PAGE_READWRITE,&old)){fps_pacing_fail("timing_import_page_unavailable");return 0;}
        InterlockedExchangePointer((PVOID volatile *)clock,(PVOID)fps_pacing_clock);
        InterlockedExchangePointer((PVOID volatile *)sleep,(PVOID)fps_pacing_sleep);
        if(!VirtualProtect(clock,24,old,&unused)){
            InterlockedExchangePointer((PVOID volatile *)clock,(PVOID)fps_pacing_original_clock);
            InterlockedExchangePointer((PVOID volatile *)sleep,(PVOID)fps_pacing_original_sleep);
            if(!VirtualProtect(clock,24,old,&unused))mod_log("fps_mod precise_frame_limit warning=timing_import_protection_restore_failed");
            fps_pacing_fail("timing_import_transaction_failed");return 0;
        }
        fps_pacing_installed=1;
    }else{
        void *clock=NULL,*sleep=NULL;memcpy(&clock,detail_base+0x33a268,sizeof(clock));memcpy(&sleep,detail_base+0x33a278,sizeof(sleep));
        if(clock!=(void *)fps_pacing_clock || sleep!=(void *)fps_pacing_sleep){fps_pacing_fail("timing_hook_changed");return 0;}
    }
    fps_pacing_reset();fps_pacing_enabled=1;return 1;
}
