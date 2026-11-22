/* FFVII Job System 0.8.0 - English FF7 PC 1.02, freestanding 7th Heaven HEXT runtime.
 * Original implementation. No Square Enix executable or assets are distributed.
 * Fixed addresses were verified against the user's English Steam executable.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include "rules.generated.h"

#define API __declspec(dllexport)
#include "hext_api.h"
#define CLOUD ((uint32_t *)0xDBFDCC)
#define INVENTORY ((uint32_t *)0xDC04B4)
#define EMPTY 0xFFFFFFFFu
#define U32(a) (*(uint32_t *)(a))
static void *stat_original, *magic_original, *equipment_original;
static volatile LONG rejects, returned_materia, overflow_masks;
static int installed;

static void log_line(const char *line) {
    HANDLE h = CreateFileA("FFVIIJobs.log", FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD written; WriteFile(h, line, (DWORD)strlen(line), &written, NULL); CloseHandle(h);
    }
}
static int selected_jobs[9]=DEFAULT_JOB_VALUES;
static unsigned short materia_owners[256];
static int owners_ready;
static int settings_pending,settings_failed;
static void rebuild_owners(void){
    memset(materia_owners,0,sizeof(materia_owners));
    for(unsigned c=0;c<9;++c)for(unsigned id=0;id<256;++id)
        if(CLASS_MATERIA[selected_jobs[c]][id])materia_owners[id]|=(unsigned short)(1u<<c);
    owners_ready=1;
}
API int jobs_configure(const int *choices) {
    if(!choices)return 0;
    for(unsigned c=0;c<9;++c)if(choices[c]<0 || choices[c]>=CLASS_COUNT)return 0;
    memcpy(selected_jobs,choices,sizeof(selected_jobs));
    settings_pending=0;settings_failed=0;
    rebuild_owners();
    log_line("CONFIGURED: one selected class per character.\r\n");
    for(unsigned c=0;c<9;++c){log_line(CHARACTER_NAMES[c]);log_line(": ");log_line(CLASS_NAMES[selected_jobs[c]]);log_line("\r\n");}
    return 1;
}
API int jobs_selected(unsigned c){return c<9?selected_jobs[c]:-1;}
API int jobs_default(unsigned c){return c<9?DEFAULT_JOBS[c]:-1;}
API unsigned jobs_class_count(void){return CLASS_COUNT;}
API const char *jobs_catalogue_name(unsigned j){return j<CLASS_COUNT?CLASS_NAMES[j]:NULL;}
API const char *jobs_class_name(unsigned c){return c<9?CLASS_NAMES[selected_jobs[c]]:NULL;}
/* 7H activates one ModFolder per character. Older releases load native DLLs
 * before their file hooks are active, so DllMain defers these reads until the
 * first game permission/stat/menu call. Absolute paths ensure files absent
 * from the base game resolve through 7H's virtual archive mapping.
 * No profile XML, app settings, save data or game installation is rewritten.
 */
