// Opt-in pinned laboratory experiment. Original UV.xy and original RGBA8 mip
// bytes are retained; UV.z supplies a layer, identically for all quad vertices.
// Include after Windows/stdint/stdio/string declarations. No game/SE dependency.
#pragma once
#define TA_CACHE_SIZE 4096
#define TA_MAX_PAGES 8
#define TA_BUDGET (128u*1024u*1024u)
typedef struct {
    unsigned source_texture,array_texture;
    int layer,width,height,mip_count;
} FpsTextureLayer;
typedef struct {
    unsigned texture,min_filter,mag_filter;
    int size,last_level,layers,used;
} FpsTextureArrayPage;
typedef struct { unsigned source;int decision;FpsTextureLayer value; } FpsTextureArrayEntry;
static int texture_array_enabled;
static _Thread_local HGLRC ta_context;
static _Thread_local int ta_context_ok,ta_context_attempted,ta_state_checked,ta_state_ok,ta_drawing;
static _Thread_local int ta_has_aniso,ta_has_swizzle,ta_has_sampler,ta_page_count;
static _Thread_local unsigned ta_program;
static _Thread_local int ta_saved_program,ta_saved_binding,ta_saved_unit;
static _Thread_local FpsTextureArrayPage ta_pages[TA_MAX_PAGES];
static _Thread_local FpsTextureArrayEntry ta_entries[TA_CACHE_SIZE];
static _Thread_local long long ta_hits,ta_uploads,ta_rejected,ta_bytes;
static _Thread_local char ta_status[1024];
static HMODULE ta_gl;
static HGLRC (WINAPI *ta_current_context)(void);
static PROC (WINAPI *ta_wgl_proc)(LPCSTR);
static void (*ta_get_int)(unsigned,int *);
static void (*ta_get_float)(unsigned,float *);
static const unsigned char *(*ta_get_string)(unsigned);
static unsigned char (*ta_is_enabled)(unsigned);
static void (*ta_get_parameter)(unsigned,unsigned,int *);
static void (*ta_get_parameter_float)(unsigned,unsigned,float *);
static void (*ta_get_level)(unsigned,int,unsigned,int *);
static void (*ta_get_image)(unsigned,int,unsigned,unsigned,void *);
static void (*ta_get_env)(unsigned,unsigned,int *);
static void (*ta_bind)(unsigned,unsigned);
static void (*ta_generate)(int,unsigned *);
static void (*ta_delete)(int,const unsigned *);
static void (*ta_parameter)(unsigned,unsigned,int);
static void (*ta_push_client)(unsigned);
static void (*ta_pop_client)(void);
static void (*ta_pixel_store)(unsigned,int);
static void (*ta_active_texture)(unsigned);
static void (*ta_image3d)(unsigned,int,int,int,int,int,int,unsigned,unsigned,const void *);
static void (*ta_subimage3d)(unsigned,int,int,int,int,int,int,int,unsigned,unsigned,const void *);
static unsigned (*ta_create_shader)(unsigned);
static void (*ta_shader_source)(unsigned,int,const char *const *,const int *);
static void (*ta_compile_shader)(unsigned);
static void (*ta_get_shader)(unsigned,unsigned,int *);
static void (*ta_shader_log)(unsigned,int,int *,char *);
static void (*ta_delete_shader)(unsigned);
static unsigned (*ta_create_program)(void);
static void (*ta_attach_shader)(unsigned,unsigned);
static void (*ta_link_program)(unsigned);
static void (*ta_get_program)(unsigned,unsigned,int *);
static void (*ta_program_log)(unsigned,int,int *,char *);
static void (*ta_delete_program)(unsigned);
static void (*ta_use_program)(unsigned);
static int (*ta_uniform_location)(unsigned,const char *);
static void (*ta_uniform1i)(int,int);

