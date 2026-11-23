#ifndef MW_WEATHER_MENU_H
#define MW_WEATHER_MENU_H
/* D3D9 in-game overlay for NFSMW 2005 PC x86.
   Weather, WetGrip and radio logic are isolated from this rendering hook.
   No files, external GUI libraries or runtime font dependencies. */
typedef long WMHR;
typedef unsigned WMUINT;
typedef unsigned long WMULONG;
typedef struct WMPoint {long x, y;} WMPoint;
typedef struct WMD3DViewport {
    WMULONG x, y, width, height;
    float min_z, max_z;
} WMD3DViewport;
typedef struct WMD3DPresent {
    WMUINT back_width, back_height, back_format, back_count, multisample_type;
    WMULONG multisample_quality;
    WMUINT swap_effect;
    HWND window;
    BOOL windowed, auto_depth;
    WMUINT depth_format;
    WMULONG flags;
    WMUINT refresh_rate, interval;
} WMD3DPresent;
typedef struct WMVertex {float x,y,z,rhw; WMULONG color;} WMVertex;
typedef WMHR (STDCALL *WMEndScene)(void *);
typedef WMHR (STDCALL *WMCreateStateBlock)(void *,int,void **);
typedef WMHR (STDCALL *WMBlockOp)(void *);
typedef WMHR (STDCALL *WMDrawPrimitiveUP)(void *,unsigned,unsigned,const void *,unsigned);
typedef WMHR (STDCALL *WMSetRenderState)(void *,unsigned,WMULONG);
typedef WMHR (STDCALL *WMSetTextureStageState)(void *,unsigned,unsigned,WMULONG);
typedef WMHR (STDCALL *WMSetTexture)(void *,unsigned,void *);
typedef WMHR (STDCALL *WMSetShader)(void *,void *);
typedef WMHR (STDCALL *WMSetFVF)(void *,WMULONG);
typedef WMHR (STDCALL *WMGetViewport)(void *,WMD3DViewport *);
typedef WMHR (STDCALL *WMRelease)(void *);
typedef void *(STDCALL *WMDirect3DCreate9)(WMUINT);
typedef WMHR (STDCALL *WMCreateDevice)(void *,WMUINT,WMUINT,HWND,WMULONG,WMD3DPresent *,void **);

/* First-use Windows GDI rasterization uses installed Times New Roman.
   A Times-compatible raster fallback is embedded for GDI failures.
   No installed font file is copied or distributed. */
#include "weather_menu_font.h"
/* GDI bitmap structure; explicit 32-bit PE layout, independent of platform headers. */
typedef struct WMBmi {
    WMULONG size;
    long width,height;
    unsigned short planes, bitcount;
    WMULONG compression, size_image;
    long xppm,yppm;
    WMULONG clr_used,clr_important;
    WMULONG reserved_color;
} WMBmi;
#ifdef _MSC_VER
typedef SIZE WMTextSize;
#else
typedef struct WMTextSize {long cx,cy;} WMTextSize;
#endif
#ifdef _MSC_VER
typedef WCHAR WMFontChar;
#else
typedef unsigned short WMFontChar;
#endif
static int wm_font_init_done;

/* Build a compact anti-aliased glyph atlas in RAM using the actual Windows
   Times New Roman font, without a font file or an extra runtime library.
   Any failure leaves the included fallback raster in place. */