API int jobs_read_settings(const wchar_t *base_directory){
    wchar_t base[1024],path[1200];
    if(base_directory){
        size_t n=wcslen(base_directory);if(!n || n>=1024)return 0;
        memcpy(base,base_directory,(n+1)*sizeof(wchar_t));
    }else{
        DWORD n=GetModuleFileNameW(NULL,base,1024);if(!n || n>=1024)return 0;
        wchar_t *last=wcsrchr(base,L'\\');if(!last)return 0;*last=0;
    }
    int choices[9];memcpy(choices,DEFAULT_JOBS,sizeof(choices));
    unsigned missing=0;
    for(unsigned c=0;c<9;++c){
        size_t len=wcslen(base);memcpy(path,base,(len+1)*sizeof(wchar_t));
        const wchar_t *suffix=L"\\ffvii-job-system-v070\\";
        wcscat(path,suffix);len=wcslen(path);
        for(const char *k=CHARACTER_KEYS[c];*k;++k)path[len++]=(wchar_t)*k;
        path[len]=0;wcscat(path,L".class");
        HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file==INVALID_HANDLE_VALUE){
            DWORD error=GetLastError();
            if(error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND){++missing;continue;}
            log_line("DISABLED: could not read a class selection file.\r\n");return 0;
        }
        char value[96];DWORD read=0;
        BOOL ok=ReadFile(file,value,sizeof(value)-1,&read,NULL);CloseHandle(file);
        if(!ok || read==sizeof(value)-1){log_line("DISABLED: invalid class selection file.\r\n");return 0;}
        while(read && (value[read-1]=='\r' || value[read-1]=='\n'))--read;
        value[read]=0;int match=-1;
        for(unsigned j=0;j<CLASS_COUNT;++j)
            if(read==strlen(CLASS_KEYS[j]) && !memcmp(value,CLASS_KEYS[j],read)){match=(int)j;break;}
        if(match<0){log_line("DISABLED: unknown class selection; permissions unchanged.\r\n");return 0;}
        choices[c]=match;
    }
    if(missing)log_line("CONFIG: missing selection files use the documented character defaults.\r\n");
    return jobs_configure(choices);
}
API int jobs_initialize(void){
    if(settings_pending){
        settings_pending=0;
        if(!jobs_read_settings(NULL)){
            settings_failed=1;
            log_line("CONFIG ERROR: Materia locked for this launch because class settings could not be read.\r\n");
        }
    }
    return !settings_failed;
}
#ifdef CMK_TEST_BUILD
API void jobs_test_defer_settings(void){settings_pending=1;settings_failed=0;}
#endif
API unsigned cmk_owner_mask(uint32_t value) {
    if(!jobs_initialize())return 0;
    if(!owners_ready)rebuild_owners();
    return materia_owners[value&255u];
}
API int cmk_can_equip(unsigned character,uint32_t value) {
    if ((value&255u)==255u || character>=9) return 1;
    unsigned owners=cmk_owner_mask(value);
    return (owners & (1u<<character))!=0;
}
API int cmk_allowed(uint32_t value){return cmk_can_equip(0,value);}
API int jobs_accessory_allowed(unsigned character,unsigned id){
    return id==255 || character>=9 || (id<32 && (ACCESSORY_OWNERS[id]&(1u<<character))!=0);
}
/* Stock equipment selection already checks this kernel bitmask. Apply the
 * character ownership before its list is built; weapon/armor masks are untouched.
 * Retain guest bits for the flashback and intersect normal characters with the
 * current kernel's own availability restrictions.
 */
static void prepare_accessory_masks(void){
    for(unsigned i=0;i<32;++i){
        uint16_t *mask=(uint16_t *)(0xDBCAEC+i*16);
        *mask=(uint16_t)((*mask&~0x1FFu)|(*mask&ACCESSORY_OWNERS[i]));
    }
}
API int __cdecl jobs_equipment_hook(int type){
    prepare_accessory_masks();
    return ((int (__cdecl *)(int))equipment_original)(type);
}
static int character_slot(const uint32_t *p) {
    uintptr_t a=(uintptr_t)p;
    if(a<0xDBFD8C || a>=0xDBFD8C+9*0x84 || (a&3))return -1;
    unsigned character=(unsigned)(a-0xDBFD8C)/0x84;
    unsigned offset=(unsigned)(a-0xDBFD8C)%0x84;
    if(offset<0x40 || offset>=0x80)return -1;
    /* The flashback may reuse a character record for a guest. */
    if(*(uint8_t *)(0xDBFD8C+character*0x84)!=character)return -1;
    return (int)character;
}
API int cmk_swap_allowed(uint32_t *a, uint32_t *b) {
    int ca=character_slot(a),cb=character_slot(b);
    return (ca<0 || cmk_can_equip((unsigned)ca,*b)) && (cb<0 || cmk_can_equip((unsigned)cb,*a));
}
API int cmk_normal_allowed(void) {
    unsigned character=U32(0xDD163C); /* record index, not party position */
    if(character>=9 || *(uint8_t *)(0xDBFD8C+character*0x84)!=character)return 1;
    uint32_t index = U32(0xDD1374) + U32(0xDD1364);
    return index < 200 && cmk_can_equip(character,INVENTORY[index]);
}
API void cmk_rejected(void) {
    if (InterlockedIncrement(&rejects) == 1) log_line("Rejected a Materia operation by character ownership.\r\n");
    ((void (__cdecl *)(int))0x74580A)(3); /* stock error sound */
}

