// Pinned laboratory experiment: exact copies of the game's existing RGBA8 mips.
// No resampling, mip generation, original texture writes, FBOs or buffer objects.
// Call only outside a native glBegin/End. Other GL users must invalidate state.
#pragma once
#define ATLAS_MAX_PAGES 8
#define ATLAS_CACHE_SIZE 4096
typedef struct {
    unsigned source_texture,texture;
    int x,y,width,height,side;
} FpsAtlasRect;
typedef struct {
    unsigned texture,min_filter,mag_filter;
    int source_size,max_level,side,next_cell,columns;
} FpsAtlasPage;
typedef struct { unsigned source;int decision;FpsAtlasRect rect; } FpsAtlasEntry;
static int atlas_enabled;
static _Thread_local HGLRC atlas_context;
static _Thread_local int atlas_state_checked,atlas_state_ok,atlas_has_pbo,atlas_has_fbo,atlas_has_aniso,atlas_has_swizzle;
static _Thread_local int atlas_pages_count;
static _Thread_local FpsAtlasPage atlas_pages[ATLAS_MAX_PAGES];
static _Thread_local FpsAtlasEntry atlas_entries[ATLAS_CACHE_SIZE];
static _Thread_local long long atlas_hits,atlas_uploads,atlas_rejected,atlas_bytes;
static void (*atlas_get_int)(unsigned,int *);
static void (*atlas_get_float)(unsigned,float *);
static const unsigned char *(*atlas_get_string)(unsigned);
static void (*atlas_get_parameter)(unsigned,unsigned,int *);
static void (*atlas_get_parameter_float)(unsigned,unsigned,float *);
static void (*atlas_get_level)(unsigned,int,unsigned,int *);
static void (*atlas_get_image)(unsigned,int,unsigned,unsigned,void *);
static void (*atlas_bind)(unsigned,unsigned);
static void (*atlas_generate)(int,unsigned *);
static void (*atlas_delete)(int,const unsigned *);
static void (*atlas_parameter)(unsigned,unsigned,int);
static void (*atlas_image)(unsigned,int,int,int,int,int,unsigned,unsigned,const void *);
static void (*atlas_subimage)(unsigned,int,int,int,int,int,unsigned,unsigned,const void *);
static void (*atlas_push_client)(unsigned);
static void (*atlas_pop_client)(void);
static void (*atlas_pixel_store)(unsigned,int);
static HGLRC (WINAPI *atlas_current_context)(void);

