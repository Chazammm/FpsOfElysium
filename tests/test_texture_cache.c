#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gl_texture_array_lab.h"
#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static FpsTextureArrayEntry *find(unsigned id) {
    unsigned start=(id*2654435761u)&(TA_CACHE_SIZE-1);
    for(unsigned i=0;i<TA_CACHE_SIZE;i++) {
        FpsTextureArrayEntry *e=&ta_entries[(start+i)&(TA_CACHE_SIZE-1)];
        if(e->source==id)return e;if(!e->source)return NULL;
    }
    return NULL;
}
static FpsTextureArrayEntry *insert(unsigned id) {
    unsigned start=(id*2654435761u)&(TA_CACHE_SIZE-1);
    for(unsigned i=0;i<TA_CACHE_SIZE;i++) {
        FpsTextureArrayEntry *e=&ta_entries[(start+i)&(TA_CACHE_SIZE-1)];
        if(!e->source){e->source=id;e->decision=-1;return e;}
    }
    return NULL;
}
int main(void) {
    ta_page_count=1;ta_pages[0]=(FpsTextureArrayPage){.texture=77,.layers=16};
    FpsTextureArrayEntry *entry=insert(17);CHECK(entry);
    for(int cycle=0;cycle<20000;cycle++) {
        int layer=ta_available_layer(&ta_pages[0]);CHECK(layer==0);
        ta_pages[0].occupied[layer]=1;ta_pages[0].used++;
        entry->decision=1;entry->value=(FpsTextureLayer){17,77,layer,64,64,7,0};
        array_texture_forget(17);CHECK(ta_pages[0].used==0 && entry->decision==0 && !entry->value.array_texture);
    }
    array_texture_remove(17);CHECK(!find(17));
    unsigned base=1;while(((base*2654435761u)&(TA_CACHE_SIZE-1))!=TA_CACHE_SIZE-1)base++;
    for(unsigned i=0;i<64;i++)CHECK(insert(base+i*TA_CACHE_SIZE));
    for(unsigned remove=0;remove<64;remove+=2)array_texture_remove(base+remove*TA_CACHE_SIZE);
    for(unsigned i=0;i<64;i++)CHECK((find(base+i*TA_CACHE_SIZE)!=NULL)==(i%2!=0));
    for(unsigned i=1;i<64;i+=2)array_texture_remove(base+i*TA_CACHE_SIZE);
    for(unsigned cycle=1;cycle<=20000;cycle++){CHECK(insert(cycle));CHECK(find(cycle));array_texture_remove(cycle);CHECK(!find(cycle));}
    for(unsigned id=1;id<=TA_CACHE_SIZE;id++)CHECK(insert(id));
    array_texture_remove(2049);CHECK(!find(2049));CHECK(insert(65537));
    for(unsigned id=1;id<=TA_CACHE_SIZE;id++)if(id!=2049)CHECK(find(id));
    CHECK(find(65537));
    puts("PASS: 20000 layer updates reuse storage; 20000 deleted-name cycles; wrapped collision clusters and full-table deletion remain searchable.");
    return 0;
}