/* These two bridges run on the game's own thread at instruction boundaries.
 * pushad/pushfd preserve the registers/flags expected by the original code.
 */
__attribute__((naked)) void normal_bridge(void) {
    __asm__ volatile (
        ".intel_syntax noprefix\n"
        "pushfd\n pushad\n call _cmk_normal_allowed\n test eax,eax\n jz 1f\n"
        "popad\n popfd\n push 0x1bf\n push 0x70dc19\n ret\n"
        "1: call _cmk_rejected\n popad\n popfd\n push 0x70dd64\n ret\n"
        ".att_syntax prefix\n");
}
__attribute__((naked)) void exchange_bridge(void) {
    __asm__ volatile (
        ".intel_syntax noprefix\n"
        "pushfd\n pushad\n push DWORD PTR [ebp-16]\n push DWORD PTR [ebp-4]\n"
        "call _cmk_swap_allowed\n add esp,8\n test eax,eax\n jz 1f\n"
        "popad\n popfd\n mov edx,DWORD PTR [ebp-4]\n mov eax,DWORD PTR [edx]\n"
        "push 0x71043c\n ret\n"
        "1: call _cmk_rejected\n popad\n popfd\n push 0x71078e\n ret\n"
        ".att_syntax prefix\n");
}
/* Replacement for the original eight-slot row exchange. Validate the complete
 * row before making any writes, so a rejected row is an atomic no-op.
 */
API void __cdecl cmk_row_swap(const uint8_t *mask_a, const uint8_t *mask_b,
                              uint32_t *a, uint32_t *b) {
    for (unsigned i=0; i<8; ++i) {
        if (mask_a[i] && mask_b[i] && !cmk_swap_allowed(a+i,b+i)) {
            cmk_rejected(); return;
        }
    }
    for (unsigned i=0; i<8; ++i) {
        if (mask_a[i] && mask_b[i]) { uint32_t t=a[i]; a[i]=b[i]; b[i]=t; }
    }
}
typedef struct { uint32_t saved[9][16]; unsigned mask[9]; uint8_t accessory[9];unsigned accessory_mask; } Masked;
static int return_accessory(unsigned id){
    uint16_t *inventory=(uint16_t *)0xDC0234;
    unsigned empty=320,item=0x120+id;
    for(unsigned i=0;i<320;++i){
        if(inventory[i]==0xFFFF){if(empty==320)empty=i;continue;}
        if((inventory[i]&0x1FF)==item && (inventory[i]>>9)<99){inventory[i]+=0x200;return 1;}
    }
    if(empty==320)return 0;
    inventory[empty]=(uint16_t)(item|0x200);return 1;
}
static void prepare_permissions(Masked *state) {
    jobs_initialize();
    memset(state->mask,0,sizeof(state->mask));
    state->accessory_mask=0;
    prepare_accessory_masks();
    for(unsigned character=0;character<9;++character) {
      uint8_t *record=(uint8_t *)(0xDBFD8C+character*0x84);
      if(record[0]!=character || record[1]==0 || record[1]>99)continue;
      unsigned accessory=record[0x1E];
      if(!jobs_accessory_allowed(character,accessory)){
        /* Unknown/modified IDs are preserved rather than converted to another item. */
        if(accessory>=32 || !return_accessory(accessory)){
            state->accessory[character]=(uint8_t)accessory;state->accessory_mask|=1u<<character;
        }
        record[0x1E]=255;
      }
      uint32_t *slots=(uint32_t *)(record+0x40);
      for (unsigned i=0; i<16; ++i) {
        uint32_t value=slots[i];
        if (cmk_can_equip(character,value)) continue;
        unsigned free_slot=0;
        while (free_slot<200 && INVENTORY[free_slot]!=EMPTY) ++free_slot;
        if (free_slot<200) {
            INVENTORY[free_slot]=value; slots[i]=EMPTY;
            if (InterlockedIncrement(&returned_materia)==1)
                log_line("Returned existing forbidden Materia to inventory, including AP.\r\n");
        } else {
            /* A full inventory must never destroy an orb. Hide its effects only
             * while derived stats/commands are being rebuilt; restore afterward.
             * Once a slot becomes free the next rebuild returns it normally.
             */
            state->saved[character][i]=value; state->mask[character] |= 1u<<i; slots[i]=EMPTY;
            if (InterlockedIncrement(&overflow_masks)==1)
                log_line("Inventory full: forbidden equipped Materia temporarily inert; free inventory space.\r\n");
        }
      }
    }
}
static void restore_masked(const Masked *state) {
    for(unsigned c=0;c<9;++c)for(unsigned i=0;i<16;++i)
        if(state->mask[c] & (1u<<i))((uint32_t *)(0xDBFDCC+c*0x84))[i]=state->saved[c][i];
    for(unsigned c=0;c<9;++c)if(state->accessory_mask&(1u<<c))
        *(uint8_t *)(0xDBFD8C+c*0x84+0x1E)=state->accessory[c];
}
API int __cdecl cmk_stat_hook(int party_slot) {
    Masked state; prepare_permissions(&state);
    int result=((int (__cdecl *)(int))stat_original)(party_slot);
    restore_masked(&state); return result;
}
API int __cdecl cmk_magic_hook(int party_slot) {
    Masked state; prepare_permissions(&state);
    int result=((int (__cdecl *)(int))magic_original)(party_slot);
    restore_masked(&state); return result;
}
/* Inventory label hooks use the stock sprite atlas and stock name renderer.
 * Cloud's sword occupies (112,112)-(128,128) on BTL_WIN_H / BTL_WIN_L.
 * High-resolution sprite rendering doubles this source rectangle; halving
 * the resulting quad yields a compact 16px icon (8px in low resolution).
 */
