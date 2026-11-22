/* Run actual compiled x86 hooks against a private game-memory fixture.
 * A large private BSS section reserves fixture addresses in this test image.
 * It never attaches to the real game.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int jobs_esui_compat(void);unsigned jobs_accessory_owners(unsigned);
int cmk_install(void),cmk_allowed(uint32_t),cmk_swap_allowed(uint32_t *,uint32_t *),cmk_normal_allowed(void);
int cmk_can_equip(unsigned,uint32_t);unsigned cmk_owner_mask(uint32_t);
int jobs_configure(const int *);int jobs_selected(unsigned);int jobs_accessory_allowed(unsigned,unsigned);const char *jobs_class_name(unsigned);
#include "rules.generated.h"
int jobs_read_settings(const wchar_t *);unsigned jobs_class_count(void);int jobs_default(unsigned);
void jobs_test_defer_settings(void);
const char *jobs_catalogue_name(unsigned);
static int find_class(const char *name){for(int i=0;i<CLASS_COUNT;++i)if(!strcmp(CLASS_NAMES[i],name))return i;return -1;}
void cmk_row_swap(const uint8_t *,const uint8_t *,uint32_t *,uint32_t *);
#define CLOUD ((uint32_t *)0xDBFDCC)
#define INV ((uint32_t *)0xDC04B4)
#define EMPTY 0xFFFFFFFFu
#define U32(a) (*(uint32_t *)(a))
static unsigned count;
__attribute__((used)) volatile unsigned char fixture_reservation[0x1000000];
#define CHECK(c) do {++count;if(!(c)){printf("FAIL line %d: %s\n",__LINE__,#c);return 1;}}while(0)
static void write_bytes(uintptr_t at,const void *bytes,size_t n){memcpy((void *)at,bytes,n);}
static void reset_data(void) {
    for(unsigned c=0;c<9;++c) {
      uint8_t *record=(uint8_t *)(0xDBFD8C+c*0x84);record[0]=(uint8_t)c;record[1]=20;record[0x1E]=255;
      for(unsigned i=0;i<16;++i)((uint32_t *)(record+0x40))[i]=EMPTY;
    }
    for(unsigned i=0;i<200;++i)INV[i]=EMPTY;
    for(unsigned i=0;i<320;++i)((uint16_t *)0xDC0234)[i]=0xFFFF;
    for(unsigned i=0;i<32;++i)*(uint16_t *)(0xDBCAEC+i*16)=0x7FF;
    *(uint8_t *)0xDBFD8C=0;*(uint8_t *)0xDBFD8D=20;
}
static unsigned text_calls,sprite_calls,last_u,last_v,last_palette;
static unsigned drawn_icons[9][2];
static int drawn_x[9];
static const unsigned stock_weapon_uv[9][2]={{112,112},{96,128},{112,128},{96,144},
  {112,144},{96,160},{112,160},{96,176},{112,176}};
static unsigned drawn_owner_mask(void){
    unsigned owners=0;
    for(unsigned i=0;i<sprite_calls && i<9;++i)
      for(unsigned c=0;c<9;++c)if(drawn_icons[i][0]==stock_weapon_uv[c][0] && drawn_icons[i][1]==stock_weapon_uv[c][1])owners|=1u<<c;
    return owners;
}
static int last_text_x,last_text_y,last_icon_x,last_icon_y;
static float last_depth,hi_vertices[32],lo_vertices[32];
static uint32_t hi_object[32],lo_object[32];
static uint8_t font_metrics[256];static unsigned glyph_calls;static float glyph_vertices[32];
static uint32_t font_object[32];
static int glyph_stub(int x,int y,int color,int ch,float z,int low){
    (void)color;++glyph_calls;
    int bearing=(font_metrics[ch]>>5)*(low?1:2),offset=low?(int)U32(0xDC105C):0;
    for(unsigned i=0;i<4;++i){
      glyph_vertices[i*8]=(float)(x+offset+bearing+((i&2)?(low?12:24):0));
      glyph_vertices[i*8+1]=(float)(y+((i&1)?(low?12:24):0));
      glyph_vertices[i*8+2]=z;glyph_vertices[i*8+6]=0.25f;
    }
    return x+bearing+(font_metrics[ch]&31)*(low?1:2);
}
static int glyph_hi(int x,int y,int c,int ch,float z){return glyph_stub(x,y,c,ch,z,0);}
static int glyph_lo(int x,int y,int c,int ch,float z){return glyph_stub(x,y,c,ch,z,1);}
const uint8_t test_name[]={0x26,0x49,0x52,0x45,0xFF}; /* Fire */
static void text_stub(int x,int y,const uint8_t *text,int color,float z) {
    (void)text;(void)color;text_calls++;last_text_x=x;last_text_y=y;last_depth=z;
}
static void sprite_stub(int x,int y,int u,int v,int w,int h,int palette,int blend,int unused,float z,int high) {
    (void)blend;(void)unused;
    if(sprite_calls<9){drawn_icons[sprite_calls][0]=(unsigned)u;drawn_icons[sprite_calls][1]=(unsigned)v;drawn_x[sprite_calls]=x;}
    sprite_calls++;last_u=u;last_v=v;last_palette=palette;
    last_icon_x=x;last_icon_y=y;last_depth=z;
    float *verts=high?hi_vertices:lo_vertices;
    for(unsigned i=0;i<4;++i) {
        verts[i*8]=x+((i&2)?w*(high?2:1):0);
        verts[i*8+1]=y+((i&1)?h*(high?2:1):0);
        verts[i*8+2]=z;verts[i*8+6]=(float)u;verts[i*8+7]=(float)v;
    }
}
static void sprite_hi(int x,int y,int u,int v,int w,int h,int p,int b,int a,float z){sprite_stub(x,y,u,v,w,h,p,b,a,z,1);}
static void sprite_lo(int x,int y,int u,int v,int w,int h,int p,int b,int a,float z){sprite_stub(x,y,u,v,w,h,p,b,a,z,0);}
static void item_stub(int x,int y,int item,int unused,float z,int low){
    (void)unused;
    static const unsigned ids[9]={0x80,0xA0,0x90,0xBE,0xB0,0xD7,0xE5,0xF2,0xC9};
    for(unsigned c=0;c<9;++c)if((unsigned)item==ids[c])
        sprite_stub(x,y,stock_weapon_uv[c][0],stock_weapon_uv[c][1],16,16,1,0,0,z,!low);
}
static void item_hi(int x,int y,int item,int unused,float z){item_stub(x,y,item,unused,z,0);}
static void item_lo(int x,int y,int item,int unused,float z){item_stub(x,y,item,unused,z,1);}
__attribute__((used)) int test_row_limit=11;
static void write_jump(uintptr_t at,void *target) {
    *(uint8_t *)at=0xE9;int32_t rel=(int32_t)((uintptr_t)target-at-5);memcpy((void *)(at+1),&rel,4);
}
__attribute__((naked)) static int call_name(int context,int row,uintptr_t site,int x,int y,int padding) {
    __asm__ volatile(".intel_syntax noprefix\n"
        "push ebp\n mov ebp,esp\n sub esp,32\n sub esp,DWORD PTR [ebp+28]\n"
        "mov eax,[ebp+12]\n mov [ebp-8],eax\n"
        "mov eax,[_test_row_limit]\n mov [ebp-12],eax\n mov [ebp-20],eax\n"
        "push 0x3e4ccccd\n push 7\n lea eax,[_test_name]\n push eax\n"
        "push DWORD PTR [ebp+24]\n push DWORD PTR [ebp+20]\n"
        "push DWORD PTR [ebp+16]\n ret\n .att_syntax prefix\n");
}
/* Copy a private local frame, preserving the game's exact accessory offsets. */
__attribute__((naked)) static int call_accessory(const uint8_t *locals,uintptr_t site,int x,int y){
    __asm__ volatile(".intel_syntax noprefix\n"
      "push ebp\n mov ebp,esp\n sub esp,128\n push esi\n push edi\n"
      "mov esi,[ebp+8]\n lea edi,[ebp-128]\n mov ecx,32\n rep movsd\n pop edi\n pop esi\n"
      "push 0x3e4ccccd\n push 7\n lea eax,[_test_name]\n push eax\n"
      "push DWORD PTR [ebp+20]\n push DWORD PTR [ebp+16]\n push DWORD PTR [ebp+12]\n ret\n .att_syntax prefix\n");
}
/* Builds an EBP frame with exactly the two locals used by exchange_bridge. */
__attribute__((naked)) static int call_exchange(uint32_t *a,uint32_t *b) {
    __asm__ volatile(".intel_syntax noprefix\n"
      "push ebp\n mov ebp,esp\n sub esp,32\n"
      "mov eax,[ebp+8]\n mov [ebp-4],eax\n"
      "mov eax,[ebp+12]\n mov [ebp-16],eax\n"
      "push 0x710437\n ret\n .att_syntax prefix\n");
}
int main(int argc,char **argv) {
    /* Keep physical settings beside the private test executable regardless of
       the caller's working directory, matching production module-path lookup. */
    char esui_fixture[1024]={0},test_executable[1024];
    if(argc>2)CHECK(GetFullPathNameA(argv[2],sizeof(esui_fixture),esui_fixture,NULL)>0);
    CHECK(GetModuleFileNameA(NULL,test_executable,sizeof(test_executable))>0);
    char *test_separator=strrchr(test_executable,'\\');CHECK(test_separator!=NULL);*test_separator=0;
    CHECK(SetCurrentDirectoryA(test_executable));
    uintptr_t starts[]={0x5C0000,0x6C0000,0x6F0000,0x700000,0x710000,0x720000,0x740000,0x910000,0x990000,0xDB0000,0xDC0000,0xDD0000};
    for(unsigned i=0;i<sizeof(starts)/sizeof(starts[0]);++i) {
        MEMORY_BASIC_INFORMATION m;VirtualQuery((void *)starts[i],&m,sizeof(m));
        CHECK(m.AllocationBase==GetModuleHandleA(NULL));
        DWORD old;CHECK(VirtualProtect((void *)starts[i],0x10000,PAGE_EXECUTE_READWRITE,&old));
    }
    const uint8_t normal[]={0x68,0xBF,1,0,0};
    const uint8_t exchange[]={0x8B,0x55,0xFC,0x8B,2};
    const uint8_t row[]={0x55,0x8B,0xEC,0x83,0xEC,8};
    const uint8_t stats[]={0x55,0x8B,0xEC,0x83,0xEC,0x24};
    const uint8_t magic[]={0x55,0x8B,0xEC,0x83,0xEC,0x28};
    write_bytes(0x70DC14,normal,5);write_bytes(0x710437,exchange,5);
    const uint8_t equipment[]={0x55,0x8B,0xEC,0x83,0xEC,0x10};
    write_bytes(0x7083ED,equipment,6);
    const uint8_t equipment_cont[]={0xA1,0xEC,0xCA,0xDB,0,0xC9,0xC3};
    write_bytes(0x7083F3,equipment_cont,7);
    write_bytes(0x710BD0,row,6);write_bytes(0x5CB2CC,stats,6);write_bytes(0x6C545B,magic,6);
    uintptr_t name_sites[]={0x70C813,0x70C9DE,0x70F394,0x70FC78};
    uintptr_t name_targets[]={0x6F5B03,0x6FAB2F,0x6F5B03,0x6FAB2F};
    for(unsigned i=0;i<4;++i) {
        write_jump(name_sites[i],(void *)name_targets[i]);*(uint8_t *)name_sites[i]=0xE8;
        const uint8_t continuation[]={0x83,0xC4,20,0xB8,77,0,0,0,0xC9,0xC3};
        write_bytes(name_sites[i]+5,continuation,sizeof(continuation));
    }
    uintptr_t accessory_sites[]={0x705ED1,0x706857,0x7063C7,0x706D49,0x715846,0x716203};
    for(unsigned i=0;i<6;++i){
        write_jump(accessory_sites[i],(void *)((i&1)?0x6FAB2F:0x6F5B03));*(uint8_t *)accessory_sites[i]=0xE8;
        const uint8_t continuation[]={0x83,0xC4,20,0xB8,77,0,0,0,0xC9,0xC3};
        write_bytes(accessory_sites[i]+5,continuation,sizeof(continuation));
    }
    *(uint16_t *)0x705DB4=618;*(uint16_t *)0x706732=309;
    *(uint16_t *)0x715941=548;*(uint16_t *)0x7162FE=274;
    write_jump(0x6F5B03,text_stub);write_jump(0x6FAB2F,text_stub);
    write_jump(0x6F7BFB,sprite_hi);write_jump(0x6FCD45,sprite_lo);
    write_jump(0x717691,item_hi);write_jump(0x7177A8,item_lo);
    *(uint16_t *)0x70C84D=*(uint16_t *)0x70F3C5=618;
    *(uint16_t *)0x70CA15=*(uint16_t *)0x70FCA9=309;
    write_jump(0x6F564E,glyph_hi);write_jump(0x6FA706,glyph_lo);
    hi_object[0x70/4]=(uint32_t)hi_vertices;lo_object[0x70/4]=(uint32_t)lo_vertices;
    U32(0xDC0FC8)=U32(0xDC0FC8+8)=(uint32_t)hi_object;U32(0xDC104C)=(uint32_t)lo_object;
    /* Dummy original recalculations return the first Cloud Materia they see. */
    const uint8_t original[]={0xA1,0xCC,0xFD,0xDB,0,0xC9,0xC3};
    write_bytes(0x5CB2D2,original,7);write_bytes(0x6C5461,original,7);
    *(uint8_t *)0x74580A=0xC3; /* fixture stock sound returns immediately */
    /* Exchange continuation implements the remaining stock two-slot swap. */
    const uint8_t accept[]={0x89,0x45,0xF4,0x8B,0x4D,0xFC,0x8B,0x55,0xF0,
        0x8B,0x02,0x89,0x01,0x8B,0x4D,0xF0,0x8B,0x55,0xF4,0x89,0x11,
        0xB8,1,0,0,0,0xC9,0xC3};
    const uint8_t deny[]={0x31,0xC0,0xC9,0xC3};
    write_bytes(0x71043C,accept,sizeof(accept));write_bytes(0x71078E,deny,sizeof(deny));
    /* Ordinary equip continuation removes the pushed sound argument. */
    const uint8_t normal_ok[]={0x83,0xC4,4,0xB8,1,0,0,0,0xC3};
    const uint8_t normal_no[]={0x31,0xC0,0xC3};
    write_bytes(0x70DC19,normal_ok,sizeof(normal_ok));write_bytes(0x70DD64,normal_no,sizeof(normal_no));
    if(argc>1 && strcmp(argv[1],"esui")!=0) { /* A conflict must reject installation without partial writes. */
        *(uint8_t *)(strcmp(argv[1],"accessory-ui-conflict")==0?0x716203:strcmp(argv[1],"accessory-conflict")==0?0x7083ED:strcmp(argv[1],"ui-conflict")==0?0x70FC78:0x6C545B)=0x90;
        CHECK(cmk_install()==0);CHECK(*(uint8_t *)0x70DC14==0x68);
        CHECK(*(uint8_t *)0x710437==0x8B);CHECK(*(uint8_t *)0x5CB2CC==0x55);
        CHECK(*(uint8_t *)0x705ED1==0xE8);CHECK(*(uint8_t *)0x7063C7==0xE8);
        printf("PASS %u conflict checks\n",count);return 0;
    }
    CHECK(cmk_install()==1);CHECK(cmk_install()==1);reset_data();

    int options[9]=DEFAULT_JOB_VALUES;
    CHECK(jobs_class_count()==25);CHECK(jobs_catalogue_name(25)==NULL);
    CHECK(jobs_configure(options)==1);
    for(unsigned c=0;c<9;++c){CHECK(jobs_selected(c)==DEFAULT_JOBS[c]);CHECK(jobs_default(c)==DEFAULT_JOBS[c]);CHECK(strcmp(jobs_class_name(c),CLASS_NAMES[DEFAULT_JOBS[c]])==0);}
    CHECK(jobs_selected(9)==-1);CHECK(jobs_default(9)==-1);CHECK(jobs_class_name(9)==NULL);
    /* Every character selects every class. Mixed and duplicate choices are valid.
       Check all 256 IDs, AP independence, guests, empty slots and exact icons. */
    for(unsigned round=0;round<25;++round){
      for(unsigned c=0;c<9;++c)options[c]=(round+c*3)%25;
      CHECK(jobs_configure(options)==1);
      for(unsigned c=0;c<9;++c)CHECK(jobs_selected(c)==options[c]);
      for(unsigned id=0;id<256;++id){
        unsigned expected=0;for(unsigned c=0;c<9;++c)if(CLASS_MATERIA[options[c]][id])expected|=1u<<c;
        CHECK(cmk_owner_mask(id)==expected);CHECK(cmk_owner_mask(0xAABBCC00u|id)==expected);
        for(unsigned c=0;c<9;++c)CHECK(cmk_can_equip(c,0xAABBCC00u|id)==(id==255 || !!(expected&(1u<<c))));
        CHECK(cmk_can_equip(9,id));CHECK(cmk_can_equip(10,id));
      }
    }
    int saved[9];memcpy(saved,options,sizeof(saved));
    for(unsigned c=0;c<9;++c){
      options[c]=25;CHECK(!jobs_configure(options));for(unsigned k=0;k<9;++k)CHECK(jobs_selected(k)==saved[k]);
      options[c]=-1;CHECK(!jobs_configure(options));options[c]=saved[c];
    }
    CHECK(!jobs_configure(NULL));
    /* All classes are selectable via real marker files; malformed settings
       reject the complete update and preserve the previous configuration. */
    wchar_t base[1024];CHECK(GetCurrentDirectoryW(1024,base)>0);
    CHECK(jobs_read_settings(base));
    for(unsigned c=0;c<9;++c)CHECK(jobs_selected(c)==DEFAULT_JOBS[c]);
    CreateDirectoryA("ffvii-job-system-v070",NULL);
    for(unsigned j=0;j<25;++j){
      for(unsigned c=0;c<9;++c){
        char path[200];snprintf(path,sizeof(path),"ffvii-job-system-v070/%s.class",CHARACTER_KEYS[c]);
        FILE *f=fopen(path,"wb");CHECK(f!=NULL);fprintf(f,"%s\r\n",CLASS_KEYS[(j+c)%25]);CHECK(!fclose(f));
      }
      CHECK(jobs_read_settings(base));
      for(unsigned c=0;c<9;++c)CHECK(jobs_selected(c)==(int)((j+c)%25));
    }
    /* 7H 4.6.1 loads libraries before activating file hooks. The first actual
       stat rebuild must load selections, before evaluating equipped orbs. */
    CHECK(jobs_configure(DEFAULT_JOBS));jobs_test_defer_settings();reset_data();CLOUD[0]=0xABCDEF31;
    CHECK(((int (__cdecl *)(int))0x5CB2CC)(0)==-1);
    for(unsigned c=0;c<9;++c)CHECK(jobs_selected(c)==(int)((24+c)%25));
    CHECK(CLOUD[0]==EMPTY);CHECK(INV[0]==0xABCDEF31);
    for(unsigned bad=0;bad<4;++bad){
      FILE *f=fopen("ffvii-job-system-v070/cid.class","wb");CHECK(f!=NULL);
      if(bad==0)fprintf(f,"unknown_class\n");
      if(bad==1)for(unsigned n=0;n<100;++n)fputc('a',f);
      if(bad==2)fwrite("monk\0fake",1,9,f);
      CHECK(!fclose(f));CHECK(!jobs_read_settings(base));
      for(unsigned c=0;c<9;++c)CHECK(jobs_selected(c)==(int)((24+c)%25));
    }
    for(unsigned c=0;c<9;++c){char path[200];snprintf(path,sizeof(path),"ffvii-job-system-v070/%s.class",CHARACTER_KEYS[c]);CHECK(DeleteFileA(path));}
    CHECK(RemoveDirectoryA("ffvii-job-system-v070"));CHECK(jobs_read_settings(base));
    memcpy(options,DEFAULT_JOBS,sizeof(options));CHECK(jobs_configure(options));
    /* Defaults intentionally leave some valid Materia unowned. The same zero
       mask is used for both icons and all equip paths. */
    CHECK(cmk_owner_mask(65)==0);CHECK(cmk_owner_mask(61)==0); /* no Time Mage selected */
    CHECK(cmk_owner_mask(44)==(1u<<4)); /* Blue Mage Red XIII */
    CHECK(cmk_owner_mask(49)==((1u<<0)|(1u<<7))); /* Mystic Knight + Black Mage */
    CHECK(cmk_owner_mask(53)==(1u<<3));CHECK(!cmk_can_equip(2,53)); /* strict researched Monk */
    /* AP must not influence permissions. */
    CHECK(cmk_allowed(0x98765431)==1);CHECK(cmk_allowed(0x98765435)==0);
    U32(0xDD163C)=0;U32(0xDD1374)=7;U32(0xDD1364)=1;
    INV[8]=0x12345635;
    CHECK(((int (__cdecl *)(void))0x70DC14)()==0);
    INV[8]=0x12345631;CHECK(((int (__cdecl *)(void))0x70DC14)()==1);
    U32(0xDD163C)=1;INV[8]=0x1234560B;CHECK(((int (__cdecl *)(void))0x70DC14)()==1);
    INV[8]=0x12345631;CHECK(((int (__cdecl *)(void))0x70DC14)()==0);
    U32(0xDD163C)=0;U32(0xDD1374)=200;CHECK(cmk_normal_allowed()==0);
    reset_data();CLOUD[0]=0x12345631;INV[0]=0x65432135;
    CHECK(call_exchange(CLOUD,INV)==0);CHECK(CLOUD[0]==0x12345631);CHECK(INV[0]==0x65432135);
    INV[0]=0x65432132;CHECK(call_exchange(CLOUD,INV)==1);CHECK(CLOUD[0]==0x65432132);CHECK(INV[0]==0x12345631);
    /* Cloud can be either endpoint, including his armor. */
    CLOUD[8]=0xABCDEF31;INV[1]=0x10203035;
    CHECK(call_exchange(INV+1,CLOUD+8)==0);CHECK(CLOUD[8]==0xABCDEF31);
    uint32_t other=0x44556635;INV[2]=0x77889949;
    CHECK(call_exchange(&other,INV+2)==1);CHECK(other==0x77889949);
    uint32_t *barret=(uint32_t *)(0xDBFDCC+0x84);barret[0]=0x12345635;INV[3]=0x65432131;
    CHECK(call_exchange(barret,INV+3)==0);CHECK(barret[0]==0x12345635);CHECK(INV[3]==0x65432131);
    INV[3]=0x6543210B;CHECK(call_exchange(barret,INV+3)==1);CHECK(barret[0]==0x6543210B);
    uint8_t mask[8]={1,1,1,1,1,1,1,1}, holes[8]={1,0,1,1,1,1,1,1};
    for(unsigned i=0;i<8;++i){CLOUD[i]=0x12345631;INV[i]=0x65432132;}
    INV[7]=0xABCDEF35;
    ((void (__cdecl *)(const uint8_t *,const uint8_t *,uint32_t *,uint32_t *))0x710BD0)(mask,mask,CLOUD,INV);
    CHECK(CLOUD[0]==0x12345631);CHECK(INV[0]==0x65432132);CHECK(INV[7]==0xABCDEF35);
    INV[7]=0x98765434;cmk_row_swap(mask,mask,CLOUD,INV);
    CHECK(CLOUD[0]==0x65432132);CHECK(CLOUD[7]==0x98765434);CHECK(INV[7]==0x12345631);
    INV[1]=0xABCDEF35;cmk_row_swap(holes,mask,CLOUD,INV);CHECK(INV[1]==0xABCDEF35);
    /* Existing saves: full 32-bit orb including AP survives relocation. */
    reset_data();CLOUD[0]=0x12345635;CLOUD[15]=0x65432101;
    int result=((int (__cdecl *)(int))0x5CB2CC)(0);
    CHECK(result==-1);CHECK(CLOUD[0]==EMPTY);CHECK(CLOUD[15]==EMPTY);
    CHECK(INV[0]==0x12345635);CHECK(INV[1]==0x65432101);
    reset_data();CLOUD[0]=0x12345635;
    CHECK(((int (__cdecl *)(int))0x6C545B)(0)==-1);CHECK(INV[0]==0x12345635);
    reset_data();barret[15]=0xABCDEF31;((int (__cdecl *)(int))0x5CB2CC)(0);
    CHECK(barret[15]==EMPTY);CHECK(INV[0]==0xABCDEF31);
    /* Full inventory: no deletion; forbidden effect masked for calculation. */
    reset_data();CLOUD[0]=0x98765435;for(unsigned i=0;i<200;++i)INV[i]=0x11223331;
    CHECK(((int (__cdecl *)(int))0x5CB2CC)(0)==-1);CHECK(CLOUD[0]==0x98765435);
    CHECK(INV[199]==0x11223331);INV[199]=EMPTY;
    ((int (__cdecl *)(int))0x5CB2CC)(0);CHECK(CLOUD[0]==EMPTY);CHECK(INV[199]==0x98765435);
    /* Do not migrate an uninitialized record. */
    reset_data();*(uint8_t *)0xDBFD8D=0;CLOUD[0]=0x12345635;
    CHECK(((int (__cdecl *)(int))0x5CB2CC)(0)==0x12345635);CHECK(INV[0]==EMPTY);
    /* All 32 accessory permissions, exact 9-character masks, guest bits preserved. */
    reset_data();
    ((int (__cdecl *)(int))0x7083ED)(2);
    for(unsigned a=0;a<32;++a){
      CHECK(*(uint16_t *)(0xDBCAEC+a*16)==(ACCESSORY_OWNERS[a]|0x600));
      for(unsigned c=0;c<9;++c)CHECK(jobs_accessory_allowed(c,a)==!!(ACCESSORY_OWNERS[a]&(1u<<c)));
    }
    uint8_t *cloud_record=(uint8_t *)0xDBFD8C;
    uint16_t *items=(uint16_t *)0xDC0234;
    reset_data();cloud_record[0x1E]=2; /* Earring belongs to mage characters */
    ((int (__cdecl *)(int))0x5CB2CC)(0);
    CHECK(cloud_record[0x1E]==255);CHECK(items[0]==(0x122|0x200));
    reset_data();cloud_record[0x1E]=2;items[0]=(0x122|(98<<9));
    ((int (__cdecl *)(int))0x6C545B)(0);
    CHECK(items[0]==(0x122|(99<<9)));CHECK(items[1]==0xFFFF);CHECK(cloud_record[0x1E]==255);
    reset_data();cloud_record[0x1E]=2;for(unsigned i=0;i<320;++i)items[i]=(0x122|(99<<9));
    ((int (__cdecl *)(int))0x5CB2CC)(0);CHECK(cloud_record[0x1E]==2);
    CHECK(items[319]==(0x122|(99<<9)));
    items[319]=0xFFFF;((int (__cdecl *)(int))0x5CB2CC)(0);
    CHECK(cloud_record[0x1E]==255);CHECK(items[319]==(0x122|0x200));
    reset_data();cloud_record[0x1E]=25;((int (__cdecl *)(int))0x5CB2CC)(0);
    CHECK(cloud_record[0x1E]==25);CHECK(items[0]==0xFFFF);

    /* Atomic row exchanges for every class on every character, including the
       last slot, armor slots, holes and empty-orb/guest behavior. */
    for(unsigned c=0;c<9;++c)for(unsigned j=0;j<25;++j){
      reset_data();memcpy(options,DEFAULT_JOBS,sizeof(options));options[c]=j;CHECK(jobs_configure(options));
      unsigned yes=0,no=0;while(!CLASS_MATERIA[j][yes])++yes;while(CLASS_MATERIA[j][no])++no;
      uint32_t *slots=(uint32_t *)(0xDBFDCC+c*0x84);
      for(unsigned i=0;i<8;++i){slots[8+i]=0x12345600u|yes;INV[i]=0x65432100u|yes;}
      INV[7]=0xABCDEF00u|no;
      cmk_row_swap(mask,mask,slots+8,INV);
      for(unsigned i=0;i<8;++i)CHECK(slots[8+i]==(0x12345600u|yes));
      CHECK(INV[7]==(0xABCDEF00u|no));INV[7]=0x65432100u|yes;
      cmk_row_swap(mask,mask,slots+8,INV);
      for(unsigned i=0;i<8;++i){CHECK(slots[8+i]==(0x65432100u|yes));CHECK(INV[i]==(0x12345600u|yes));}
      U32(0xDD163C)=c;U32(0xDD1374)=0;U32(0xDD1364)=0;
      INV[0]=0xABCDEF00u|no;CHECK(((int (__cdecl *)(void))0x70DC14)()==0);
      INV[0]=0xABCDEF00u|yes;CHECK(((int (__cdecl *)(void))0x70DC14)()==1);
      INV[0]=EMPTY;CHECK(((int (__cdecl *)(void))0x70DC14)()==1);
      *(uint8_t *)(0xDBFD8C+c*0x84)=10;INV[0]=no;CHECK(((int (__cdecl *)(void))0x70DC14)()==1);
    }
    memcpy(options,DEFAULT_JOBS,sizeof(options));CHECK(jobs_configure(options));reset_data();
    /* Stock/ESUI, normal/Exchange, high/low resolution: zero to nine emitted
       owner icons agree with the equip bridge. All nine remain on one row. */
    int black=find_class("Black Mage"),monk=find_class("Monk");CHECK(black>=0 && monk>=0);
    for(unsigned layout=0;layout<2;++layout){
      *(uint16_t *)0x70C84D=*(uint16_t *)0x70F3C5=layout?623:618;
      *(uint16_t *)0x70CA15=*(uint16_t *)0x70FCA9=layout?311:309;
      test_row_limit=layout?15:11;
      for(unsigned number=0;number<=9;++number){
        for(unsigned c=0;c<9;++c)options[c]=(int)c<(int)number?black:monk;
        CHECK(jobs_configure(options));
        for(unsigned site=0;site<4;++site){
          unsigned low=site&1;reset_data();U32(0xDD12CC)=10;U32(0xDD1534)=10;
          INV[12]=0xAABBCC31;sprite_calls=text_calls=0;
          int x=low?(layout?222:214):(layout?444:427),y=low?105:210;
          int edge=low?(layout?311:309):(layout?623:618);
          CHECK(call_name(0,2,name_sites[site],x,y,site*4)==77);
          CHECK(text_calls==1);CHECK(last_text_x==x);CHECK(last_text_y==y);
          CHECK(sprite_calls==number);CHECK(drawn_owner_mask()==((1u<<number)-1));
          if(number){
            int compact=number>5;
            int size=low?(compact?4:8):(compact?8:16);
            CHECK(last_icon_x+size==edge-(low?8:16));CHECK(drawn_x[0]>x+15);
            CHECK(last_icon_y==y+(low?2:4));CHECK(last_palette==1);
            float *v=low?lo_vertices:hi_vertices;
            CHECK(v[16]-v[0]==size);CHECK(v[9]-v[1]==size);
            if(number>1){
              int step=low?(compact?4:9):(compact?9:18);
              for(unsigned i=1;i<number;++i){CHECK(drawn_x[i]-drawn_x[i-1]==step);CHECK(drawn_x[i]>=drawn_x[i-1]+size);}
            }
            CHECK(last_depth>0.199f && last_depth<0.201f);CHECK(v[2]==last_depth);
          }
          for(unsigned c=0;c<9;++c){
            U32(0xDD163C)=c;U32(0xDD1374)=10;U32(0xDD1364)=2;
            CHECK(((int (__cdecl *)(void))0x70DC14)()==(c<number));
          }
        }
      }
    }
    /* Every valid or invalid Materia, for every catalogue choice, in all four
       list variants: unowned entries still render text and zero icons. */
    for(unsigned j=0;j<25;++j){
      for(unsigned c=0;c<9;++c)options[c]=(j+c)%25;CHECK(jobs_configure(options));
      for(unsigned id=0;id<256;++id)for(unsigned site=0;site<4;++site){
        reset_data();U32(0xDD12CC)=10;U32(0xDD1534)=10;INV[12]=0xABCDEF00u|id;
        text_calls=sprite_calls=0;
        CHECK(call_name(0,2,name_sites[site],site&1?222:444,site&1?102:204,0)==77);
        CHECK(text_calls==1);CHECK(drawn_owner_mask()==cmk_owner_mask(id));
      }
    }
    /* Different normal-menu contexts and ESUI's expanded last row. */
    for(unsigned c=0;c<9;++c)options[c]=black;CHECK(jobs_configure(options));reset_data();
    U32(0xDD12CC)=10;U32(0xDD12CC+0x38)=40;U32(0xDD1534)=10;INV[42]=49;
    sprite_calls=text_calls=0;CHECK(call_name(1,2,name_sites[0],444,204,0)==77);CHECK(sprite_calls==9);
    INV[24]=49;sprite_calls=0;CHECK(call_name(0,14,name_sites[2],451,204,0)==77);CHECK(sprite_calls==9);
    U32(0xDD1534)=199;sprite_calls=0;CHECK(call_name(0,2,name_sites[2],451,204,0)==77);CHECK(sprite_calls==0);
    U32(0xDD1534)=10;
    /* Full names fit before the first icon even for nine owners. */
    typedef struct {int x,y;const uint8_t *text;int color;float depth;} NameArgs;
    void jobs_render_name(const uint8_t *,const NameArgs *,int,int);
    uint8_t frame_space[160]={0},*frame=frame_space+128;
    *(int *)(frame-8)=2;*(int *)(frame-12)=15;*(int *)(frame-20)=15;
    /* Regression: MP Plus has five owners with the defaults, but all five
       normal icons fit. Check actual rendered sizes in every list/layout. */
    const uint8_t mp_plus_name[]={0x2D,0x30,0,0x30,0x4C,0x55,0x53,255};
    memcpy(options,DEFAULT_JOBS,sizeof(options));CHECK(jobs_configure(options));
    CHECK(cmk_owner_mask(0)==((1u<<0)|(1u<<3)|(1u<<4)|(1u<<6)|(1u<<7)));
    for(unsigned layout=0;layout<2;++layout){
      *(uint16_t *)0x70C84D=*(uint16_t *)0x70F3C5=layout?623:618;
      *(uint16_t *)0x70CA15=*(uint16_t *)0x70FCA9=layout?311:309;
      for(unsigned site=0;site<4;++site){
        int low=site&1;INV[12]=0;
        NameArgs args={low?(layout?222:214):(layout?444:427),low?102:204,mp_plus_name,7,0.2f};
        sprite_calls=text_calls=0;jobs_render_name(frame,&args,site>=2,low);
        CHECK(sprite_calls==5);CHECK(drawn_owner_mask()==cmk_owner_mask(0));CHECK(text_calls==1);
        float *v=low?lo_vertices:hi_vertices;
        CHECK(v[16]-v[0]==(low?8:16));CHECK(v[9]-v[1]==(low?8:16));
        CHECK(drawn_x[1]-drawn_x[0]==(low?9:18));
      }
    }
    /* Six or more owners stay compact even when a UI gives a wider row. */
    for(unsigned c=0;c<9;++c)options[c]=black;CHECK(jobs_configure(options));INV[12]=49;
    for(unsigned site=0;site<4;++site){
      int low=site&1;NameArgs args={low?150:300,low?102:204,test_name,7,0.2f};
      sprite_calls=text_calls=0;jobs_render_name(frame,&args,site>=2,low);
      CHECK(sprite_calls==9);CHECK(text_calls==1);
      float *v=low?lo_vertices:hi_vertices;
      CHECK(v[16]-v[0]==(low?4:8));CHECK(drawn_x[1]-drawn_x[0]==(low?4:9));
      CHECK(drawn_x[0]>args.x+(low?16:32));
    }
    const uint8_t wide_name[]={0x26,0x49,0x52,0x45,0x26,0x49,0x52,0x45,0x26,0x49,0x52,0x45,0x26,0x49,0x52,255};
    memset(font_metrics,10,sizeof(font_metrics));U32(0xDB958C)=(uint32_t)font_metrics;
    font_object[0x70/4]=(uint32_t)glyph_vertices;
    U32(0xDC100C)=U32(0xDC1010)=U32(0xDC1048)=(uint32_t)font_object;U32(0xDC105C)=7;
    INV[12]=49;
    for(unsigned number=1;number<=9;++number){
      for(unsigned c=0;c<9;++c)options[c]=c<number?black:monk;CHECK(jobs_configure(options));
      for(unsigned site=0;site<4;++site){
        int low=site&1;NameArgs args={low?222:444,low?102:204,wide_name,7,0.2f};sprite_calls=text_calls=glyph_calls=0;
        jobs_render_name(frame,&args,site>=2,low);
        CHECK(sprite_calls==number);CHECK(glyph_calls==15);CHECK(text_calls==0);
        CHECK(glyph_vertices[16]<(float)(drawn_x[0]+(low?7:0)));
        CHECK(glyph_vertices[6]==0.25f);
      }
    }
    for(unsigned c=0;c<9;++c)options[c]=black;CHECK(jobs_configure(options));
    U32(0xDB958C)=0;
    /* Graphics pointers can be unavailable temporarily without crashing. */
    U32(0xDC0FC8+8)=0;sprite_calls=text_calls=0;
    CHECK(call_name(0,2,name_sites[0],444,204,0)==77);CHECK(sprite_calls==9);CHECK(text_calls==1);
    U32(0xDC0FC8+8)=(uint32_t)hi_object;
    /* All 32 accessories, six actual name bridges, stock and ESUI layouts.
       Icons decode to the exact same nine-character equip permission mask. */
    U32(0xDC0FC8)=U32(0xDC0FC8+8)=(uint32_t)hi_object;U32(0xDB958C)=0;
    uint8_t accessory_locals[128]={0};
    for(unsigned layout=0;layout<2;++layout){
      *(uint16_t *)0x705DB4=layout?623:618;*(uint16_t *)0x706732=layout?311:309;
      *(uint16_t *)0x715941=layout?555:548;*(uint16_t *)0x7162FE=layout?277:274;
      for(unsigned a=0;a<32;++a){
        unsigned owners=jobs_accessory_owners(a),number=0;
        for(unsigned c=0;c<9;++c){CHECK(jobs_accessory_allowed(c,a)==!!(owners&(1u<<c)));if(owners&(1u<<c))++number;}
        CHECK(number>=1 && number<=4);
        for(unsigned site=0;site<6;++site){
          unsigned low=site&1,view=site/2;reset_data();U32(0xDCA5C4)=2;
          *(unsigned *)(accessory_locals+128-0x18)=0x120+a;
          *(unsigned *)(accessory_locals+128-0x20)=7;
          *(unsigned *)(accessory_locals+128-0x3C)=0x120+a;
          *(uint8_t *)(0xDBFD8C+7*0x84+0x1E)=(uint8_t)a;
          int x=view==2?(low?186:373):view==1?(low?152:(layout?331:303)):(low?214:(layout?443:427));
          int edge=view==2?(low?(layout?277:274):(layout?555:548)):(low?(layout?311:309):(layout?623:618));
          sprite_calls=text_calls=glyph_calls=0;
          CHECK(call_accessory(accessory_locals,accessory_sites[site],x,low?55:110)==77);
          CHECK(text_calls==1);CHECK(last_text_x==x);CHECK(last_text_y==(low?55:110));
          CHECK(sprite_calls==number);CHECK(drawn_owner_mask()==owners);CHECK(last_palette==1);
          CHECK(last_icon_x+(low?8:16)==edge-(low?8:16));
          CHECK(drawn_x[0]==last_icon_x-(int)(number-1)*(low?9:18));
          CHECK(last_icon_y==(low?57:114));
          /* Weapon/armor labels and unknown/empty accessory IDs stay untouched. */
          if(view==0)U32(0xDCA5C4)=1;
          if(view==1)*(uint8_t *)(0xDBFD8C+7*0x84+0x1E)=255;
          if(view==2)*(unsigned *)(accessory_locals+128-0x3C)=0x80;
          sprite_calls=text_calls=0;
          CHECK(call_accessory(accessory_locals,accessory_sites[site],x,110)==77);
          CHECK(sprite_calls==0);CHECK(text_calls==1);
        }
      }
    }
    CHECK(jobs_accessory_owners(32)==0);CHECK(jobs_accessory_owners(255)==0);CHECK(jobs_accessory_owners(0xFFFFFFFF)==0);
    /* Condense complete long accessory names before the first of four icons. */
    void jobs_render_accessory(const uint8_t *,const NameArgs *,int,int);
    U32(0xDB958C)=(uint32_t)font_metrics;
    for(int view=0;view<3;++view)for(int low=0;low<2;++low){
      U32(0xDCA5C4)=2;
      *(unsigned *)(accessory_locals+128-0x18)=0x122;
      *(unsigned *)(accessory_locals+128-0x20)=7;
      *(unsigned *)(accessory_locals+128-0x3C)=0x122;
      *(uint8_t *)(0xDBFD8C+7*0x84+0x1E)=2;
      NameArgs args={view==2?(low?186:373):(low?214:443),low?55:110,wide_name,7,0.2f};
      sprite_calls=glyph_calls=text_calls=0;
      jobs_render_accessory(accessory_locals+128,&args,view,low);
      CHECK(sprite_calls==4);CHECK(glyph_calls==15);CHECK(text_calls==0);
      CHECK(glyph_vertices[16]<(float)(drawn_x[0]+(low?7:0)));
      CHECK(glyph_vertices[6]==0.25f);CHECK(drawn_owner_mask()==ACCESSORY_OWNERS[2]);
    }
    if(argc>2 && strcmp(argv[1],"esui")==0){
      FILE *f=fopen(esui_fixture,"rb");CHECK(f!=NULL);
      CHECK(fread((void *)0x914000,1,0x415,f)==0x415);fclose(f);
      static const uint8_t esui_entry[9]={0xE9,0x36,0x28,0x1F,0,0x90,0x90,0x90,0x90};
      write_bytes(0x7217C5,esui_entry,9);*(uint8_t *)0x7217CE=0xC3;
      uint8_t original_stub[0x415];memcpy(original_stub,(void *)0x914000,sizeof(original_stub));
      DWORD old;MEMORY_BASIC_INFORMATION m;
      CHECK(VirtualProtect((void *)0x914000,0x415,PAGE_READWRITE,&old));
      *(uint8_t *)0x7217C5=0x90;CHECK(jobs_esui_compat()==0);
      *(uint8_t *)0x7217C5=0xE9;*(uint8_t *)0x914000^=1;CHECK(jobs_esui_compat()==0);
      *(uint8_t *)0x914000^=1;
      CHECK(VirtualProtect((void *)0x914000,0x415,PAGE_NOACCESS,&old));CHECK(jobs_esui_compat()==0);
      CHECK(VirtualProtect((void *)0x914000,0x415,PAGE_READWRITE,&old));
      CHECK(jobs_esui_compat()==1);CHECK(jobs_esui_compat()==1);
      VirtualQuery((void *)0x914000,&m,sizeof(m));CHECK((m.Protect&255)==PAGE_EXECUTE_READWRITE);
      CHECK(memcmp(original_stub,(void *)0x914000,sizeof(original_stub))==0);
      U32(0xDD6F24)=0;((void (__cdecl *)(void))0x914000)();
      CHECK(U32(0x99DDA8)!=0); /* Exact installed ESUI routine executed safely. */
    }
    printf("PASS %u native x86 checks\n",count);return 0;
}
