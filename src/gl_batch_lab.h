// Lab-only immediate-mode coalescing. Draw order and submitted vertices remain
// unchanged. A pending primitive is ended before any relevant state mutation.
static _Thread_local int decor_scope, pending_quads;
static int batch_enabled, pixel_oracle;
static int color_cache_enabled;
static _Thread_local int color_valid;
static _Thread_local float last_color[4];
static _Thread_local long long color_skipped;
static long long skipped_sum;
static void (*real_gl_color3)(float,float,float);
static void (*real_gl_begin)(unsigned);
static void (*real_gl_end)(void);
static void (*real_gl_bind)(unsigned,unsigned);
static void (*real_gl_enable)(unsigned);
static void (*real_gl_disable)(unsigned);
static void (*real_gl_blend)(unsigned,unsigned);
static void (*real_gl_alpha)(unsigned,float);
static void (*real_gl_depth_mask)(unsigned char);
static void (*real_gl_tex_env)(unsigned,unsigned,int);
static _Thread_local long long end_ticks,bind_ticks,color_ticks;
static long long end_sum,bind_sum,color_sum;
static void (*real_gl_color)(float,float,float,float);
static void batch_flush(void) { if(pending_quads) { real_gl_end();pending_quads=0; } }
static void begin_probe(unsigned mode) {
    if(batch_enabled && decor_scope && mode==7) {
        if(!pending_quads) { real_gl_begin(mode);pending_quads=1; }
    } else { batch_flush();real_gl_begin(mode); }
}
static void end_probe(void) {
    long long t=now_ticks();
    if(!(batch_enabled && decor_scope && pending_quads)) {
        if(pending_quads) batch_flush();else real_gl_end();
    }
    if(in_highest) end_ticks+=now_ticks()-t;
}
static void bind_probe(unsigned target,unsigned texture) {
    long long t=now_ticks();batch_flush();real_gl_bind(target,texture);
    if(in_highest) bind_ticks+=now_ticks()-t;
}
static void color_probe(float r,float g,float b,float a) {
    long long t=now_ticks();float value[4]={r,g,b,a};
    if(color_cache_enabled && in_highest && color_valid && !memcmp(last_color,value,sizeof(value))) color_skipped++;
    else { real_gl_color(r,g,b,a);memcpy(last_color,value,sizeof(value));color_valid=in_highest; }
    if(in_highest) color_ticks+=now_ticks()-t;
}
static void color3_probe(float r,float g,float b) { color_valid=0;real_gl_color3(r,g,b); }
static void enable_probe(unsigned flag) { color_valid=0;batch_flush();real_gl_enable(flag); }
static void disable_probe(unsigned flag) { color_valid=0;batch_flush();real_gl_disable(flag); }
static void blend_probe(unsigned a,unsigned b) { batch_flush();real_gl_blend(a,b); }
static void alpha_probe(unsigned a,float b) { batch_flush();real_gl_alpha(a,b); }
static void depth_mask_probe(unsigned char a) { batch_flush();real_gl_depth_mask(a); }
static void tex_env_probe(unsigned a,unsigned b,int c) { batch_flush();real_gl_tex_env(a,b,c); }
static int replace_gl_import(uintptr_t rva,void *hook,void *saved) {
    void **slot=(void **)(detail_base+rva);
    memcpy(saved,slot,sizeof(void *));
    DWORD old,unused;
    if(!VirtualProtect(slot,sizeof(void *),PAGE_READWRITE,&old)) return 0;
    InterlockedExchangePointer(slot,hook);VirtualProtect(slot,sizeof(void *),old,&unused);return 1;
}
static int install_gl_batch(void) {
    char env[32];batch_enabled=GetEnvironmentVariableA("FPS_GL_BATCH",env,sizeof(env))>0;
    pixel_oracle=GetEnvironmentVariableA("FPS_PIXEL_ORACLE",env,sizeof(env))>0;
    color_cache_enabled=GetEnvironmentVariableA("FPS_COLOR_CACHE",env,sizeof(env))>0;
#ifdef FPS_SE_MOD
    batch_enabled=0;pixel_oracle=0;color_cache_enabled=0;
#endif
    return replace_gl_import(0x33a3e0,(void *)begin_probe,&real_gl_begin) &&
       replace_gl_import(0x33a410,(void *)end_probe,&real_gl_end) &&
       replace_gl_import(0x33a4b0,(void *)bind_probe,&real_gl_bind) &&
       replace_gl_import(0x33a460,(void *)color_probe,&real_gl_color) &&
       replace_gl_import(0x33a388,(void *)color3_probe,&real_gl_color3) &&
       replace_gl_import(0x33a4a8,(void *)enable_probe,&real_gl_enable) &&
       replace_gl_import(0x33a458,(void *)disable_probe,&real_gl_disable) &&
       replace_gl_import(0x33a498,(void *)blend_probe,&real_gl_blend) &&
       replace_gl_import(0x33a488,(void *)alpha_probe,&real_gl_alpha) &&
       replace_gl_import(0x33a4a0,(void *)depth_mask_probe,&real_gl_depth_mask) &&
       replace_gl_import(0x33a468,(void *)tex_env_probe,&real_gl_tex_env);
}