typedef struct {int x,y;const uint8_t *text;int color;float depth;} NameArgs;
/* Guard graphics pointers because another UI mod can replace the font objects. */
static int memory_range(uintptr_t address,size_t size,int writable){
    MEMORY_BASIC_INFORMATION m;
    if(!address || !VirtualQuery((void *)address,&m,sizeof(m)) || m.State!=MEM_COMMIT ||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS)) || address+size<address ||
       address+size>(uintptr_t)m.BaseAddress+m.RegionSize)return 0;
    DWORD p=m.Protect&255;
    return writable?(p==PAGE_READWRITE || p==PAGE_WRITECOPY || p==PAGE_EXECUTE_READWRITE || p==PAGE_EXECUTE_WRITECOPY):
        (p==PAGE_READONLY || p==PAGE_READWRITE || p==PAGE_WRITECOPY || p==PAGE_EXECUTE_READ || p==PAGE_EXECUTE_READWRITE || p==PAGE_EXECUTE_WRITECOPY);
}
static float *quad_vertices(uintptr_t object_address){
    uintptr_t object=U32(object_address);
    if(!memory_range(object,0x74,0))return NULL;
    float *v=*(float **)(object+0x70);
    return memory_range((uintptr_t)v,32*sizeof(float),1)?v:NULL;
}
static void scale_last_quad(uintptr_t object_address,float scale) {
    float *v=quad_vertices(object_address);
    if(!v)return;
    float x=v[0],y=v[1];
    for(unsigned i=1;i<4;++i){v[i*8]=x+(v[i*8]-x)*scale;v[i*8+1]=y+(v[i*8+1]-y)*scale;}
}
/* Follow the actual scrollbar, including ESUI's shifted list. Leave a full
 * compact icon's width of empty space before it (16 high / 8 low pixels). */