static void ta_message(const char *text) {snprintf(ta_status,sizeof(ta_status),"%s",text);}
static int ta_extension(const char *list,const char *word) {
    const char *p=list;size_t n=strlen(word);
    while(p && (p=strstr(p,word))) {if((p==list || p[-1]==' ') && (!p[n] || p[n]==' '))return 1;p+=n;}
    return 0;
}
static void *ta_proc(const char *name) {
    void *p=(void *)GetProcAddress(ta_gl,name);
    if(!p && ta_wgl_proc)p=(void *)ta_wgl_proc(name);
    if((uintptr_t)p<=3 || (uintptr_t)p==UINTPTR_MAX)return NULL;
    return p;
}
static unsigned ta_shader(unsigned kind,const char *source) {
    unsigned shader=ta_create_shader(kind);if(!shader)return 0;
    ta_shader_source(shader,1,&source,NULL);ta_compile_shader(shader);
    int ok=0;ta_get_shader(shader,0x8b81,&ok);
    if(!ok) {ta_shader_log(shader,sizeof(ta_status),NULL,ta_status);ta_delete_shader(shader);return 0;}
    return shader;
}
static int ta_build_program(void) {
    static const char vertex[]=
        "#version 130\n"
        "out vec2 sampleUV;out vec4 sampleColor;flat out int sampleLayer;\n"
        "void main(){gl_Position=ftransform();sampleUV=gl_MultiTexCoord0.xy;"
        "sampleLayer=int(gl_MultiTexCoord0.z+0.5);sampleColor=clamp(gl_Color,0.0,1.0);}\n";
    static const char fragment[]=
        "#version 130\n"
        "uniform sampler2DArray sourceImages;in vec2 sampleUV;in vec4 sampleColor;flat in int sampleLayer;\n"
        "void main(){gl_FragColor=texture(sourceImages,vec3(sampleUV,float(sampleLayer)))*sampleColor;}\n";
    unsigned vs=ta_shader(0x8b31,vertex);if(!vs)return 0;
    unsigned fs=ta_shader(0x8b30,fragment);if(!fs){ta_delete_shader(vs);return 0;}
    unsigned program=ta_create_program();
    if(!program) {ta_delete_shader(vs);ta_delete_shader(fs);return 0;}
    ta_attach_shader(program,vs);ta_attach_shader(program,fs);ta_link_program(program);
    ta_delete_shader(vs);ta_delete_shader(fs);
    int ok=0;ta_get_program(program,0x8b82,&ok);
    if(!ok) {ta_program_log(program,sizeof(ta_status),NULL,ta_status);ta_delete_program(program);return 0;}
    int location=ta_uniform_location(program,"sourceImages");
    if(location<0) {ta_message("sampler uniform unavailable");ta_delete_program(program);return 0;}
    int before=0;ta_get_int(0x8b8d,&before);ta_use_program(program);ta_uniform1i(location,0);ta_use_program((unsigned)before);
    ta_program=program;ta_message("ready GLSL130 compatibility original UV and mip layers");return 1;
}
static int ta_initialize_context(void) {
    HGLRC context=ta_current_context();if(!context)return 0;
    if(ta_context_attempted)return context==ta_context && ta_context_ok;
    ta_context_attempted=1;ta_context=context;
    const char *v=(const char *)ta_get_string(0x1f02),*ext=(const char *)ta_get_string(0x1f03);
    int major=0,minor=0;
    if(!v || sscanf(v,"%d.%d",&major,&minor)!=2 || major<3) {ta_message("OpenGL 3 compatibility context required");return 0;}
    if(major>3 || (major==3 && minor>=2)) {int profile=0;ta_get_int(0x9126,&profile);if(!(profile&2)) {ta_message("compatibility profile required");return 0;}}
    ta_has_aniso=ta_extension(ext,"GL_EXT_texture_filter_anisotropic") || ta_extension(ext,"GL_ARB_texture_filter_anisotropic");
    ta_has_swizzle=major>3 || (major==3 && minor>=3) || ta_extension(ext,"GL_ARB_texture_swizzle");
    ta_has_sampler=major>3 || (major==3 && minor>=3) || ta_extension(ext,"GL_ARB_sampler_objects");
#define TA_EXT(var,name) do {*(void **)&var=ta_proc(name);if(!var){ta_message("missing " name);return 0;}}while(0)
    TA_EXT(ta_active_texture,"glActiveTexture");TA_EXT(ta_image3d,"glTexImage3D");TA_EXT(ta_subimage3d,"glTexSubImage3D");
    TA_EXT(ta_create_shader,"glCreateShader");TA_EXT(ta_shader_source,"glShaderSource");TA_EXT(ta_compile_shader,"glCompileShader");
    TA_EXT(ta_get_shader,"glGetShaderiv");TA_EXT(ta_shader_log,"glGetShaderInfoLog");TA_EXT(ta_delete_shader,"glDeleteShader");
    TA_EXT(ta_create_program,"glCreateProgram");TA_EXT(ta_attach_shader,"glAttachShader");TA_EXT(ta_link_program,"glLinkProgram");
    TA_EXT(ta_get_program,"glGetProgramiv");TA_EXT(ta_program_log,"glGetProgramInfoLog");TA_EXT(ta_delete_program,"glDeleteProgram");
    TA_EXT(ta_use_program,"glUseProgram");TA_EXT(ta_uniform_location,"glGetUniformLocation");TA_EXT(ta_uniform1i,"glUniform1i");
#undef TA_EXT
    if(!ta_build_program())return 0;
    ta_context_ok=1;return 1;
}
static void array_texture_invalidate_state(void) {ta_state_checked=0;}
static void array_texture_begin_frame(void) {array_texture_invalidate_state();}
static int ta_allowed_state(void) {
    if(!texture_array_enabled || !ta_current_context || !ta_initialize_context())return 0;
    if(ta_state_checked)return ta_state_ok;
    ta_state_checked=1;ta_state_ok=0;
    int value=0;ta_get_int(0x8b8d,&value);if(value)return 0;
    ta_get_int(0x84e0,&value);if(value!=0x84c0)return 0;
    ta_get_int(0x88ed,&value);if(value)return 0;ta_get_int(0x88ef,&value);if(value)return 0;
    ta_get_int(0x8ca6,&value);if(value)return 0;ta_get_int(0x8caa,&value);if(value)return 0;
    ta_get_int(0x0d10,&value);if(value)return 0;
    if(ta_has_sampler) {ta_get_int(0x8919,&value);if(value)return 0;}
    if(!ta_is_enabled(0x0de1) || ta_is_enabled(0x0b50) || ta_is_enabled(0x0b57) || ta_is_enabled(0x0b60))return 0;
    if(ta_is_enabled(0x0de0) || ta_is_enabled(0x806f) || ta_is_enabled(0x8513) || ta_is_enabled(0x8458))return 0;
    for(unsigned cap=0x3000;cap<0x3006;cap++)if(ta_is_enabled(cap))return 0;
    for(unsigned cap=0x0c60;cap<=0x0c63;cap++)if(ta_is_enabled(cap))return 0;
    ta_get_env(0x2300,0x2200,&value);if(value!=0x2100)return 0; // GL_MODULATE only
    ta_get_int(0x0c40,&value);if(value!=0x1c00)return 0;
    ta_get_int(0x0b33,&value);if(value)return 0;
    float matrix[16];ta_get_float(0x0ba8,matrix);
    for(int i=0;i<16;i++)if(matrix[i]!=((i%5)==0?1.0f:0.0f))return 0;
    const unsigned scales[]={0x0d14,0x0d18,0x0d1a,0x0d1c},biases[]={0x0d15,0x0d19,0x0d1b,0x0d1d};
    for(int i=0;i<4;i++){float f;ta_get_float(scales[i],&f);if(f!=1.0f)return 0;ta_get_float(biases[i],&f);if(f!=0.0f)return 0;}
    int units=0;ta_get_int(0x84e2,&units);if(units<1 || units>32)return 0;
    for(int unit=1;unit<units;unit++) {
        ta_active_texture(0x84c0u+(unsigned)unit);
        int enabled=ta_is_enabled(0x0de0) || ta_is_enabled(0x0de1) || ta_is_enabled(0x806f) || ta_is_enabled(0x8513);
        ta_active_texture(0x84c0);if(enabled)return 0;
    }
    ta_state_ok=1;return 1;
}
static int ta_source_parameters(int *size,int *last,unsigned *min,unsigned *mag) {
    int w=0,base=0,max=0,s=0,t=0,mi=0,ma=0,compare=0;
    ta_get_level(0x0de1,0,0x1000,&w);if(w!=64 && w!=128)return 0;
    *size=w;*last=w==64?6:7;
    ta_get_parameter(0x0de1,0x2801,&mi);ta_get_parameter(0x0de1,0x2800,&ma);
    if((mi!=0x2600 && mi!=0x2601 && (mi<0x2700 || mi>0x2703)) || (ma!=0x2600 && ma!=0x2601))return 0;
    ta_get_parameter(0x0de1,0x2802,&s);ta_get_parameter(0x0de1,0x2803,&t);if(s!=0x812f || t!=0x812f)return 0;
    ta_get_parameter(0x0de1,0x813c,&base);ta_get_parameter(0x0de1,0x813d,&max);
    ta_get_parameter(0x0de1,0x884c,&compare);if(base || max<*last || compare)return 0;
    float f=0;ta_get_parameter_float(0x0de1,0x813a,&f);if(f!=-1000.0f)return 0;
    ta_get_parameter_float(0x0de1,0x813b,&f);if(f!=1000.0f)return 0;
    ta_get_parameter_float(0x0de1,0x8501,&f);if(f!=0.0f)return 0;
    if(ta_has_aniso){ta_get_parameter_float(0x0de1,0x84fe,&f);if(!(f<=1.0f))return 0;}
    if(ta_has_swizzle)for(int i=0;i<4;i++){int a=0;ta_get_parameter(0x0de1,0x8e42u+(unsigned)i,&a);if(a!=0x1903+i)return 0;}
    for(int level=0;level<=*last;level++) {
        int width=0,height=0,border=0,format=0,compressed=0;
        ta_get_level(0x0de1,level,0x1000,&width);ta_get_level(0x0de1,level,0x1001,&height);
        ta_get_level(0x0de1,level,0x1005,&border);ta_get_level(0x0de1,level,0x1003,&format);ta_get_level(0x0de1,level,0x86a1,&compressed);
        if(width!=(w>>level) || height!=width || border || compressed || (format!=0x8058 && format!=0x1908 && format!=4))return 0;
        for(unsigned attr=0x805c;attr<=0x805f;attr++){int bits=0;ta_get_level(0x0de1,level,attr,&bits);if(bits!=8)return 0;}
    }
    *min=(unsigned)mi;*mag=(unsigned)ma;return 1;
}
static FpsTextureArrayPage *ta_find_page(int size,int last,unsigned min,unsigned mag) {
    for(int i=0;i<ta_page_count;i++){FpsTextureArrayPage *p=&ta_pages[i];if(p->size==size && p->last_level==last && p->min_filter==min && p->mag_filter==mag && p->used<p->layers)return p;}
    if(ta_page_count==TA_MAX_PAGES)return NULL;
    int layers=0;ta_get_int(0x88ff,&layers);if(layers>256)layers=256;
    size_t one=0;for(int level=0;level<=last;level++)one+=(size_t)(size>>level)*(size>>level)*4;
    long long remaining=(long long)TA_BUDGET-ta_bytes;if(remaining<=0)return NULL;
    if((long long)layers>(remaining/(long long)one))layers=(int)(remaining/(long long)one);
    if(layers<1)return NULL;
    unsigned texture=0;ta_generate(1,&texture);if(!texture)return NULL;
    ta_bind(0x8c1a,texture);ta_parameter(0x8c1a,0x2801,(int)min);ta_parameter(0x8c1a,0x2800,(int)mag);
    ta_parameter(0x8c1a,0x2802,0x812f);ta_parameter(0x8c1a,0x2803,0x812f);
    ta_parameter(0x8c1a,0x813c,0);ta_parameter(0x8c1a,0x813d,last);
    for(int level=0;level<=last;level++) {
        int w=size>>level;ta_image3d(0x8c1a,level,0x8058,w,w,layers,0,0x1908,0x1401,NULL);
        int actual=0;ta_get_level(0x8c1a,level,0x1000,&actual);if(actual!=w){ta_delete(1,&texture);return NULL;}
    }
    FpsTextureArrayPage *p=&ta_pages[ta_page_count++];*p=(FpsTextureArrayPage){texture,min,mag,size,last,layers,0};
    ta_bytes+=(long long)one*layers;return p;
}
static void ta_packing(void) {
    ta_pixel_store(0x0d05,1);ta_pixel_store(0x0d02,0);ta_pixel_store(0x0d03,0);ta_pixel_store(0x0d04,0);
    ta_pixel_store(0x0d00,0);ta_pixel_store(0x0d01,0);ta_pixel_store(0x806b,0);ta_pixel_store(0x806c,0);
    ta_pixel_store(0x0cf5,1);ta_pixel_store(0x0cf2,0);ta_pixel_store(0x0cf3,0);ta_pixel_store(0x0cf4,0);
    ta_pixel_store(0x0cf0,0);ta_pixel_store(0x0cf1,0);ta_pixel_store(0x806d,0);ta_pixel_store(0x806e,0);
}
static int ta_clone(unsigned source,FpsTextureLayer *out) {
    int before2d=0,before_array=0,size=0,last=0,ok=0;unsigned min=0,mag=0;
    ta_get_int(0x8069,&before2d);ta_get_int(0x8c1d,&before_array);ta_bind(0x0de1,source);
    unsigned char *memory=NULL;
    if(!ta_source_parameters(&size,&last,&min,&mag))goto done;
    memory=HeapAlloc(GetProcessHeap(),0,(size_t)size*size*4);if(!memory)goto done;
    ta_push_client(0x00000001u);ta_packing();FpsTextureArrayPage *p=ta_find_page(size,last,min,mag);
    if(!p){ta_pop_client();goto done;}
    for(int level=0;level<=last;level++) {
        int w=size>>level;ta_bind(0x0de1,source);ta_get_image(0x0de1,level,0x1908,0x1401,memory);
        ta_bind(0x8c1a,p->texture);ta_subimage3d(0x8c1a,level,0,0,p->used,w,w,1,0x1908,0x1401,memory);
    }
    ta_pop_client();*out=(FpsTextureLayer){source,p->texture,p->used++,size,size,last+1};ta_uploads++;ok=1;
done:
    if(memory)HeapFree(GetProcessHeap(),0,memory);
    ta_bind(0x8c1a,(unsigned)before_array);ta_bind(0x0de1,(unsigned)before2d);return ok;
}
static int array_texture_prepare(unsigned source,FpsTextureLayer *out) {
    if(!source || !out || !ta_allowed_state())return 0;
    for(int i=0;i<ta_page_count;i++)if(ta_pages[i].texture==source)return 0;
    unsigned index=(source*2654435761u)&(TA_CACHE_SIZE-1);
    for(unsigned i=0;i<TA_CACHE_SIZE;i++) {
        FpsTextureArrayEntry *e=&ta_entries[(index+i)&(TA_CACHE_SIZE-1)];if(e->source && e->source!=source)continue;
        if(!e->source)e->source=source;
        if(!e->decision){e->decision=ta_clone(source,&e->value)?1:-1;if(e->decision<0)ta_rejected++;}
        if(e->decision<0)return 0;
        *out=e->value;ta_hits++;return 1;
    }
    return 0;
}
static void array_texture_forget(unsigned source) {
    unsigned index=(source*2654435761u)&(TA_CACHE_SIZE-1);
    for(unsigned i=0;i<TA_CACHE_SIZE;i++){FpsTextureArrayEntry *e=&ta_entries[(index+i)&(TA_CACHE_SIZE-1)];if(e->source==source){e->decision=0;return;}if(!e->source)return;}
}
static int array_texture_begin_draw(unsigned texture) {
    if(ta_drawing || !ta_allowed_state())return 0;
    int own=0;for(int i=0;i<ta_page_count;i++)if(ta_pages[i].texture==texture)own=1;if(!own)return 0;
    ta_get_int(0x8b8d,&ta_saved_program);ta_get_int(0x84e0,&ta_saved_unit);
    if(ta_saved_program || ta_saved_unit!=0x84c0)return 0;
    ta_get_int(0x8c1d,&ta_saved_binding);ta_use_program(ta_program);ta_bind(0x8c1a,texture);ta_drawing=1;return 1;
}
static void array_texture_end_draw(void) {
    if(!ta_drawing)return;
    ta_bind(0x8c1a,(unsigned)ta_saved_binding);ta_use_program((unsigned)ta_saved_program);
    ta_active_texture((unsigned)ta_saved_unit);ta_drawing=0;
}
static int install_texture_array(void) {
    char env[16];texture_array_enabled=GetEnvironmentVariableA("FPS_TEXTURE_ARRAY",env,sizeof(env))>0;
#ifdef FPS_SE_MOD
    texture_array_enabled=1;
#endif
    if(!texture_array_enabled)return 1;
    ta_gl=GetModuleHandleA("opengl32.dll");if(!ta_gl)return 0;
#define TA_CORE(var,name) do {*(void **)&var=(void *)GetProcAddress(ta_gl,name);if(!var)return 0;}while(0)
    TA_CORE(ta_current_context,"wglGetCurrentContext");TA_CORE(ta_wgl_proc,"wglGetProcAddress");
    TA_CORE(ta_get_int,"glGetIntegerv");TA_CORE(ta_get_float,"glGetFloatv");TA_CORE(ta_get_string,"glGetString");TA_CORE(ta_is_enabled,"glIsEnabled");
    TA_CORE(ta_get_parameter,"glGetTexParameteriv");TA_CORE(ta_get_parameter_float,"glGetTexParameterfv");TA_CORE(ta_get_level,"glGetTexLevelParameteriv");
    TA_CORE(ta_get_image,"glGetTexImage");TA_CORE(ta_get_env,"glGetTexEnviv");TA_CORE(ta_bind,"glBindTexture");TA_CORE(ta_generate,"glGenTextures");TA_CORE(ta_delete,"glDeleteTextures");
    TA_CORE(ta_parameter,"glTexParameteri");TA_CORE(ta_push_client,"glPushClientAttrib");TA_CORE(ta_pop_client,"glPopClientAttrib");TA_CORE(ta_pixel_store,"glPixelStorei");
#undef TA_CORE
    ta_message("awaiting compatibility OpenGL context");return 1;
}