static void wm_init_windows_font(void)
{
    static const WMFontChar font_name[]={
        'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n',0
    };
    void *dc=0,*bitmap=0,*old_bitmap=0,*pixels=0;
    WMBmi bmi={0};
    unsigned face,glyph,row,col;
    if(wm_font_init_done)return;
    wm_font_init_done=1;
    dc=CreateCompatibleDC(0);
    if(!dc)return;
    bmi.size=40u;
    bmi.width=WM_FONT_WIDTH;
    bmi.height=-WM_FONT_HEIGHT;  /* top-down GDI DIB */
    bmi.planes=1u;bmi.bitcount=32u;
#ifdef _MSC_VER
    bitmap=CreateDIBSection(dc,(const BITMAPINFO *)&bmi,0,&pixels,0,0);
#else
    bitmap=CreateDIBSection(dc,&bmi,0,&pixels,0,0);
#endif
    if(!bitmap||!pixels){if(bitmap)DeleteObject(bitmap);DeleteDC(dc);return;}
    old_bitmap=SelectObject(dc,bitmap);
    SetBkMode(dc,1);  /* TRANSPARENT */
    SetTextColor(dc,0x00ffffffu);
    for(face=0;face<2u;++face){
        void *font=CreateFontW(-16,0,0,0,face?700:400,0,0,0,
            1,0,0,4,0,font_name);  /* DEFAULT_CHARSET, ANTIALIASED_QUALITY */
        void *old_font;
        if(!font)continue;
        old_font=SelectObject(dc,font);
        for(glyph=0;glyph<WM_FONT_GLYPHS;++glyph){
            WMFontChar letter;
            WMTextSize width={0};
            WMULONG *image_pixels=(WMULONG *)pixels;
            unsigned c;
            if(glyph<95u)letter=(WMFontChar)(32u+glyph);
            else if(glyph==95u)letter=(WMFontChar)0x0401u;
            else if(glyph==96u)letter=(WMFontChar)0x0451u;
            else letter=(WMFontChar)(0x0410u+glyph-97u);
            for(c=0;c<WM_FONT_WIDTH*WM_FONT_HEIGHT;++c)image_pixels[c]=0;
            if(!TextOutW(dc,0,0,&letter,1))continue;
            if(GetTextExtentPoint32W(dc,&letter,1,&width)) {
                long advance=width.cx;
                if(advance<2)advance=2;
                if(advance>WM_FONT_WIDTH)advance=WM_FONT_WIDTH;
                wm_font_advance[face][glyph]=(unsigned char)advance;
            }
            for(row=0;row<WM_FONT_HEIGHT;++row){
                for(col=0;col<WM_FONT_WIDTH;++col){
                    unsigned brightness=(unsigned)(image_pixels[row*WM_FONT_WIDTH+col]&0xffu);
                    unsigned coverage=(brightness*3u+110u)/255u;
                    unsigned byte_index=col/4u,shift=2u*(col%4u);
                    unsigned char *byte=&wm_font_data[face][glyph][row][byte_index];
                    if(coverage>3u)coverage=3u;
                    *byte=(unsigned char)((*byte&~(3u<<shift))|(coverage<<shift));
                }
            }
        }
        if(old_font)SelectObject(dc,old_font);
        DeleteObject(font);
    }
    if(old_bitmap)SelectObject(dc,old_bitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
}


enum {WM_PANEL_WIDTH=354,WM_PANEL_HEIGHT=575,WM_PANEL_HEADER=34};
static void **wm_d3d_vtable;
static WMEndScene wm_original_endscene;
static HWND wm_game_window;
static int wm_init_attempted,wm_visible,wm_row,wm_slider_drag=-1;
static int wm_prev_hotkey,wm_prev_keys[256];
static int wm_mouse_x,wm_mouse_y,wm_click,wm_left_down;
static int wm_window_move,wm_window_resize,wm_layout_dirty;
/* Only one in-game setting can have keyboard focus. Typing edits a temporary
   buffer; the registry and game state change only after Enter/click-away. */
static int wm_edit_index=-1,wm_edit_len,wm_edit_replace;
static char wm_edit_text[5];
static float wm_drag_offset_x,wm_drag_offset_y;
static int wm_virt_x,wm_virt_y;
static float wm_draw_scale;
static WMVertex wm_vertices[12288];
static unsigned wm_vertex_count;
static void *wm_device;
static DWORD wm_last_init_ms;
/* Windows system cursor while the in-game panel is active. We preserve the
   Win32 cursor visibility counter instead of blindly hiding it on exit. */
typedef struct WMCursorInfo {
    DWORD size, flags;
    void *handle;
    WMPoint screen_pos;
} WMCursorInfo;
enum {WM_CURSOR_SHOWING=1,WM_ARROW_RESOURCE=32512,WM_MAX_CURSOR_ADJUST=32};
static int wm_cursor_active,wm_cursor_show_adjustments;
static void *wm_system_arrow,*wm_previous_cursor;
static void wm_sync_system_cursor(int enable)
{
    if(enable){
        int result;
        WMCursorInfo ci;
        if(!wm_system_arrow)
            wm_system_arrow=LoadCursorA(0,(const char *)(Addr)WM_ARROW_RESOURCE);
        if(!wm_cursor_active){
            wm_cursor_active=1;
            wm_previous_cursor=0;
            /* The game normally keeps the Win32 cursor hidden. */
            do {
                result=ShowCursor(1);
                ++wm_cursor_show_adjustments;
            }while(result<0&&wm_cursor_show_adjustments<WM_MAX_CURSOR_ADJUST);
            if(wm_system_arrow)wm_previous_cursor=SetCursor(wm_system_arrow);
        }else{
            ci.size=sizeof ci;
            /* Some games hide the cursor again while processing input. */
            if(GetCursorInfo(&ci) && !(ci.flags&WM_CURSOR_SHOWING) &&
                wm_cursor_show_adjustments<WM_MAX_CURSOR_ADJUST){
                ++wm_cursor_show_adjustments;
                ShowCursor(1);
            }
            if(wm_system_arrow)SetCursor(wm_system_arrow);
        }
    }else if(wm_cursor_active){
        wm_cursor_active=0;
        if(wm_previous_cursor)SetCursor(wm_previous_cursor);
        else SetCursor(0);
        while(wm_cursor_show_adjustments>0){
            ShowCursor(0);
            --wm_cursor_show_adjustments;
        }
    }
}


static void **wm_vtable(void *object) {return *(void ***)object;}
#define WM_METHOD(object,index,type) ((type)wm_vtable(object)[index])

static int wm_key_edge(int key)
{
    int down=((unsigned short)GetAsyncKeyState(key)&0x8000u)!=0;
    int edge=down&&!wm_prev_keys[key];
    wm_prev_keys[key]=down;
    return edge;
}
static int wm_focus(void)
{
    DWORD pid=0;
    HWND hwnd=GetForegroundWindow();
    return hwnd && GetWindowThreadProcessId(hwnd,&pid) && pid==GetCurrentProcessId();
}
static float wm_clamp(float n,float a,float b)
{
    return n<a?a:n>b?b:n;
}
static int wm_hover(int x,int y,int w,int h)
{
    return wm_mouse_x>=x && wm_mouse_x<x+w &&
        wm_mouse_y>=y && wm_mouse_y<y+h;
}
static void wm_flush(void)
{
    if(!wm_vertex_count||!wm_device)return;
    WM_METHOD(wm_device,83,WMDrawPrimitiveUP)(wm_device,4,wm_vertex_count/3,
        wm_vertices,sizeof(WMVertex));
    wm_vertex_count=0;
}
static void wm_rect(float x,float y,float w,float h,WMULONG color)
{
    WMVertex v[6];
    float x0=(float)wm_virt_x+x*wm_draw_scale-0.5f;
    float y0=(float)wm_virt_y+y*wm_draw_scale-0.5f;
    float x1=x0+w*wm_draw_scale,y1=y0+h*wm_draw_scale;
    unsigned i;
    if(w<=0||h<=0)return;
    if(wm_vertex_count+6u>sizeof wm_vertices/sizeof wm_vertices[0])wm_flush();
    v[0].x=x0;v[0].y=y0;
    v[1].x=x1;v[1].y=y0;
    v[2].x=x0;v[2].y=y1;
    v[3].x=x1;v[3].y=y0;
    v[4].x=x1;v[4].y=y1;
    v[5].x=x0;v[5].y=y1;
    for(i=0;i<6;++i){
        v[i].z=0.0f;v[i].rhw=1.0f;v[i].color=color;
        wm_vertices[wm_vertex_count++]=v[i];
    }
}
/* UTF-8 decoder. Invalid sequences are replaced with a question mark. */
static unsigned wm_utf8_next(const unsigned char **stream)
{
    const unsigned char *p=*stream;
    unsigned cp;
    if(!*p)return 0;
    if(*p<0x80u){*stream=p+1;return *p;}
    if((*p&0xe0u)==0xc0u && (p[1]&0xc0u)==0x80u){
        cp=((unsigned)(p[0]&31u)<<6)|(unsigned)(p[1]&63u);
        *stream=p+2;return cp;
    }
    if((*p&0xf0u)==0xe0u && p[1] && p[2] &&
       (p[1]&0xc0u)==0x80u && (p[2]&0xc0u)==0x80u){
        cp=((unsigned)(p[0]&15u)<<12)|((unsigned)(p[1]&63u)<<6)|(unsigned)(p[2]&63u);
        *stream=p+3;return cp;
    }
    *stream=p+1;return '?';
}
static unsigned wm_glyph_index(unsigned cp)
{
    if(cp>=32 && cp<=126)return cp-32;
    if(cp==0x401)return 95;
    if(cp==0x451)return 96;
    if(cp>=0x410 && cp<=0x44f)return cp-0x410+97;
    return '?' -32;
}
static float wm_text_width(const char *s,int bold)
{
    const unsigned char *p=(const unsigned char *)s;
    unsigned cp;
    float w=0;
    while((cp=wm_utf8_next(&p))!=0)
        w+=(float)wm_font_advance[bold?1:0][wm_glyph_index(cp)];
    return w;
}
static WMULONG wm_fade_alpha(WMULONG color,unsigned opacity)
{
    unsigned base=(unsigned)(color>>24);
    return (color&0xffffffu)|((WMULONG)(base*opacity/3u)<<24);
}
static void wm_text(float x,float y,const char *s,int bold,WMULONG color)
{
    const unsigned char *p=(const unsigned char *)s;
    unsigned cp;
    const unsigned face=bold?1u:0u;
    while((cp=wm_utf8_next(&p))!=0){
        unsigned glyph=wm_glyph_index(cp),row;
        for(row=0;row<WM_FONT_HEIGHT;++row){
            unsigned col=0;
            while(col<WM_FONT_WIDTH){
                unsigned start,coverage;
                const unsigned char *bitmap=wm_font_data[face][glyph][row];
                coverage=(bitmap[col/4]>>(2*(col%4)))&3u;
                if(!coverage){++col;continue;}
                start=col++;
                while(col<WM_FONT_WIDTH &&
                    ((bitmap[col/4]>>(2*(col%4)))&3u)==coverage)++col;
                wm_rect(x+(float)start,y+(float)row,(float)(col-start),1.0f,
                    wm_fade_alpha(color,coverage));
            }
        }
        x+=(float)wm_font_advance[face][glyph];
    }
}
static void wm_center_text(float x,float y,float w,const char *s,int bold,WMULONG color)
{
    float tw=wm_text_width(s,bold);
    wm_text(x+(w-tw)*0.5f,y,s,bold,color);
}
static int wm_button(int x,int y,int w,int h,const char *label,int selected)
{
    int hover=wm_hover(x,y,w,h);
    int click=hover&&wm_click;
    WMULONG fill=selected?0xff236e96u:hover?0xff416d8au:0xe02b3c4cu;
    wm_rect((float)x,(float)y,(float)w,(float)h,fill);
    wm_rect((float)x,(float)y,(float)w,1.0f,selected?0xff8be8ffu:0xff658aa1u);
    wm_center_text((float)x,(float)(y+(h-WM_FONT_HEIGHT)/2), (float)w,label,1,0xffedf7ffu);
    return click;
}
static const char *const wm_labels[2][WM_SETTING_COUNT]={
    {
        "Minimum rain duration, min", "Maximum rain duration, min",
        "Minimum dry interval, min", "Maximum dry interval, min",
        "Overall wet-road grip, %", "Braking / acceleration grip, %",
        "Rear axle cornering grip, %", "Time to lose grip, sec",
        "Time to regain grip, sec"
    },{
        "Мин. длительность дождя, мин", "Макс. длительность дождя, мин",
        "Мин. сухой период, мин", "Макс. сухой период, мин",
        "Сцепление с мокрой дорогой, %", "Разгон и торможение, %",
        "Сцепление задней оси, %", "Время намокания, сек",
        "Время высыхания, сек"
    }
};
static const char *wm_translate(const char *en,const char *ru)
{
    return wm_language_ru?ru:en;
}
static int wm_minimum(int i){return i>=4 && i<=6?50:1;}
static int wm_maximum(int i)
{
    if(i<=3)return 60; /* Rain and dry breaks: 1–60 minutes */
    if(i>=4 && i<=6)return 100;
    return 60;        /* Soaking and drying: 1–60 seconds */
}
static void wm_set_setting(int index,int value)
{
    int previous;
    if(index<0||index>=WM_SETTING_COUNT)return;
    if(value<wm_minimum(index))value=wm_minimum(index);
    if(value>wm_maximum(index))value=wm_maximum(index);
    previous=(int)menu_values[index];
    if(previous==value)return;
    menu_values[index]=(RainU32)value;
    if(index==0&&menu_values[1]<menu_values[0])menu_values[1]=menu_values[0];
    if(index==1&&menu_values[0]>menu_values[1])menu_values[0]=menu_values[1];
    if(index==2&&menu_values[3]<menu_values[2])menu_values[3]=menu_values[2];
    if(index==3&&menu_values[2]>menu_values[3])menu_values[2]=menu_values[3];
    ++menu_revision;
    save_menu_settings();
}
static void wm_change_setting(int index,int direction)
{
    /* Keyboard arrows and +/- always change exactly one unit, even with Shift. */
    if(index<0||index>=WM_SETTING_COUNT||!direction)return;
    wm_set_setting(index,(int)menu_values[index]+(direction>0?1:-1));
}
static void wm_number(char *out,RainU32 n)
{
    char digits[16];
    unsigned len=0,j=0;
    do{digits[len++]=(char)('0'+n%10u);n/=10u;}while(n && len<sizeof digits);
    while(len)out[j++]=digits[--len];
    out[j]=0;
}
/* Numeric field: 0-9 and numeric keypad, Backspace/Delete, Enter and Esc.
   Clicking the current value starts replacement typing. Click-away commits. */
static void wm_edit_cancel(void)
{
    wm_edit_index=-1;
    wm_edit_len=0;
    wm_edit_replace=0;
    wm_edit_text[0]=0;
}
static void wm_edit_commit(void)
{
    int i,n=0;
    const int idx=wm_edit_index;
    if(idx>=0 && wm_edit_len>0){
        for(i=0;i<wm_edit_len;++i)n=n*10+(wm_edit_text[i]-'0');
        wm_set_setting(idx,n);  /* Clamp and maintain min <= max pairs. */
    }
    wm_edit_cancel();
}
static void wm_edit_begin(int idx)
{
    wm_edit_index=idx;
    wm_number(wm_edit_text,menu_values[idx]);
    wm_edit_len=0;
    while(wm_edit_text[wm_edit_len] && wm_edit_len<3)++wm_edit_len;
    wm_edit_replace=1;  /* First typed digit replaces the old number. */
    wm_row=idx;
}
static void wm_edit_digit(int digit)
{
    if(wm_edit_index<0 || digit<0 || digit>9)return;
    if(wm_edit_replace){wm_edit_len=0;wm_edit_replace=0;}
    if(wm_edit_len>=3)return;  /* All limits are at most 100. */
    wm_edit_text[wm_edit_len++]=(char)('0'+digit);
    wm_edit_text[wm_edit_len]=0;
}
static void wm_edit_backspace(void)
{
    if(wm_edit_index<0)return;
    if(wm_edit_replace){wm_edit_len=0;wm_edit_replace=0;}
    else if(wm_edit_len>0)--wm_edit_len;
    wm_edit_text[wm_edit_len]=0;
}
static int wm_number_field_hover(int index)
{
    int y=index<4?110+index*36:332+(index-4)*36;
    return wm_hover(245,y+17,54,19);
}
static void wm_setting_row(int index,int y)
{
    const int slider_x=16,slider_w=190,slider_y=y+27;
    const int low=wm_minimum(index),high=wm_maximum(index);
    const int value=(int)menu_values[index];
    int active=wm_hover(10,y,334,36),selected=wm_row==index;
    int fill=(int)(((unsigned)(value-low)*slider_w)/(high-low));
    char number[16];
    const char *label=wm_labels[wm_language_ru?1:0][index];
    if(active&&wm_click)wm_row=index;
    wm_rect(10,(float)y,334,35,selected?0x9b254357u:active?0x9831475au:0x761a2632u);
    wm_text(16,(float)y+1,label,0,0xffe3eef7u);
    wm_rect((float)slider_x,(float)slider_y,(float)slider_w,5,0xff2a3540u);
    if(fill)wm_rect((float)slider_x,(float)slider_y,(float)fill,5,0xff3496c4u);
    wm_rect((float)(slider_x+fill-2),(float)slider_y-2,5,9,0xffa6e8ffu);
    if(wm_click&&wm_hover(slider_x,slider_y-3,slider_w,11)){
        wm_slider_drag=index;wm_row=index;
    }
    if(wm_slider_drag==index&&wm_left_down){
        int pos=wm_mouse_x-slider_x;
        if(pos<0)pos=0;
        if(pos>slider_w)pos=slider_w;
        wm_set_setting(index,low+(int)(((unsigned)pos*(high-low)+slider_w/2)/slider_w));
    }
    if(wm_button(215,y+17,25,19,"-",0)){
        wm_row=index;wm_slider_drag=-1;wm_change_setting(index,-1);
    }
    {
        int editing=wm_edit_index==index;
        int hover=wm_number_field_hover(index);
        WMULONG field=editing?0xff275574u:hover?0xff3c5262u:0xdd172a37u;
        wm_rect(245,(float)y+17,54,19,field);
        wm_rect(245,(float)y+17,54,1,editing?0xffa7e9ffu:0xff67869bu);
        if(wm_click&&hover&&wm_edit_index!=index){
            wm_edit_begin(index);editing=1;
        }
        if(editing){
            /* The selection highlight means the next digit replaces the value. */
            if(wm_edit_replace)wm_rect(248,(float)y+19,48,15,0xff3478a0u);
            wm_center_text(245,(float)y+18,54,wm_edit_len?wm_edit_text:"_",1,0xffffffffu);
        }else{
            wm_number(number,menu_values[index]);
            wm_center_text(245,(float)y+18,54,number,1,0xffe5f5ffu);
        }
    }
    if(wm_button(305,y+17,28,19,"+",0)){
        wm_row=index;wm_slider_drag=-1;wm_change_setting(index,1);
    }
}
static void wm_handle_mouse(const WMD3DViewport *vp,float base_scale,float ui_zoom)
{
    WMPoint cursor;
    float x=-999.0f,y=-999.0f;
    float viewport_w=(float)vp->width/base_scale;
    float viewport_h=(float)vp->height/base_scale;
    float height=wm_window_collapsed?WM_PANEL_HEADER:WM_PANEL_HEIGHT;
    float max_x=viewport_w-(float)WM_PANEL_WIDTH*ui_zoom;
    float max_y=viewport_h-height*ui_zoom;
    if(max_x<0)max_x=0;
    if(max_y<0)max_y=0;
    wm_window_x=(DWORD)wm_clamp((float)wm_window_x,0,max_x);
    wm_window_y=(DWORD)wm_clamp((float)wm_window_y,0,max_y);
    if(GetCursorPos(&cursor)&&ScreenToClient(wm_game_window,&cursor)){
        x=((float)cursor.x-(float)vp->x)/base_scale;
        y=((float)cursor.y-(float)vp->y)/base_scale;
    }
    wm_mouse_x=(int)((x-(float)wm_window_x)/ui_zoom);
    wm_mouse_y=(int)((y-(float)wm_window_y)/ui_zoom);
    wm_click=wm_key_edge(0x01);
    wm_left_down=((unsigned short)GetAsyncKeyState(0x01)&0x8000u)!=0;
    if(wm_click){
        if(wm_hover(8,2,214,WM_PANEL_HEADER-3)){
            wm_window_move=1;wm_drag_offset_x=x-(float)wm_window_x;
            wm_drag_offset_y=y-(float)wm_window_y;
        }else if(!wm_window_collapsed &&
            wm_hover(WM_PANEL_WIDTH-21,WM_PANEL_HEIGHT-21,21,21)){
            wm_window_resize=1;
        }
    }
    if(wm_left_down&&wm_window_move){
        DWORD nx=(DWORD)wm_clamp(x-wm_drag_offset_x,0,max_x);
        DWORD ny=(DWORD)wm_clamp(y-wm_drag_offset_y,0,max_y);
        if(nx!=wm_window_x||ny!=wm_window_y)wm_layout_dirty=1;
        wm_window_x=nx;wm_window_y=ny;
    }
    if(wm_left_down&&wm_window_resize){
        float wish=(x-(float)wm_window_x)/(float)WM_PANEL_WIDTH;
        float max_zoom=wm_clamp((viewport_h-(float)wm_window_y-4.0f)/WM_PANEL_HEIGHT,
            0.65f,1.55f);
        unsigned pct=(unsigned)(wm_clamp(wish,0.65f,max_zoom)*100.0f+0.5f);
        if(pct!=wm_window_scale_pct){wm_window_scale_pct=pct;wm_layout_dirty=1;}
    }
    if(!wm_left_down){
        wm_window_move=0;wm_window_resize=0;wm_slider_drag=-1;
        if(wm_layout_dirty){wm_layout_dirty=0;save_menu_settings();}
    }
}
static void wm_render(void *device)
{
    WMD3DViewport vp;
    void *block=0;
    float base_scale,ui_zoom,viewport_height;
    int keyboard_up,keyboard_down,keyboard_left,keyboard_right,keyboard_enter;
    int keyboard_escape;
    int rain_state=(int)menu_rain_state;
    int i;
    const char *status;
    if(WM_METHOD(device,48,WMGetViewport)(device,&vp)<0 || !vp.width || !vp.height)return;
    wm_init_windows_font();
    base_scale=(vp.width/800.0f<vp.height/600.0f)?vp.width/800.0f:vp.height/600.0f;
    if(base_scale<0.30f)return;
    ui_zoom=(float)wm_window_scale_pct/100.0f;
    viewport_height=(float)vp.height/base_scale;
    if(!wm_window_collapsed && (float)WM_PANEL_HEIGHT*ui_zoom>viewport_height-4.0f){
        ui_zoom=wm_clamp((viewport_height-4.0f)/(float)WM_PANEL_HEIGHT,0.65f,1.55f);
        wm_window_scale_pct=(DWORD)(ui_zoom*100.0f);
    }
    wm_handle_mouse(&vp,base_scale,ui_zoom);
    /* Clicking outside the current number field applies the new value. */
    if(wm_click && wm_edit_index>=0 && !wm_number_field_hover(wm_edit_index))
        wm_edit_commit();
    /* Handle instantly changed scale/position, including active resize. */
    ui_zoom=(float)wm_window_scale_pct/100.0f;
    wm_virt_x=(int)((float)vp.x+(float)wm_window_x*base_scale);
    wm_virt_y=(int)((float)vp.y+(float)wm_window_y*base_scale);
    wm_draw_scale=base_scale*ui_zoom;
    keyboard_up=wm_key_edge(0x26);
    keyboard_down=wm_key_edge(0x28);
    keyboard_left=wm_key_edge(0x25);
    keyboard_right=wm_key_edge(0x27);
    keyboard_enter=wm_key_edge(0x0d);
    keyboard_escape=wm_key_edge(0x1b);
    if(wm_key_edge(0x75)){wm_language_ru^=1u;save_menu_settings();} /* F6 */
    if(wm_edit_index>=0){
        unsigned digit;
        int tab=wm_key_edge(0x09);
        if(keyboard_escape)wm_edit_cancel();
        else if(keyboard_enter || tab){
            int next=wm_edit_index;
            wm_edit_commit();
            if(tab)wm_edit_begin((next+1)%WM_SETTING_COUNT);
        }else{
            if(wm_key_edge(0x08)||wm_key_edge(0x2e))wm_edit_backspace();
            for(digit=0;digit<10u;++digit){
                if(wm_key_edge(0x30+(int)digit)||wm_key_edge(0x60+(int)digit))
                    wm_edit_digit((int)digit);
            }
        }
    }else if(keyboard_escape){wm_visible=0;return;}
    if(!wm_window_collapsed && wm_edit_index<0){
        if(wm_key_edge(0x09))wm_row=wm_row<4?4:0;
        if(keyboard_up)wm_row=(wm_row+11)%12;
        if(keyboard_down)wm_row=(wm_row+1)%12;
        if(wm_row>=9){
            if(keyboard_left)wm_row=wm_row==9?11:wm_row-1;
            if(keyboard_right)wm_row=wm_row==11?9:wm_row+1;
            if(keyboard_enter)menu_command=wm_row-8;
        }else{
            if(keyboard_left)wm_change_setting(wm_row,-1);
            if(keyboard_right||keyboard_enter)wm_change_setting(wm_row,1);
        }
    }
    if(WM_METHOD(device,59,WMCreateStateBlock)(device,1,&block)<0||!block)return;
    WM_METHOD(block,4,WMBlockOp)(block);
    wm_device=device;wm_vertex_count=0;
    WM_METHOD(device,92,WMSetShader)(device,0);
    WM_METHOD(device,107,WMSetShader)(device,0);
    WM_METHOD(device,65,WMSetTexture)(device,0,0);
    WM_METHOD(device,57,WMSetRenderState)(device,7,0);
    WM_METHOD(device,57,WMSetRenderState)(device,27,1);
    WM_METHOD(device,57,WMSetRenderState)(device,19,5);
    WM_METHOD(device,57,WMSetRenderState)(device,20,6);
    WM_METHOD(device,57,WMSetRenderState)(device,22,1);
    WM_METHOD(device,57,WMSetRenderState)(device,28,0);
    WM_METHOD(device,57,WMSetRenderState)(device,137,0);
    WM_METHOD(device,57,WMSetRenderState)(device,174,0);
    WM_METHOD(device,57,WMSetRenderState)(device,168,15);
    WM_METHOD(device,67,WMSetTextureStageState)(device,0,1,2);
    WM_METHOD(device,67,WMSetTextureStageState)(device,0,2,0);
    WM_METHOD(device,67,WMSetTextureStageState)(device,0,4,2);
    WM_METHOD(device,67,WMSetTextureStageState)(device,0,5,0);
    WM_METHOD(device,67,WMSetTextureStageState)(device,1,1,1);
    WM_METHOD(device,89,WMSetFVF)(device,0x44u);

    wm_rect(0,2,WM_PANEL_WIDTH,wm_window_collapsed?WM_PANEL_HEADER:WM_PANEL_HEIGHT,0x5a000000u);
    wm_rect(0,0,WM_PANEL_WIDTH,wm_window_collapsed?WM_PANEL_HEADER:WM_PANEL_HEIGHT,0xe5202934u);
    wm_rect(0,0,WM_PANEL_WIDTH,2,0xff55afd8u);
    wm_rect(0,2,WM_PANEL_WIDTH,32,0xe831546cu);
    wm_text(13,9,"MW WEATHER CONTROL",1,0xfff4f9feu);
    if(wm_button(234,7,49,24,wm_language_ru?"RU":"EN",1)){
        wm_language_ru^=1u;save_menu_settings();
    }
    if(wm_button(290,7,25,24,wm_window_collapsed?"+":"-",0)){
        wm_window_collapsed^=1u;save_menu_settings();
    }
    if(wm_button(322,7,25,24,"X",0)){wm_visible=0;wm_slider_drag=-1;}
    if(!wm_window_collapsed){
        wm_text(12,39,wm_translate("Click a value to type it manually",
            "Нажмите на число для ручного ввода"),0,0xffb4c9d8u);
        wm_rect(10,63,334,20,0x9d275776u);
        wm_text(17,64,wm_translate("RAIN / DURATION","ДОЖДЬ / ДЛИТЕЛЬНОСТЬ"),1,0xffc8edffu);
        status=rain_state==2?wm_translate("RAIN FORCED ON","ДОЖДЬ ВКЛЮЧЁН"):
            rain_state==3?wm_translate("RAIN FORCED OFF","ДОЖДЬ ВЫКЛЮЧЕН"):
            rain_state==1?wm_translate("AUTO: RAINING","АВТО: ДОЖДЬ"):
            wm_translate("AUTO: DRY","АВТО: СУХО");
        wm_text(17,87,wm_translate("Current mode:","Режим:"),0,0xffb9ccdbu);
        wm_text(wm_language_ru?88:120,87,status,1,0xfff6fafdu);
        for(i=0;i<4;++i)wm_setting_row(i,110+i*36);
        if(wm_button(11,258,105,29,wm_translate("RAIN ON","ВКЛЮЧИТЬ"),rain_state==2||wm_row==9)){
            wm_row=9;menu_command=1;
        }
        if(wm_button(123,258,116,29,wm_translate("RAIN OFF","ВЫКЛЮЧИТЬ"),rain_state==3||wm_row==10)){
            wm_row=10;menu_command=2;
        }
        if(wm_button(246,258,97,29,wm_translate("AUTO","АВТО"),rain_state<2||wm_row==11)){
            wm_row=11;menu_command=3;
        }
        wm_rect(10,299,334,1,0xff446173u);
        wm_rect(10,306,334,20,0x9d275776u);
        wm_text(17,307,wm_translate("WET ROAD / TIRE GRIP",
            "МОКРАЯ ДОРОГА / СЦЕПЛЕНИЕ"),1,0xffc8edffu);
        for(i=4;i<9;++i)wm_setting_row(i,332+(i-4)*36);
        wm_rect(10,520,334,1,0xff446173u);
        wm_text(14,525,wm_translate("-/+: step 1  |  Enter: save  |  Esc: cancel",
            "-/+: шаг 1 | Enter: ввод | Esc: отмена"),0,0xffafc1cdu);
        wm_text(14,548,wm_translate("Drag header: move  |  Corner: resize",
            "Заголовок: переместить | Угол: размер"),0,0xffafc1cdu);
        wm_rect(WM_PANEL_WIDTH-21,WM_PANEL_HEIGHT-21,20,20,0x9b345c76u);
        wm_rect(WM_PANEL_WIDTH-15,WM_PANEL_HEIGHT-7,9,2,0xffbdeeffu);
        wm_rect(WM_PANEL_WIDTH-9,WM_PANEL_HEIGHT-13,2,8,0xffbdeeffu);
    }
    /* Use the genuine Windows arrow cursor (shown by wm_sync_system_cursor),
       not the previous hand-drawn two-rectangle pointer. */
    wm_flush();
    WM_METHOD(block,5,WMBlockOp)(block);
    WM_METHOD(block,2,WMRelease)(block);
    wm_device=0;
}
static WMHR STDCALL wm_endscene(void *device)
{
    int focus,chord;
    if(!wm_original_endscene)return 0;
    focus=wm_focus();
    chord=focus&&(((unsigned short)GetAsyncKeyState(0x11)&0x8000u)!=0)&&
        (((unsigned short)GetAsyncKeyState(0x10)&0x8000u)!=0)&&
        (((unsigned short)GetAsyncKeyState(0x71)&0x8000u)!=0);
    if(chord&&!wm_prev_hotkey){
        wm_game_window=GetForegroundWindow();
        wm_visible=!wm_visible;
        if(!wm_visible && wm_edit_index>=0)wm_edit_commit();
        wm_prev_keys[0x1b]=0;wm_prev_keys[0x01]=0;
    }
    wm_prev_hotkey=chord;
    wm_sync_system_cursor(wm_visible&&focus);
    if(wm_visible&&focus)wm_render(device);
    /* ESC, the close button or a focus change may have hidden the panel. */
    wm_sync_system_cursor(wm_visible&&focus);
    return wm_original_endscene(device);
}

static int weather_menu_init(void)
{
    HINSTANCE d3d;
    WMDirect3DCreate9 make_d3d;
    WMCreateDevice make_device;
    WMD3DPresent pp={0};
    void *api=0, *dummy=0;
    HWND dummy_window;
    DWORD old,unused;
    void **table;
    if(wm_original_endscene) return 1;
    if(wm_init_attempted>=12) return 0;
    if(!wm_focus()) return 0;
    if(wm_init_attempted && (RainU32)(GetTickCount()-wm_last_init_ms)<1000u) return 0;
    wm_last_init_ms=GetTickCount();
    ++wm_init_attempted;
    wm_game_window=GetForegroundWindow();
    d3d=GetModuleHandleA("d3d9.dll");
    if(!d3d) return 0;
    make_d3d=(WMDirect3DCreate9)GetProcAddress(d3d,"Direct3DCreate9");
    if(!make_d3d) return 0;
    api=make_d3d(32);
    if(!api) return 0;
    dummy_window=CreateWindowExA(0,"STATIC","MW Weather Dummy",0x80000000u,
        0,0,1,1,0,0,0,0);
    if(!dummy_window) {WM_METHOD(api,2,WMRelease)(api);return 0;}
    pp.back_width=1;
    pp.back_height=1;
    pp.back_count=1;
    pp.swap_effect=1;
    pp.window=dummy_window;
    pp.windowed=1;
    make_device=WM_METHOD(api,16,WMCreateDevice);
    if(make_device(api,0,1,dummy_window,0x20u,&pp,&dummy)<0 || !dummy) {
        DestroyWindow(dummy_window);
        WM_METHOD(api,2,WMRelease)(api);
        return 0;
    }
    table=wm_vtable(dummy);
    if(table && table[42] && VirtualProtect(&table[42],sizeof(void *),0x40u,&old)) {
        wm_original_endscene=(WMEndScene)table[42];
        wm_d3d_vtable=table;
        table[42]=(void *)wm_endscene;
        VirtualProtect(&table[42],sizeof(void *),old,&unused);
    }
    WM_METHOD(dummy,2,WMRelease)(dummy);
    DestroyWindow(dummy_window);
    WM_METHOD(api,2,WMRelease)(api);
    if(wm_original_endscene)
        radio_log("menu-d3d9-hook-installed",(RainU32)(Addr)wm_d3d_vtable,42,1);
    return wm_original_endscene!=0;
}
#endif