static int icon_right(const NameArgs *args,int exchange,int low){
    uintptr_t at=exchange?(low?0x70FCA9:0x70F3C5):(low?0x70CA15:0x70C84D);
    int scrollbar=*(const uint16_t *)at;
    if(scrollbar<args->x+(low?32:64) || scrollbar>(low?320:640))scrollbar=low?309:618;
    return scrollbar-(low?16:32);
}
static unsigned owner_count(unsigned owners){
    unsigned number=0;for(unsigned c=0;c<9;++c)if(owners&(1u<<c))++number;return number;
}
/* Five to nine owners use smaller icons, keeping all icons on the same row
 * and reserving enough space for the Materia name in stock UI and ESUI. */
static int owner_first_x(unsigned owners,int right,int low){
    unsigned number=owner_count(owners);
    int small=number>4,step=low?(small?5:10):(small?10:20);
    return right+(small?(low?4:8):0)-(number?(int)(number-1)*step:0);
}
static void draw_owner_icons(const NameArgs *args,unsigned owners,int right,int low){
    if(!owners)return;
    /* Actual stock Items renderer (0x717691), verified using each character's
     * starting weapon. Barret is the arm gun (96,128); Red XIII the clip
     * (112,144). The identities match the original Items menu.
     */
    static const uint8_t uv[9][2]={{112,112},{96,128},{112,128},{96,144},
        {112,144},{96,160},{112,160},{96,176},{112,176}};
    static const unsigned weapon[9]={0x80,0xA0,0x90,0xBE,0xB0,0xD7,0xE5,0xF2,0xC9};
    int small=owner_count(owners)>4;
    int step=low?(small?5:10):(small?10:20);
    int x=owner_first_x(owners,right,low),y=args->y+(low?2:4);
    for(unsigned c=0;c<9;++c)if(owners&(1u<<c)) {
        /* Use the Items menu renderer, so the active UI mod supplies its art. */
        ((void (__cdecl *)(int,int,int,int,float))(low?0x7177A8:0x717691))
            (x,y,(int)weapon[c],0,args->depth);
        scale_last_quad(low?0xDC104C:(0xDC0FC8+((uv[c][0]>=128?1:0)|(uv[c][1]>=128?2:0))*4),small?0.25f:0.5f);
        x+=step;
    }
}
API unsigned cmk_before_name(const uint8_t *frame,const NameArgs *args,int exchange,int low) {
    int row=*(const int *)(frame-8);
    int rows=*(const int *)(frame-(exchange?0x14:0xC));
    if(rows<1 || rows>32)rows=11;
    if(row<0 || row>=rows)return 0;
    unsigned scroll;
    if(exchange)scroll=U32(0xDD1534);
    else {
        unsigned context=*(const unsigned *)(frame+8);
        if(context>3)return 0;
        scroll=U32(0xDD12CC+context*0x38);
    }
    if(scroll>=200 || (unsigned)row>=200-scroll)return 0;
    unsigned owners=cmk_owner_mask(INVENTORY[scroll+row]);
    if(!owners)return 0;
    draw_owner_icons(args,owners,icon_right(args,exchange,low),low);
    return owners;
}
/* Keep full original names. Only names too wide for their icon reservation are
 * condensed horizontally, using the game's own glyphs and current font metrics.
 * The rest keep the unchanged stock text renderer. No font/texture replacement.
 */