static int atlas_extension(const char *list,const char *name) {
    size_t n=strlen(name);const char *p=list;
    while(p && (p=strstr(p,name))) {if((p==list || p[-1]==' ') && (!p[n] || p[n]==' '))return 1;p+=n;}
    return 0;
}
static void atlas_invalidate_state(void) { atlas_state_checked=0; }
static void atlas_begin_frame(void) { atlas_invalidate_state(); }
static void atlas_forget(unsigned source) {
    unsigned index=(source*2654435761u)&(ATLAS_CACHE_SIZE-1);
    for(unsigned i=0;i<ATLAS_CACHE_SIZE;i++) {
        FpsAtlasEntry *e=&atlas_entries[(index+i)&(ATLAS_CACHE_SIZE-1)];
        if(e->source==source) {e->decision=0;return;}
        if(!e->source)return;
    }
}
static int atlas_check_state(void) {
    if(!atlas_enabled || !atlas_current_context)return 0;
    HGLRC context=atlas_current_context();
    if(!context)return 0;
    // Atlas objects belong exclusively to the first observed context. Refuse
    // other contexts instead of assuming shared objects or deleting foreign IDs.
    if(atlas_context && context!=atlas_context)return 0;
    if(!atlas_context) {
        atlas_context=context;
        const char *v=(const char *)atlas_get_string(0x1f02),*e=(const char *)atlas_get_string(0x1f03);
        int major=0,minor=0;if(!v || sscanf(v,"%d.%d",&major,&minor)!=2 || major<2)return 0;
        atlas_has_pbo=major>2 || (major==2 && minor>=1) || atlas_extension(e,"GL_ARB_pixel_buffer_object");
        atlas_has_fbo=major>=3 || atlas_extension(e,"GL_ARB_framebuffer_object") || atlas_extension(e,"GL_EXT_framebuffer_object");
        atlas_has_aniso=atlas_extension(e,"GL_EXT_texture_filter_anisotropic") || atlas_extension(e,"GL_ARB_texture_filter_anisotropic");
        atlas_has_swizzle=major>3 || (major==3 && minor>=3) || atlas_extension(e,"GL_ARB_texture_swizzle");
    }
    if(atlas_state_checked)return atlas_state_ok;
    atlas_state_checked=1;atlas_state_ok=0;
    int p=0;if(atlas_has_pbo) {atlas_get_int(0x88ed,&p);if(p)return 0;atlas_get_int(0x88ef,&p);if(p)return 0;}
    if(atlas_has_fbo) {atlas_get_int(0x8ca6,&p);if(p)return 0;}
    atlas_get_int(0x84e0,&p);if(p!=0x84c0)return 0; // server texture unit zero
    atlas_get_int(0x0d10,&p);if(p)return 0; // color maps during upload
    const unsigned scales[]={0x0d14,0x0d18,0x0d1a,0x0d1c};
    const unsigned biases[]={0x0d15,0x0d19,0x0d1b,0x0d1d};
    for(int i=0;i<4;i++) {float f;atlas_get_float(scales[i],&f);if(f!=1.0f)return 0;atlas_get_float(biases[i],&f);if(f!=0.0f)return 0;}
    atlas_state_ok=1;return 1;
}
static int atlas_level_rgba8(int level,int wanted) {
    int w=0,h=0,border=0,fmt=0,compressed=0;
    atlas_get_level(0x0de1,level,0x1000,&w);atlas_get_level(0x0de1,level,0x1001,&h);
    atlas_get_level(0x0de1,level,0x1005,&border);atlas_get_level(0x0de1,level,0x1003,&fmt);
    atlas_get_level(0x0de1,level,0x86a1,&compressed);
    if(w!=wanted || h!=wanted || border || compressed || (fmt!=0x8058 && fmt!=0x1908 && fmt!=4))return 0;
    for(unsigned attr=0x805c;attr<=0x805f;attr++) {int bits=0;atlas_get_level(0x0de1,level,attr,&bits);if(bits!=8)return 0;}
    return 1;
}
static int atlas_source_parameters(int *size,int *last,unsigned *min,unsigned *mag) {
    int w=0,s=0,t=0,base=0,max=0,mi=0,ma=0,compare=0;
    atlas_get_level(0x0de1,0,0x1000,&w);if(w!=64 && w!=128)return 0;
    *size=w;*last=w==64?6:7;
    atlas_get_parameter(0x0de1,0x2801,&mi);atlas_get_parameter(0x0de1,0x2800,&ma);
    if((mi!=0x2600 && mi!=0x2601 && (mi<0x2700 || mi>0x2703)) || (ma!=0x2600 && ma!=0x2601))return 0;
    atlas_get_parameter(0x0de1,0x2802,&s);atlas_get_parameter(0x0de1,0x2803,&t);
    // First slice supports the observed game case. REPEAT and legacy CLAMP fall
    // back; neither may use replicated-edge padding as an approximation.
    if(s!=0x812f || t!=0x812f)return 0;
    atlas_get_parameter(0x0de1,0x813c,&base);atlas_get_parameter(0x0de1,0x813d,&max);
    atlas_get_parameter(0x0de1,0x884c,&compare);if(base || max<*last || compare)return 0;
    float f=0;atlas_get_parameter_float(0x0de1,0x813a,&f);if(f!=-1000.0f)return 0;
    atlas_get_parameter_float(0x0de1,0x813b,&f);if(f!=1000.0f)return 0;
    atlas_get_parameter_float(0x0de1,0x8501,&f);if(f!=0.0f)return 0;
    if(atlas_has_aniso) {atlas_get_parameter_float(0x0de1,0x84fe,&f);if(!(f<=1.0f))return 0;}
    if(atlas_has_swizzle) {for(int i=0;i<4;i++) {int value=0;atlas_get_parameter(0x0de1,0x8e42u+(unsigned)i,&value);if(value!=0x1903+i)return 0;}}
    for(int level=0;level<=*last;level++)if(!atlas_level_rgba8(level,w>>level))return 0;
    *min=(unsigned)mi;*mag=(unsigned)ma;return 1;
}
static FpsAtlasPage *atlas_page(int size,int last,unsigned min,unsigned mag) {
    for(int i=0;i<atlas_pages_count;i++) {
        FpsAtlasPage *p=&atlas_pages[i];
        if(p->source_size==size && p->max_level==last && p->min_filter==min && p->mag_filter==mag && p->next_cell<p->columns*p->columns)return p;
    }
    if(atlas_pages_count==ATLAS_MAX_PAGES)return NULL;
    int maximum=0;atlas_get_int(0x0d33,&maximum);int side=4096;
    while(side>maximum)side/=2;
    if(side<size*4)return NULL;
    size_t bytes=0;for(int level=0;level<=last;level++)bytes+=(size_t)(side>>level)*(side>>level)*4;
    if(bytes>128u*1024u*1024u)return NULL;
    unsigned texture=0;atlas_generate(1,&texture);if(!texture)return NULL;
    atlas_bind(0x0de1,texture);
    atlas_parameter(0x0de1,0x2801,(int)min);atlas_parameter(0x0de1,0x2800,(int)mag);
    atlas_parameter(0x0de1,0x2802,0x812f);atlas_parameter(0x0de1,0x2803,0x812f);
    atlas_parameter(0x0de1,0x813c,0);atlas_parameter(0x0de1,0x813d,last);
    for(int level=0;level<=last;level++) {
        atlas_image(0x0de1,level,0x8058,side>>level,side>>level,0,0x1908,0x1401,NULL);
        int actual=0;atlas_get_level(0x0de1,level,0x1000,&actual);
        if(actual!=(side>>level)) {atlas_delete(1,&texture);return NULL;}
    }
    FpsAtlasPage *p=&atlas_pages[atlas_pages_count++];
    *p=(FpsAtlasPage){texture,min,mag,size,last,side,0,side/(size*4)};
    atlas_bytes+=(long long)bytes;return p;
}
static void atlas_pixel_packing(void) {
    atlas_pixel_store(0x0d05,1);atlas_pixel_store(0x0d02,0);atlas_pixel_store(0x0d03,0);atlas_pixel_store(0x0d04,0);
    atlas_pixel_store(0x0d00,0);atlas_pixel_store(0x0d01,0);
    atlas_pixel_store(0x0cf5,1);atlas_pixel_store(0x0cf2,0);atlas_pixel_store(0x0cf3,0);atlas_pixel_store(0x0cf4,0);
    atlas_pixel_store(0x0cf0,0);atlas_pixel_store(0x0cf1,0);
}
static int atlas_clone(unsigned source,FpsAtlasRect *rect) {
    static unsigned checked_sizes;
    int before=0,size=0,last=0;unsigned min=0,mag=0;int ok=0;
    atlas_get_int(0x8069,&before);atlas_bind(0x0de1,source);
    unsigned char *memory=NULL;FpsAtlasPage *page=NULL;
    if(!atlas_source_parameters(&size,&last,&min,&mag))goto done;
    memory=HeapAlloc(GetProcessHeap(),0,(size_t)size*size*4*17);if(!memory)goto done;
    atlas_push_client(0x00000001u); // GL_CLIENT_PIXEL_STORE_BIT
    atlas_pixel_packing();page=atlas_page(size,last,min,mag);
    if(!page) {atlas_pop_client();goto done;}
    int cell=page->next_cell,cell_x=(cell%page->columns)*size*4,cell_y=(cell/page->columns)*size*4;
    unsigned size_bit=size==64?1u:2u;
    int check_copy=(checked_sizes&size_bit)==0;
    unsigned long long copy_mismatches=0;
    for(int level=0;level<=last;level++) {
        int w=size>>level,tile_w=w*4;unsigned char *raw=memory,*tile=raw+(size_t)size*size*4;
        atlas_bind(0x0de1,source);atlas_get_image(0x0de1,level,0x1908,0x1401,raw);
        for(int y=0;y<tile_w;y++) {
            int sy=y-w;if(sy<0)sy=0;if(sy>=w)sy=w-1;
            for(int x=0;x<tile_w;x++) {int sx=x-w;if(sx<0)sx=0;if(sx>=w)sx=w-1;memcpy(tile+((size_t)y*tile_w+x)*4,raw+((size_t)sy*w+sx)*4,4);}
        }
        atlas_bind(0x0de1,page->texture);
        atlas_subimage(0x0de1,level,cell_x>>level,cell_y>>level,tile_w,tile_w,0x1908,0x1401,tile);
        if(check_copy) {
            int side=page->side>>level;
            unsigned char *readback=HeapAlloc(GetProcessHeap(),0,(size_t)side*side*4);
            if(readback) {
                atlas_get_image(0x0de1,level,0x1908,0x1401,readback);
                for(int y=0;y<w;y++) for(int x=0;x<w;x++) for(int c=0;c<4;c++) {
                    size_t position=((size_t)((cell_y>>level)+w+y)*side+(cell_x>>level)+w+x)*4+c;
                    if(readback[position]!=raw[((size_t)y*w+x)*4+c]) copy_mismatches++;
                }
                HeapFree(GetProcessHeap(),0,readback);
            } else copy_mismatches++;
        }
    }
    if(check_copy) {
        checked_sizes|=size_bit;char line[256];snprintf(line,sizeof(line),"fps_atlas_copy_oracle source=%u size=%d levels=%d different_bytes=%llu",source,size,last+1,copy_mismatches);
        plugin_api->logLine((const void *)atlas_clone,line);
    }
    page->next_cell++;atlas_pop_client();
    *rect=(FpsAtlasRect){source,page->texture,cell_x+size,cell_y+size,size,size,page->side};ok=1;atlas_uploads++;
done:
    if(memory)HeapFree(GetProcessHeap(),0,memory);
    atlas_bind(0x0de1,(unsigned)before);return ok;
}
static int atlas_prepare(unsigned source,FpsAtlasRect *out) {
    if(!source || !out || !atlas_check_state())return 0;
    for(int i=0;i<atlas_pages_count;i++)if(atlas_pages[i].texture==source)return 0;
    unsigned index=(source*2654435761u)&(ATLAS_CACHE_SIZE-1);
    for(unsigned i=0;i<ATLAS_CACHE_SIZE;i++) {
        FpsAtlasEntry *e=&atlas_entries[(index+i)&(ATLAS_CACHE_SIZE-1)];
        if(e->source && e->source!=source)continue;
        if(!e->source)e->source=source;
        if(!e->decision) {e->decision=atlas_clone(source,&e->rect)?1:-1;if(e->decision<0)atlas_rejected++;}
        if(e->decision<0)return 0;
        *out=e->rect;atlas_hits++;return 1;
    }
    return 0;
}
static int atlas_remap_uv(const FpsAtlasRect *rect,const float in[4],float out[4]) {
    memcpy(out,in,sizeof(float)*4);
    if(!rect || !(in[0]>=0 && in[0]<=1 && in[1]>=0 && in[1]<=1) || in[2]!=0 || in[3]!=1)return 0;
    out[0]=((float)rect->x+in[0]*rect->width)/rect->side;
    out[1]=((float)rect->y+in[1]*rect->height)/rect->side;return 1;
}
static int install_gl_atlas(void) {
    char env[16];atlas_enabled=GetEnvironmentVariableA("FPS_GL_ATLAS",env,sizeof(env))>0;
#ifdef FPS_SE_MOD
    atlas_enabled=0;
#endif
    if(!atlas_enabled)return 1;
    HMODULE gl=GetModuleHandleA("opengl32.dll");if(!gl)return 0;
#define ATLAS_PROC(var,name) do {*(void **)&var=(void *)GetProcAddress(gl,name);if(!var)return 0;}while(0)
    ATLAS_PROC(atlas_get_int,"glGetIntegerv");ATLAS_PROC(atlas_get_float,"glGetFloatv");ATLAS_PROC(atlas_get_string,"glGetString");
    ATLAS_PROC(atlas_get_parameter,"glGetTexParameteriv");ATLAS_PROC(atlas_get_parameter_float,"glGetTexParameterfv");ATLAS_PROC(atlas_get_level,"glGetTexLevelParameteriv");
    ATLAS_PROC(atlas_get_image,"glGetTexImage");ATLAS_PROC(atlas_bind,"glBindTexture");ATLAS_PROC(atlas_generate,"glGenTextures");ATLAS_PROC(atlas_delete,"glDeleteTextures");
    ATLAS_PROC(atlas_parameter,"glTexParameteri");ATLAS_PROC(atlas_image,"glTexImage2D");ATLAS_PROC(atlas_subimage,"glTexSubImage2D");
    ATLAS_PROC(atlas_push_client,"glPushClientAttrib");ATLAS_PROC(atlas_pop_client,"glPopClientAttrib");ATLAS_PROC(atlas_pixel_store,"glPixelStorei");
    ATLAS_PROC(atlas_current_context,"wglGetCurrentContext");
#undef ATLAS_PROC
    return 1;
}