static void render_owned_text(const NameArgs *args,unsigned owners,int right,int low){
    unsigned number=owner_count(owners);
    const uint8_t *font=(const uint8_t *)(uintptr_t)U32(0xDB958C);
    if(font && !memory_range((uintptr_t)font,256,0))font=NULL;
    unsigned length=0,width=0;
    if(font && memory_range((uintptr_t)font,256,0) && args->text){
        while(length<64 && args->text[length]!=255 && args->text[length]<0xF8){
            uint8_t metric=font[args->text[length++]];
            width+=((metric>>5)+(metric&31))*(low?1:2);
        }
    }
    int boundary=number?owner_first_x(owners,right,low)-(low?3:6):right+(low?8:16);
    unsigned available=(unsigned)(boundary>args->x?boundary-args->x:1);
    if(!font || !length || length>=64 || args->text[length]!=255 || width<=available){
        ((void (__cdecl *)(int,int,const uint8_t *,int,float))(low?0x6FAB2F:0x6F5B03))
            (args->x,args->y,args->text,args->color,args->depth);
        return;
    }
    float scale=(float)available/(float)width;
    int cursor=0,offset=low?(int)U32(0xDC105C):0;
    for(unsigned i=0;i<length;++i){
        unsigned ch=args->text[i];
        int advance=((int (__cdecl *)(int,int,int,int,float))(low?0x6FA706:0x6F564E))
            (0,args->y,args->color,(int)ch,args->depth);
        float *vertices=quad_vertices(low?0xDC1048:((ch%21)*24>240?0xDC1010:0xDC100C));
        if(vertices)for(unsigned v=0;v<4;++v)
            vertices[v*8]=(float)(args->x+offset)+((float)cursor+vertices[v*8]-(float)offset)*scale;
        cursor+=advance;
    }
}
API void jobs_render_name(const uint8_t *frame,const NameArgs *args,int exchange,int low){
    unsigned owners=cmk_before_name(frame,args,exchange,low);
    render_owned_text(args,owners,icon_right(args,exchange,low),low);
}
API unsigned jobs_accessory_owners(unsigned id){return id<32?ACCESSORY_OWNERS[id]:0;}
/* The name's ID is already available in the original menu's EBP locals:
 * Equip list: item ID at -0x18; equipped summary: character index at -0x20;
 * Items Use/Arrange lists: item ID at -0x3C (quantity bits already removed).
 */
API void jobs_render_accessory(const uint8_t *frame,const NameArgs *args,int view,int low){
    unsigned id=255;
    if(view==0){
        unsigned item=*(const unsigned *)(frame-0x18);
        if(U32(0xDCA5C4)==2 && item>=0x120 && item<0x140)id=item-0x120;
    }else if(view==1){
        unsigned character=*(const unsigned *)(frame-0x20);
        if(character<9){
            const uint8_t *record=(const uint8_t *)(0xDBFD8C+character*0x84);
            if(record[0]==character)id=record[0x1E];
        }
    }else if(view==2){
        unsigned item=*(const unsigned *)(frame-0x3C);
        if(item>=0x120 && item<0x140)id=item-0x120;
    }
    unsigned owners=jobs_accessory_owners(id);
    if(!owners){
        ((void (__cdecl *)(int,int,const uint8_t *,int,float))(low?0x6FAB2F:0x6F5B03))
            (args->x,args->y,args->text,args->color,args->depth);
        return;
    }
    /* Equip labels align before the equipment scrollbar. Items labels align
     * before the colon/quantity column, which must remain clear. Both positions
     * follow the actual instruction operands so ESUI layout changes are used.
     */
    int edge=*(const uint16_t *)(view==2?(low?0x7162FE:0x715941):(low?0x706732:0x705DB4));
    if(edge<args->x+(low?32:64) || edge>(low?320:640))edge=view==2?(low?274:548):(low?309:618);
    int right=edge-(low?16:32);
    draw_owner_icons(args,owners,right,low);
    render_owned_text(args,owners,right,low);
}
#define ACCESSORY_BRIDGE(name,view,low) \
__attribute__((naked)) void name(void) { \
    __asm__ volatile(".intel_syntax noprefix\n pushfd\n pushad\n lea eax,[esp+40]\n" \
        "push " #low "\n push " #view "\n push eax\n push ebp\n" \
        "call _jobs_render_accessory\n add esp,16\n popad\n popfd\n ret\n .att_syntax prefix\n"); \
}
ACCESSORY_BRIDGE(accessory_list_hi,0,0)
ACCESSORY_BRIDGE(accessory_list_lo,0,1)
ACCESSORY_BRIDGE(accessory_equipped_hi,1,0)
ACCESSORY_BRIDGE(accessory_equipped_lo,1,1)
ACCESSORY_BRIDGE(accessory_items_hi,2,0)
ACCESSORY_BRIDGE(accessory_items_lo,2,1)
#define NAME_BRIDGE(name,exchange,low,target) \
__attribute__((naked)) void name(void) { \
    __asm__ volatile(".intel_syntax noprefix\n pushfd\n pushad\n lea eax,[esp+40]\n" \
        "push " #low "\n push " #exchange "\n push eax\n push ebp\n" \
        "call _jobs_render_name\n add esp,16\n popad\n popfd\n" \
        "ret\n .att_syntax prefix\n"); \
}
NAME_BRIDGE(name_normal_hi,0,0,0x6F5B03)
NAME_BRIDGE(name_normal_lo,0,1,0x6FAB2F)
NAME_BRIDGE(name_exchange_hi,1,0,0x6F5B03)
NAME_BRIDGE(name_exchange_lo,1,1,0x6FAB2F)

typedef struct { uintptr_t address; const void *replacement; unsigned size;
    uint8_t expected[8]; DWORD old_protect; uint8_t original[8]; } Patch;
static Patch patches[] = {
    {0x70DC14,normal_bridge,5,{0x68,0xBF,0x01,0,0},0,{0}},
    {0x710437,exchange_bridge,5,{0x8B,0x55,0xFC,0x8B,0x02},0,{0}},
    {0x710BD0,cmk_row_swap,6,{0x55,0x8B,0xEC,0x83,0xEC,0x08},0,{0}},
    {0x5CB2CC,cmk_stat_hook,6,{0x55,0x8B,0xEC,0x83,0xEC,0x24},0,{0}},
    {0x6C545B,cmk_magic_hook,6,{0x55,0x8B,0xEC,0x83,0xEC,0x28},0,{0}},
    {0x70C813,name_normal_hi,5,{0xE8,0xEB,0x92,0xFE,0xFF},0,{0}},
    {0x70C9DE,name_normal_lo,5,{0xE8,0x4C,0xE1,0xFE,0xFF},0,{0}},
    {0x70F394,name_exchange_hi,5,{0xE8,0x6A,0x67,0xFE,0xFF},0,{0}},
    {0x70FC78,name_exchange_lo,5,{0xE8,0xB2,0xAE,0xFE,0xFF},0,{0}},
    {0x7083ED,jobs_equipment_hook,6,{0x55,0x8B,0xEC,0x83,0xEC,0x10},0,{0}},
    {0x705ED1,accessory_list_hi,5,{0xE8,0x2D,0xFC,0xFE,0xFF},0,{0}},
    {0x706857,accessory_list_lo,5,{0xE8,0xD3,0x42,0xFF,0xFF},0,{0}},
    {0x7063C7,accessory_equipped_hi,5,{0xE8,0x37,0xF7,0xFE,0xFF},0,{0}},
    {0x706D49,accessory_equipped_lo,5,{0xE8,0xE1,0x3D,0xFF,0xFF},0,{0}},
    {0x715846,accessory_items_hi,5,{0xE8,0xB8,0x02,0xFE,0xFF},0,{0}},
    {0x716203,accessory_items_lo,5,{0xE8,0x27,0x49,0xFE,0xFF},0,{0}}
};
#define PATCH_COUNT (sizeof(patches)/sizeof(patches[0]))
static void jump_bytes(uint8_t *buffer, uintptr_t at, const void *dest) {
    buffer[0]=0xE9; int32_t rel=(int32_t)((uintptr_t)dest-at-5); memcpy(buffer+1,&rel,4);
}
static void *trampoline(Patch *p) {
    uint8_t *t=VirtualAlloc(NULL,16,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if (!t) return NULL;
    memcpy(t,(void *)p->address,p->size);
    jump_bytes(t+p->size,(uintptr_t)t+p->size,(void *)(p->address+p->size));
    DWORD prev; VirtualProtect(t,16,PAGE_EXECUTE_READ,&prev);
    FlushInstructionCache(GetCurrentProcess(),t,16); return t;
}
/* ESUI 2.968 places a startup font routine in the executable's .data page.
 * The 7H HEXT writer can leave that page non-executable. Match both its entry
 * jump and the full known routine before restoring this one page's permission.
 * No ESUI bytes or textures are bundled or modified.
 */
API int jobs_esui_compat(void){
    static const uint8_t entry[9]={0xE9,0x36,0x28,0x1F,0,0x90,0x90,0x90,0x90};
    if(!memory_range(0x7217C5,sizeof(entry),0) || memcmp((void *)0x7217C5,entry,sizeof(entry)) ||
       !memory_range(0x914000,0x415,0))return 0;
    uint32_t hash=2166136261u;
    for(unsigned i=0;i<0x415;++i)hash=(hash^*(const uint8_t *)(0x914000+i))*16777619u;
    if(hash!=0x46EDD751u)return 0;
    MEMORY_BASIC_INFORMATION m;VirtualQuery((void *)0x914000,&m,sizeof(m));
    if((m.Protect&255)==PAGE_EXECUTE_READ || (m.Protect&255)==PAGE_EXECUTE_READWRITE ||
       (m.Protect&255)==PAGE_EXECUTE_WRITECOPY)return 1;
    DWORD old;
    if(!VirtualProtect((void *)0x914000,0x415,PAGE_EXECUTE_READWRITE,&old)){
        log_line("ESUI: recognized startup font routine, but could not restore executable permission.\r\n");return 0;
    }
    FlushInstructionCache(GetCurrentProcess(),(void *)0x914000,0x415);
    log_line("ESUI: restored executable permission for the verified startup font routine.\r\n");
    return 1;
}
API int cmk_install(void) {
    if (installed) return 1;
    for (unsigned i=0;i<PATCH_COUNT;++i) {
        Patch *p=patches+i;
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((void *)p->address,&mbi,sizeof(mbi)) || mbi.State!=MEM_COMMIT ||
            memcmp((void *)p->address,p->expected,p->size)) {
            log_line("DISABLED: unsupported executable or conflicting code patch. No hooks installed.\r\n");
            return 0;
        }
    }
    stat_original=trampoline(patches+3); magic_original=trampoline(patches+4);
    equipment_original=trampoline(patches+9);
    if (!stat_original || !magic_original || !equipment_original) {
        log_line("DISABLED: could not allocate trampolines.\r\n"); return 0;
    }
    unsigned ready=0;
    for (;ready<PATCH_COUNT;++ready) {
        Patch *p=patches+ready;
        if (!VirtualProtect((void *)p->address,p->size,PAGE_EXECUTE_READWRITE,&p->old_protect)) break;
        memcpy(p->original,(void *)p->address,p->size);
    }
    if (ready!=PATCH_COUNT) {
        for(unsigned i=ready;i>0;--i) {DWORD ignored; VirtualProtect((void *)patches[i-1].address,patches[i-1].size,patches[i-1].old_protect,&ignored);}
        log_line("DISABLED: cannot change hook page protection.\r\n"); return 0;
    }
    for(unsigned i=0;i<PATCH_COUNT;++i) {
        Patch *p=patches+i; uint8_t bytes[8]; memset(bytes,0x90,p->size);
        jump_bytes(bytes,p->address,p->replacement); if(p->expected[0]==0xE8)bytes[0]=0xE8;
        memcpy((void *)p->address,bytes,p->size);
    }
    for(unsigned i=PATCH_COUNT;i>0;--i) {DWORD ignored; VirtualProtect((void *)patches[i-1].address,patches[i-1].size,patches[i-1].old_protect,&ignored);}
    FlushInstructionCache(GetCurrentProcess(),NULL,0); installed=1;
    jobs_esui_compat();
    log_line("ACTIVE: FFVII Job System 0.8.0; sixteen verified hooks installed; 25 selectable classes.\r\n");
    return 1;
}
API int jobs_hext_entry(void){
    if(!hext_resolve_api())return 0;
    if(!jobs_read_settings(NULL))return 0;
    return cmk_install();
}
