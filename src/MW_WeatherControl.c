/* Need for Speed: Most Wanted (2005), Windows x86. */
#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define STDCALL WINAPI
#define FASTCALL __fastcall
#else
#define STDCALL __attribute__((stdcall))
#define FASTCALL __attribute__((fastcall))
typedef void *HANDLE;
typedef void *HINSTANCE;
typedef void *HWND;
typedef unsigned long DWORD;
typedef int BOOL;
extern HINSTANCE STDCALL GetModuleHandleA(const char *);
extern BOOL STDCALL GetModuleHandleExA(DWORD, const char *, HINSTANCE *);
extern BOOL STDCALL DisableThreadLibraryCalls(HINSTANCE);
extern DWORD STDCALL GetModuleFileNameA(HINSTANCE, char *, DWORD);
extern BOOL STDCALL VirtualProtect(void *, unsigned long, DWORD, DWORD *);
extern HANDLE STDCALL GetCurrentProcess(void);
extern DWORD STDCALL GetCurrentProcessId(void);
extern DWORD STDCALL GetTickCount(void);
extern HANDLE STDCALL CreateFileA(const char *, DWORD, DWORD, void *, DWORD, DWORD, HANDLE);
extern BOOL STDCALL WriteFile(HANDLE, const void *, DWORD, DWORD *, void *);
extern BOOL STDCALL CloseHandle(HANDLE);
extern BOOL STDCALL FlushInstructionCache(HANDLE, const void *, unsigned long);
extern short STDCALL GetAsyncKeyState(int);
extern void * STDCALL GetProcAddress(HINSTANCE, const char *);
extern HWND STDCALL CreateWindowExA(DWORD,const char *,const char *,DWORD,int,int,int,int,HWND,void *,HINSTANCE,void *);
extern BOOL STDCALL DestroyWindow(HWND);
extern BOOL STDCALL GetCursorPos(void *);
extern BOOL STDCALL ScreenToClient(HWND,void *);
extern int STDCALL ShowCursor(BOOL);
extern void *STDCALL LoadCursorA(HINSTANCE,const char *);
extern void *STDCALL SetCursor(void *);
extern BOOL STDCALL GetCursorInfo(void *);
/* Windows GDI: resolve the user's installed Times New Roman at runtime.
   Font files are not shipped or copied into the ASI. */
extern void *STDCALL CreateCompatibleDC(void *);
extern int STDCALL DeleteDC(void *);
extern void *STDCALL CreateDIBSection(void *, const void *, unsigned, void **,void *, DWORD);
extern void *STDCALL SelectObject(void *,void *);
extern int STDCALL DeleteObject(void *);
extern void *STDCALL CreateFontW(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,const unsigned short *);
extern DWORD STDCALL SetTextColor(void *,DWORD);
extern int STDCALL SetBkMode(void *,int);
extern BOOL STDCALL TextOutW(void *,int,int,const unsigned short *,int);
extern BOOL STDCALL GetTextExtentPoint32W(void *,const unsigned short *,int,void *);
extern long STDCALL RegOpenKeyExA(void *,const char *,DWORD,DWORD,void **);
extern long STDCALL RegCreateKeyExA(void *,const char *,DWORD,char *,DWORD,DWORD,void *,void **,DWORD *);
extern long STDCALL RegQueryValueExA(void *,const char *,DWORD *,DWORD *,unsigned char *,DWORD *);
extern long STDCALL RegSetValueExA(void *,const char *,DWORD,DWORD,const unsigned char *,DWORD);
extern long STDCALL RegCloseKey(void *);
extern HWND STDCALL GetForegroundWindow(void);
extern DWORD STDCALL GetWindowThreadProcessId(HWND, DWORD *);
#endif

#include "rain_controller.h"
#include "weather_radio.h"
#include "wet_grip.h"
#include "wet_ai.h"

typedef unsigned char U8;
typedef unsigned long Addr;
typedef char Win32Required[sizeof(void *)==4 && sizeof(float)==4 && sizeof(RainU32)==4 ? 1 : -1];
#define AT(type, address) ((type)(Addr)(address))

enum {
    GAME_BASE=0x00400000,
    GAME_IMAGE_SIZE=0x00678e4e,
    RAIN_UPDATE_CALL=0x00758351,
    NATIVE_TARGET=0x007539d0,
    GAME_CLOCK=0x009885d8,
    SCRIPT_RAIN_OVERRIDE=0x009b0a48,
    WEATHER_VIEW=0x009196b8,
    ACTUAL_RAIN=0x28c,
    RENDER_MODE=0x00982b3c,
    TIRE_GRIP_DRIVE=0x0069dd22,
    TIRE_GRIP_LIMIT=0x0069dfbe,
    TIRE_LATERAL_FORCE=0x0069e09e,
    TIRE_LONG_DEMAND=0x0069df7f,
    TIRE_LONG_FORCE=0x0069e0b0,
    /* v1.3 AIVehicleCopCar/Racecar primary vtables: GetDriveSpeed
       followed immediately by SetDriveSpeed.  Do not touch Human/Traffic. */
    AI_COP_GET_DRIVE_SPEED_SLOT=0x008924b4,
    AI_RACER_GET_DRIVE_SPEED_SLOT=0x00892674,
    AI_NATIVE_GET_DRIVE_SPEED=0x00431c70,
    AI_NATIVE_SET_DRIVE_SPEED=0x00431c80,
    PLAYER_PURSUIT=0x00993cc8,
    RADIO_READY=0x008f8700,
    RADIO_TICK_CALL=0x0072162c,
    NATIVE_RADIO_TICK=0x00701090,
    RADIO_EVENT_TABLE=0x009933f0,
    RADIO_KEY_HASH=0x004549b0,
    RADIO_KEY_TO_ID=0x004ba9b0,
    RADIO_IS_PENDING=0x00709d00,
    RADIO_IS_ACTIVE=0x00709c80,
    RADIO_IS_CURRENT=0x006ff320,
    RADIO_CHANNEL_BUSY=0x006ff2f0,
    RADIO_ACTIVE_LIST=0x009933e8,
    WEATHER_NAME=0x008b136c,
    RADIO_UNMAPPED=0x108,
    RADIO_EVENT_POOL=0x009922dc,
    NATIVE_SPEECH_FILTER=0x0070c600,
    SPEECH_FILTER_QUEUE_CALL=0x0070cd1a,
    SPEECH_FILTER_READY_CALL=0x00711aad,
    WEATHER_REPORT=0x00719b00,
    FIND_UNIT=0x007153b0,
    AIR_SPEAKER_VTABLE=0x008b2278,
    GROUND_SPEAKER_VTABLE=0x008b1fa0,
    SPEAKER_READY_OFFSET=0x35
};

typedef float (*NativeRainTarget)(float, float);
static HINSTANCE module_handle;
/* Weather police reports stay functional but have no public settings UI.
   These match the radio defaults of Rain Duration 2.0/Weather Control 3.0. */
static int settings_loaded, radio_enabled=1, ground_fallback=0, ground_only=0, both_sides=1;
/* Diagnostic code remains in the source. Set to 1 and rebuild only when
   investigating an issue. The public 1.0 release never writes a log. */
#ifndef MW_WEATHER_ENABLE_LOGGING
#define MW_WEATHER_ENABLE_LOGGING 0
#endif
static int radio_installed, filter_sites, diag_enabled=MW_WEATHER_ENABLE_LOGGING;
static char log_path[260];
static struct WeatherRadioRain rain_radio;
static struct WeatherRadioReport reports[2];
static float dispatch_after, last_radio_time;
static RainU32 radio_rng;
static struct {
    RainU32 id, role, serial;
    Addr event, speaker;
    float requested_at;
    int tracking, state, saw_playback, filter_result;
} voice_trace;
enum { AIR_REPORT, GROUND_REPORT };
static volatile RainU32 published_rain;
static volatile int rain_requested;
static struct RainController controller;
static struct WetGrip wet_grip;
static int grip_installed, rear_lateral_installed, longitudinal_installed;
static int ai_wet_pace_installed;
static volatile RainU32 ai_racer_speed_calls, ai_cop_speed_calls;
static RainU32 ai_last_logged_racer_calls, ai_last_logged_cop_calls;
static float ai_last_log_time;
/* Live in-game menu values. Registry storage replaces the legacy INI. */
enum {
    WM_RAIN_MIN, WM_RAIN_MAX, WM_DRY_MIN, WM_DRY_MAX,
    WM_GRIP, WM_LONGITUDINAL, WM_REAR_LATERAL, WM_SOAK, WM_DRY_TIME,
    WM_SETTING_COUNT
};
static const char *const menu_keys[WM_SETTING_COUNT] = {
    "MinimumRainMinutes", "MaximumRainMinutes", "MinimumDryMinutes",
    "MaximumDryMinutes", "WetGripPercent", "WetLongitudinalPercent",
    "WetRearLateralPercent", "GripSoakSeconds", "GripDrySeconds"
};
static volatile RainU32 menu_values[WM_SETTING_COUNT] = {
    1u, 3u, 1u, 4u, 80u, 75u, 78u, 5u, 30u
};
static volatile unsigned menu_revision;
static volatile int menu_command;
static volatile int menu_rain_state;
/* Live overlay layout + language, saved under HKCU. 0=English, 1=Russian. */
static DWORD wm_window_x=14u, wm_window_y=42u, wm_window_scale_pct=100u;
static DWORD wm_window_collapsed=0u, wm_language_ru=1u;
static unsigned menu_last_revision;
static int weather_menu_init(void);
static void save_menu_settings(void);

#if !defined(_MSC_VER)
__attribute__((used))
#endif
static volatile float wet_grip_scale = 1.0f;
#if !defined(_MSC_VER)
__attribute__((used))
#endif
static volatile float wet_rear_lateral_scale = 1.0f;
#if !defined(_MSC_VER)
__attribute__((used))
#endif
static volatile float wet_longitudinal_scale = 1.0f;

/* Inline x87 site, not a C ABI boundary. Replay stock FMUL and preserve
   EFLAGS, GPRs and the depth of the x87 stack. No second FMUL when dry. */
#if defined(_MSC_VER)
static void __declspec(naked) tire_grip_hook(void)
{
    __asm {
        fmul dword ptr [esi+0ECh]
        pushfd
        cmp dword ptr [wet_grip_scale], 03F800000h
        je done
        fmul dword ptr [wet_grip_scale]
done:
        popfd
        ret
    }
}
/* wheel+0x10C is wheel index >> 1. Axle 1 is rear. Wheel+0xD8 is
   the lateral component returned by this solver; +0xDC is longitudinal. */
static void __declspec(naked) rear_lateral_hook(void)
{
    __asm {
        fmul dword ptr [esi+0D8h]
        pushfd
        cmp dword ptr [wet_rear_lateral_scale], 03F800000h
        je done
        cmp dword ptr [esi+010Ch], 1
        jne done
        fmul dword ptr [wet_rear_lateral_scale]
done:
        popfd
        ret
    }
}

static void __declspec(naked) longitudinal_demand_hook(void)
{
    __asm {
        fmul dword ptr [esi+0DCh]
        pushfd
        cmp dword ptr [wet_longitudinal_scale], 03F800000h
        je done
        fdiv dword ptr [wet_longitudinal_scale]
done:
        popfd
        ret
    }
}

static void __declspec(naked) longitudinal_force_hook(void)
{
    __asm {
        fmul dword ptr [esi+0DCh]
        pushfd
        cmp dword ptr [wet_longitudinal_scale], 03F800000h
        je done
        fmul dword ptr [wet_longitudinal_scale]
done:
        popfd
        ret
    }
}

#else
#define GRIP_STR2(s) #s
#define GRIP_STR(s) GRIP_STR2(s)
#define GRIP_ASM_PREFIX GRIP_STR(__USER_LABEL_PREFIX__)
static void __attribute__((naked)) tire_grip_hook(void)
{
    __asm__ volatile (
        "fmuls 0xec(%esi)\n\t"
        "pushfl\n\t"
        "cmpl $0x3f800000, " GRIP_ASM_PREFIX "wet_grip_scale\n\t"
        "je 1f\n\t"
        "fmuls " GRIP_ASM_PREFIX "wet_grip_scale\n"
        "1: popfl\n\t"
        "ret");
}
static void __attribute__((naked)) rear_lateral_hook(void)
{
    __asm__ volatile (
        "fmuls 0xd8(%esi)\n\t"
        "pushfl\n\t"
        "cmpl $0x3f800000, " GRIP_ASM_PREFIX "wet_rear_lateral_scale\n\t"
        "je 1f\n\t"
        "cmpl $1, 0x10c(%esi)\n\t"
        "jne 1f\n\t"
        "fmuls " GRIP_ASM_PREFIX "wet_rear_lateral_scale\n"
        "1: popfl\n\t"
        "ret");
}
static void __attribute__((naked)) longitudinal_demand_hook(void)
{
    __asm__ volatile (
        "fmuls 0xdc(%esi)\n\t"
        "pushfl\n\t"
        "cmpl $0x3f800000, " GRIP_ASM_PREFIX "wet_longitudinal_scale\n\t"
        "je 1f\n\t"
        "fdivs " GRIP_ASM_PREFIX "wet_longitudinal_scale\n"
        "1: popfl\n\t"
        "ret");
}

static void __attribute__((naked)) longitudinal_force_hook(void)
{
    __asm__ volatile (
        "fmuls 0xdc(%esi)\n\t"
        "pushfl\n\t"
        "cmpl $0x3f800000, " GRIP_ASM_PREFIX "wet_longitudinal_scale\n\t"
        "je 1f\n\t"
        "fmuls " GRIP_ASM_PREFIX "wet_longitudinal_scale\n"
        "1: popfl\n\t"
        "ret");
}

#endif

/* AI speed readers are shared with other AIVehicle subclasses, so the
   physical function at 0x431c70 must NOT be inline-hooked.  We replace only
   the verified cop and racer class vtable entries.  This changes the speed
   readout dynamically: no repeated writes, no compounding and an immediate
   return to stock dry behavior.  Ordinary traffic and player stay untouched. */
static float FASTCALL ai_racer_wet_drive_speed(void *self, void *ignored)
{
    float original = *AT(const volatile float *, (Addr)self + 0x38u);
    float multiplier = ai_wet_pace_multiplier(wet_grip_scale, 0.60f);
    (void)ignored;
    ++ai_racer_speed_calls;
    return original * multiplier;
}

static float FASTCALL ai_cop_wet_drive_speed(void *self, void *ignored)
{
    float original = *AT(const volatile float *, (Addr)self + 0x38u);
    float multiplier = ai_wet_pace_multiplier(wet_grip_scale, 0.70f);
    (void)ignored;
    ++ai_cop_speed_calls;
    return original * multiplier;
}

/* Patch slots as a pair.  Original next entries verify the two functions
   really are GetDriveSpeed/SetDriveSpeed, not an unrelated class interface.
   On any failure roll back both pointers and disable the AI extension. */
static int install_ai_wet_pace(void)
{
    const Addr slots[2] = { AI_RACER_GET_DRIVE_SPEED_SLOT,
                            AI_COP_GET_DRIVE_SPEED_SLOT };
    const Addr hooks[2] = { (Addr)ai_racer_wet_drive_speed,
                            (Addr)ai_cop_wet_drive_speed };
    const Addr first = AI_COP_GET_DRIVE_SPEED_SLOT;
    const unsigned span = AI_RACER_GET_DRIVE_SPEED_SLOT - first + 4u;
    DWORD old, unused;
    unsigned i;
    int ok = 1;
    for (i = 0; i < 2; ++i) {
        if (*AT(const volatile Addr *, slots[i]) != AI_NATIVE_GET_DRIVE_SPEED ||
            *AT(const volatile Addr *, slots[i] + 4u) != AI_NATIVE_SET_DRIVE_SPEED)
            return 0;
    }
    /* Both slots are on one .rdata page. A single protection change also
       avoids accidentally leaving the shared page writable on return. */
    if (!VirtualProtect(AT(void *, first), span, 0x40u, &old))
        return 0;
    for (i = 0; i < 2; ++i)
        *AT(volatile Addr *, slots[i]) = hooks[i];
    for (i = 0; i < 2; ++i)
        if (*AT(const volatile Addr *, slots[i]) != hooks[i])
            ok = 0;
    if (!ok)
        for (i = 0; i < 2; ++i)
            *AT(volatile Addr *, slots[i]) = AI_NATIVE_GET_DRIVE_SPEED;
    VirtualProtect(AT(void *, first), span, old, &unused);
    return ok;
}

static void radio_log(const char *state, RainU32 a, RainU32 b, int repeat)
{
    static const char *last;
    static const char hex[] = "0123456789ABCDEF";
    char line[112];
    char *out = line;
    const char *p;
    unsigned shift;
    DWORD written;
    HANDLE f;
    float now;
    RainU32 ms;
    if (!diag_enabled || !log_path[0] || (!repeat && last == state))
        return;
    last = state;
    for (p = state; *p; ++p)
        *out++ = *p;
    *out++ = ' ';
    for (shift=32; shift; shift-=4)
        *out++ = hex[(a >> (shift-4)) & 15u];
    *out++ = ' ';
    for (shift=32; shift; shift-=4)
        *out++ = hex[(b >> (shift-4)) & 15u];
    now = *AT(const volatile float *, GAME_CLOCK);
    ms = now >= 0.0f && now < 4000000.0f ? (RainU32)(now*1000.0f) : 0;
    *out++ = ' ';
    *out++ = 't';
    *out++ = '=';
    for (shift=32; shift; shift-=4)
        *out++ = hex[(ms >> (shift-4)) & 15u];
    *out++ = '\r';
    *out++ = '\n';
    f = CreateFileA(log_path, 4u, 3u, 0, 4u, 128u, 0); /* append, open or create */
    if (f == AT(HANDLE, -1))
        return;
    WriteFile(f, line, (DWORD)(out-line), &written, 0);
    CloseHandle(f);
}

static void load_menu_registry(void)
{
    void *key=0;
    unsigned i;
    /* Current-user settings: no files, no admin rights, no INI dependency. */
    if (RegOpenKeyExA(AT(void *,0x80000001u), "Software\\MW_WeatherControl",0,1,&key))
        return;
    for(i=0;i<WM_SETTING_COUNT;++i) {
        DWORD type=0, bytes=sizeof(DWORD), value=0;
        if (!RegQueryValueExA(key,menu_keys[i],0,&type,(unsigned char *)&value,&bytes)
            && type==4u && bytes==sizeof(DWORD)) {
            static const RainU32 maximums[WM_SETTING_COUNT]={
                60u,60u,60u,60u,100u,100u,100u,60u,60u
            };
            static const RainU32 minimums[WM_SETTING_COUNT]={
                1u,1u,1u,1u,50u,50u,50u,1u,1u
            };
            if(value<minimums[i]) value=minimums[i];
            if(value>maximums[i]) value=maximums[i];
            menu_values[i]=value;
        } else if (i==WM_DRY_MIN || i==WM_DRY_MAX) {
            /* Upgrade existing 1.0 settings: the old registry names stored
               the dry interval in SECONDS, the new names store MINUTES.
               Favor the closest whole minute; retain old entries untouched
               so switching back to an older mod remains reversible. */
            const char *legacy_key = i==WM_DRY_MIN ?
                "MinimumDrySeconds" : "MaximumDrySeconds";
            DWORD seconds=0;
            type=0;bytes=sizeof seconds;
            if (!RegQueryValueExA(key,legacy_key,0,&type,
                    (unsigned char *)&seconds,&bytes) &&
                type==4u && bytes==sizeof seconds) {
                RainU32 minutes = (seconds / 60u) + ((seconds % 60u)>=30u);
                if (minutes < 1u) minutes = 1u;
                if (minutes > 60u) minutes = 60u;
                menu_values[i] = minutes;
            }
        }
    }
    {
        static const char *const ui_names[5]={
            "WindowX","WindowY","WindowScalePercent","WindowCollapsed","LanguageRU"
        };
        DWORD *const target[5]={
            &wm_window_x,&wm_window_y,&wm_window_scale_pct,
            &wm_window_collapsed,&wm_language_ru
        };
        unsigned j;
        for(j=0;j<5;++j) {
            DWORD type=0, bytes=sizeof(DWORD), value=0;
            if(!RegQueryValueExA(key,ui_names[j],0,&type,(unsigned char *)&value,&bytes)
                && type==4u && bytes==sizeof(DWORD))
                *target[j]=value;
        }
    }
    RegCloseKey(key);
    if(wm_window_x>4000u) wm_window_x=14u;
    if(wm_window_y>4000u) wm_window_y=42u;
    if(wm_window_scale_pct<65u) wm_window_scale_pct=65u;
    if(wm_window_scale_pct>155u) wm_window_scale_pct=155u;
    wm_window_collapsed=wm_window_collapsed?1u:0u;
    wm_language_ru=wm_language_ru?1u:0u;
    if(menu_values[WM_RAIN_MAX]<menu_values[WM_RAIN_MIN])
        menu_values[WM_RAIN_MAX]=menu_values[WM_RAIN_MIN];
    if(menu_values[WM_DRY_MAX]<menu_values[WM_DRY_MIN])
        menu_values[WM_DRY_MAX]=menu_values[WM_DRY_MIN];
}

static void save_menu_settings(void)
{
    void *key=0;
    unsigned i;
    DWORD disposition=0;
    if (RegCreateKeyExA(AT(void *,0x80000001u),"Software\\MW_WeatherControl",
         0,0,0,2,0,&key,&disposition)) return;
    for(i=0;i<WM_SETTING_COUNT;++i) {
        DWORD value=(DWORD)menu_values[i];
        RegSetValueExA(key,menu_keys[i],0,4,(const unsigned char *)&value,sizeof value);
    }
    {
        static const char *const ui_names[5]={
            "WindowX","WindowY","WindowScalePercent","WindowCollapsed","LanguageRU"
        };
        const DWORD values[5]={
            wm_window_x,wm_window_y,wm_window_scale_pct,wm_window_collapsed,wm_language_ru
        };
        unsigned j;
        for(j=0;j<5;++j)
            RegSetValueExA(key,ui_names[j],0,4,
                (const unsigned char *)&values[j],sizeof(DWORD));
    }
    RegCloseKey(key);
}

static void apply_menu_settings(int first)
{
    if(first)
        rain_configure(&controller,menu_values[WM_RAIN_MIN],menu_values[WM_RAIN_MAX],
            menu_values[WM_DRY_MIN],menu_values[WM_DRY_MAX],controller.random_state);
    else {
        /* No timer reset while editing: the newly selected bounds affect the
           next weather episode. This also preserves the RNG state. */
        controller.min_rain=menu_values[WM_RAIN_MIN];
        controller.max_rain=menu_values[WM_RAIN_MAX];
        controller.min_dry=menu_values[WM_DRY_MIN];
        controller.max_dry=menu_values[WM_DRY_MAX];
        /* Bound an already running automatic interval if settings change.
           Do not restart timers, extend them or override scripted weather. */
        if (controller.observed_clock) {
            RainU32 maximum=controller.phase==RAIN_SCHEDULED ?
                controller.max_rain : controller.max_dry;
            if (controller.phase==RAIN_DRY || controller.phase==RAIN_SCHEDULED) {
                double latest=(double)controller.last_time+(double)maximum*60.0;
                if (controller.until>latest) controller.until=latest;
            }
        }
    }
    wet_grip_configure(&wet_grip,menu_values[WM_GRIP],menu_values[WM_SOAK],
        menu_values[WM_DRY_TIME],menu_values[WM_REAR_LATERAL],
        menu_values[WM_LONGITUDINAL]);
    menu_last_revision=menu_revision;
}

static void load_settings(void)
{
    static const char logname[]="MW_WeatherControl_Radio.log";
    char path[260];
    RainU32 seed;
    DWORD n=GetModuleFileNameA(module_handle,path,sizeof path);
    load_menu_registry();
    if (diag_enabled && n && n<sizeof path) {
        unsigned i;
        while(n && path[n-1]!='\\' && path[n-1]!='/') --n;
        if(n && n+sizeof logname<=sizeof log_path) {
            for(i=0;i<n;++i) log_path[i]=path[i];
            for(i=0;i<sizeof logname;++i) log_path[n+i]=logname[i];
        }
    }
    seed=(RainU32)GetTickCount() ^ ((RainU32)GetCurrentProcessId()*0x9e3779b9u);
    controller.random_state=seed;
    apply_menu_settings(1);
    radio_rng=seed^0xf1bbcdc9u;
    if(!radio_rng) radio_rng=0x7fc9d1abu;
    settings_loaded=1;
    radio_log("init-1.0 AI-wet-pace with physics grip",(RainU32)radio_installed,
        (RainU32)(radio_enabled | (ground_fallback<<1) | (ground_only<<2) | (both_sides<<3)),1);
    radio_log("weather-repeat-range-sec",RADIO_GAP_MIN,RADIO_GAP_MAX,1);
    radio_log("weather-filter-sites",(RainU32)filter_sites,0,1);
    radio_log("wet-grip patch/percent",(RainU32)grip_installed,
        (RainU32)(100.5f - 100.0f*wet_grip.loss),1);
    radio_log("wet-rear-lateral patch/percent",(RainU32)rear_lateral_installed,
        (RainU32)(100.5f - 100.0f*wet_grip.rear_loss),1);
    radio_log("wet-longitudinal patch/percent",(RainU32)longitudinal_installed,
        (RainU32)(100.5f - 100.0f*wet_grip.longitudinal_loss),1);
    radio_log("ai-wet-speed-hooks-racer-and-cop",(RainU32)ai_wet_pace_installed,
        (RainU32)(ai_wet_pace_multiplier(1.0f-wet_grip.loss,0.60f)*100.0f),1);
}

static float measured_rain(void)
{
    Addr view = *AT(const volatile Addr *, WEATHER_VIEW);
    float amount = view ? *AT(const volatile float *, view + ACTUAL_RAIN) : 0.0f;
    return radio_normalize_rain(amount, *AT(const volatile RainU32 *, RENDER_MODE));
}

static float rain_target(float x, float y)
{
    float native_target = AT(NativeRainTarget, NATIVE_TARGET)(x, y);
    float now = *AT(const volatile float *, GAME_CLOCK);
    int scripted = *AT(const volatile float *, SCRIPT_RAIN_OVERRIDE) != 0.0f;
    float desired;
    if (!settings_loaded)
        load_settings();
    /* Install the renderer only after the game has entered the running world;
       failure here leaves all existing weather and grip patches working. */
    weather_menu_init();
    if (menu_last_revision != menu_revision)
        apply_menu_settings(0);
    desired = rain_step(&controller, now, native_target, scripted, 0);
    if (menu_command) {
        int command=menu_command;
        menu_command=0;
        if (command==1) {
            controller.phase=RAIN_MANUAL;
            desired=1.0f;
        } else if (command==2) {
            controller.phase=RAIN_MANUAL_OFF;
            controller.suppress_script=1;
            desired=0.0f;
        } else if (command==3) {
            rain_begin_dry(&controller,now);
            controller.suppress_script=0;
            desired=scripted?native_target:0.0f;
        }
    }
    /* Sparse call-count diagnostic: validates actual virtual dispatch under
       rain without tracing each physics tick or creating large log files. */
    if (diag_enabled && ai_wet_pace_installed && now >= 0.0f &&
        (ai_last_log_time == 0.0f || now - ai_last_log_time >= 20.0f)) {
        if (ai_last_logged_racer_calls != ai_racer_speed_calls ||
            ai_last_logged_cop_calls != ai_cop_speed_calls) {
            radio_log("ai-speed-calls-racer-cop", ai_racer_speed_calls,
                ai_cop_speed_calls,1);
            ai_last_logged_racer_calls=ai_racer_speed_calls;
            ai_last_logged_cop_calls=ai_cop_speed_calls;
        }
        ai_last_log_time=now;
    }
    menu_rain_state=controller.phase==RAIN_MANUAL?2:
        controller.phase==RAIN_MANUAL_OFF?3:desired>0.25f?1:0;
    if (grip_installed || radio_enabled) {
        float rain = measured_rain();
        if (grip_installed) {
            wet_grip_scale = wet_grip_step(&wet_grip, now, rain);
            if (longitudinal_installed)
                wet_longitudinal_scale = 1.0f - wet_grip.longitudinal_loss * wet_grip.wetness;
            if (rear_lateral_installed)
                wet_rear_lateral_scale = 1.0f - wet_grip.rear_loss * wet_grip.wetness;
        }
        if (radio_enabled && now >= 0.0f && now < 3.4e38f) {
            radio_note_rain(&rain_radio, now, rain);
            published_rain = rain_radio.serial;
            rain_requested = desired > 0.25f;
        }
    }
    return desired;
}

/* ECX is the original this pointer. The EDX placeholder gives a C type
   with the same stack layout as the native methods (including ret 8). */
typedef void (FASTCALL *NativeRadioTick)(void *, void *);
typedef Addr (FASTCALL *NativeFindUnit)(void *, void *, int, int);
typedef Addr (FASTCALL *NativeWeatherReport)(void *, void *);
typedef RainU32 (*NativeSpeechHash)(const char *);
typedef U8 (*NativeSpeechPending)(RainU32, unsigned int);
typedef U8 (*NativeSpeechActive)(RainU32);
typedef U8 (*NativeChannelBusy)(unsigned int);
typedef int (*NativeSpeechFilter)(Addr, unsigned int);
typedef RainU32 (FASTCALL *NativeSpeechID)(void *, void *, RainU32);
static RainU32 speech_hash;

static int speaker_is_ready(Addr speaker, Addr vtable)
{
    return speaker && *AT(const volatile Addr *, speaker) == vtable &&
        *AT(const volatile U8 *, speaker + SPEAKER_READY_OFFSET) != 0;
}

static Addr select_ground(void *pursuit, Addr start, Addr end)
{
    Addr item;
    Addr chosen = AT(NativeFindUnit, FIND_UNIT)(pursuit, 0, 1, 0);
    if (speaker_is_ready(chosen, GROUND_SPEAKER_VTABLE))
        return chosen;

    /* Native lookup may return the air object, or exclude ground speakers
       with its optional additional predicate. Match our intended speaker
       type in the same live array and still require the native ready bit. */
    if ((end-start) > 2048u || ((end-start) & 7u))
        return 0;
    for (item = start; item < end; item += 8) {
        chosen = *AT(const volatile Addr *, item+4);
        if (speaker_is_ready(chosen, GROUND_SPEAKER_VTABLE))
            return chosen;
    }
    return 0;
}

static int weather_speech_filter(Addr event, unsigned int flags)
{
    static const unsigned int native_repeat_tests = 0x0au;
    int result;
    if (!event || !voice_trace.tracking || event != voice_trace.event ||
        *AT(const volatile RainU32 *, event + 8u) != voice_trace.id ||
        *AT(const volatile Addr *, event + 0xcu) != voice_trace.speaker ||
        *AT(const volatile float *, GAME_CLOCK) < voice_trace.requested_at)
        return AT(NativeSpeechFilter, NATIVE_SPEECH_FILTER)(event, flags);

    /* Per-event maximum count and gap from the last play are already
       managed by this mod. Preserve every other native speech check. */
    if (rain_requested)
        flags &= ~native_repeat_tests;
    result = AT(NativeSpeechFilter, NATIVE_SPEECH_FILTER)(event, flags);
    if (voice_trace.filter_result != result) {
        voice_trace.filter_result = result;
        radio_log("weather-native-filter result/flags", (RainU32)result, (RainU32)flags, 1);
    }
    return result;
}

static int weather_list_active(RainU32 id)
{
    return *AT(const volatile Addr *, RADIO_ACTIVE_LIST) &&
        AT(NativeSpeechActive, RADIO_IS_ACTIVE)(id);
}

static void trace_voice(float now)
{
    int state, current;
    RainU32 id;
    if (!voice_trace.tracking)
        return;
    if (now < voice_trace.requested_at) {
        voice_trace.tracking = 0;
        return;
    }
    id = voice_trace.id;
    current = AT(NativeSpeechActive, RADIO_IS_CURRENT)(id) != 0;
    if (current && AT(NativeChannelBusy, RADIO_CHANNEL_BUSY)(1)) {
        state = 4;
        voice_trace.saw_playback = 1;
    } else if (current)
        state = 3;
    else if (weather_list_active(id))
        state = 2;
    else if (AT(NativeSpeechPending, RADIO_IS_PENDING)(id, 4))
        state = 1;
    else
        state = 0;
    if (state != voice_trace.state) {
        static const char *const stages[] = {
            "weather-cleared id/role", "weather-pending id/role",
            "weather-active-list id/role", "weather-current id/role",
            "weather-channel-playing id/role"
        };
        voice_trace.state = state;
        if (state)
            radio_log(stages[state], id, voice_trace.role, 1);
    }
    if (!state) {
        /* A very short state transition can land between polls. Report
           only what was observed, not that the signal was inaudible. */
        if (now - voice_trace.requested_at < 2.0f)
            return;
        radio_log(voice_trace.saw_playback ? "weather-finished id/role" :
            "weather-left-without-observed-playback id/role", id, voice_trace.role, 1);
        radio_note_result(&reports[voice_trace.role], voice_trace.serial,
            now, voice_trace.saw_playback);
        voice_trace.tracking = 0;
    }
}

static void FASTCALL radio_tick(void *pursuit, void *unused)
{
    Addr p = (Addr)pursuit, heli, speaker, event;
    RainU32 serial, id, gap = 0;
    float now, actual;
    int role, air_due, ground_due, air_ready;
    struct WeatherRadioReport *report;
    (void)unused;
    AT(NativeRadioTick, NATIVE_RADIO_TICK)(pursuit, 0);

    if (!radio_enabled || !p)
        return;
    if (p != *AT(const volatile Addr *, PLAYER_PURSUIT)) {
        if (rain_requested && !*AT(const volatile Addr *, PLAYER_PURSUIT))
            radio_log("no-player-pursuit", 0, 0, 0);
        return;
    }
    now = *AT(const volatile float *, GAME_CLOCK);
    if (!(now >= 0.0f && now < 3.4e38f))
        return;
    if (!*AT(const volatile RainU32 *, RADIO_READY)) {
        radio_log("radio-unavailable", 0, 0, 0);
        return;
    }
    trace_voice(now);
    if (*AT(const volatile RainU32 *, p + 0x1d4) != 0 ||
        *AT(const volatile RainU32 *, p + 0x138) == 1 ||
        !*AT(const volatile Addr *, p + 0x130)) {
        radio_log("pursuit-not-active", *AT(const volatile RainU32 *, p + 0x1d4),
            *AT(const volatile RainU32 *, p + 0x138), 0);
        return;
    }
    actual = measured_rain();
    if (!rain_requested || !(actual > 0.25f)) {
        radio_log("rain-not-ready mode/amount-x1000", *AT(const volatile RainU32 *, RENDER_MODE),
            actual >= 0.f && actual <= 8.f ? (RainU32)(actual*1000.f) : 0, 0);
        return;
    }

    serial = published_rain;
    if (now < last_radio_time)
        dispatch_after = 0.0f;
    last_radio_time = now;
    if (now < dispatch_after)
        return;

    air_due = radio_report_due(&reports[AIR_REPORT], serial, now) && !ground_only;
    ground_due = radio_report_due(&reports[GROUND_REPORT], serial, now) &&
        (both_sides || ground_only || ground_fallback);
    if (!air_due && !ground_due)
        return;

    /* A late helicopter can report after ground; an available helicopter
       goes first if both roles are ready on the same update. */
    heli = *AT(const volatile Addr *, p + 0xe0);
    air_ready = !ground_only && speaker_is_ready(heli, AIR_SPEAKER_VTABLE);
    if (air_due && air_ready) {
        speaker = heli;
        role = AIR_REPORT;
    } else if (ground_due && (both_sides || !air_ready)) {
        Addr start = *AT(const volatile Addr *, p + 0x5c);
        Addr end = *AT(const volatile Addr *, p + 0x60);
        if (!start || end <= start) {
            radio_log("ground-none", 0, 0, 0);
            return;
        }
        speaker = select_ground(pursuit, start, end);
        if (!speaker) {
            radio_log("ground-not-ready", speaker, 0, 0);
            reports[GROUND_REPORT].retry_at = now + 2.0f;
            return;
        }
        role = GROUND_REPORT;
    } else {
        if (air_due && !air_ready)
            radio_log("air-not-ready pointer/flags", heli, heli ?
                *AT(const volatile U8 *, heli + SPEAKER_READY_OFFSET) : 0, 0);
        return;
    }
    report = &reports[role];

    if (!*AT(const volatile Addr *, RADIO_EVENT_TABLE+4) ||
        !*AT(const volatile RainU32 *, RADIO_EVENT_TABLE+0xc)) {
        radio_log("speech-map-not-ready", 0, 0, 0);
        dispatch_after = now + 2.0f;
        return;
    }
    if (!speech_hash)
        speech_hash = AT(NativeSpeechHash, RADIO_KEY_HASH)(AT(const char *, WEATHER_NAME));
    id = AT(NativeSpeechID, RADIO_KEY_TO_ID)(AT(void *, RADIO_EVENT_TABLE), 0, speech_hash);
    if (id == RADIO_UNMAPPED) {
        radio_log("weather-event-unmapped id/hash", id, speech_hash, 0);
        dispatch_after = now + 6.0f;
        return;
    }
    /* The stock enqueue function deduplicates by ID, not speaker.
       Don't submit the second request while the identical ID is pending
       or in the native active-event list. */
    if (AT(NativeSpeechPending, RADIO_IS_PENDING)(id, 4) ||
        weather_list_active(id) || AT(NativeSpeechActive, RADIO_IS_CURRENT)(id)) {
        radio_log("weather-event-busy id/role", id, (RainU32)role, 0);
        dispatch_after = now + 1.0f;
        return;
    }
    {
        Addr pool = *AT(const volatile Addr *, RADIO_EVENT_POOL);
        if (pool && *AT(const volatile RainU32 *, pool+0x18u) ==
                    *AT(const volatile RainU32 *, pool+0x2cu)) {
            radio_log("weather-slotpool-full id/role", id, (RainU32)role, 0);
            dispatch_after = now + 2.0f;
            return;
        }
    }
    event = AT(NativeWeatherReport, WEATHER_REPORT)(AT(void *, speaker), 0);
    if (event || report->attempts == 3u)
        gap = radio_draw_gap(report, &radio_rng);
    radio_record_attempt(report, now, event != 0, gap);
    if (event) {
        dispatch_after = now + (float)RADIO_SHARED_GAP;
        voice_trace.tracking = 1;
        voice_trace.state = -1;
        voice_trace.saw_playback = 0;
        voice_trace.filter_result = -1;
        voice_trace.id = id;
        voice_trace.role = (RainU32)role;
        voice_trace.serial = serial;
        voice_trace.event = event;
        voice_trace.speaker = speaker;
        voice_trace.requested_at = now;
    }
    radio_log(role == AIR_REPORT ? (event ? "air-queued id/event" : "air-rejected id/try") :
        (event ? "ground-queued id/event" : "ground-rejected id/try"),
        id, event ? (RainU32)event : (RainU32)report->attempts, 1);
    if (event)
        radio_log("weather-next-gap sec/role", gap, (RainU32)role, 1);
}

static RainU32 read32(const U8 *p)
{
    return (RainU32)p[0] | ((RainU32)p[1]<<8) |
        ((RainU32)p[2]<<16) | ((RainU32)p[3]<<24);
}

static int same(const U8 *a, const U8 *b, unsigned n)
{
    unsigned i;
    for (i=0; i<n; ++i)
        if (a[i]!=b[i])
            return 0;
    return 1;
}

static int supported_image(void)
{
    static const U8 callsite[] = {
        0xe8,0x7a,0xb6,0xff,0xff,0xd8,0x15,0x68,0x09,0x89,0x00,0x83,0xc4,0x08,
        0xd9,0x96,0x94,0x36,0x00,0x00
    };
    static const U8 native_entry[] = {
        0x51,0xa0,0xb9,0x0f,0x9b,0x00,0x84,0xc0
    };
    static const U8 clock_add[] = {0xd8,0x46,0x1c};
    static const U8 clock_store[] = {0xd9,0x1d,0xd8,0x85,0x98,0x00};
    const U8 *base = AT(const U8 *, GAME_BASE);
    RainU32 pe;
    if (GetModuleHandleA(0) != AT(HINSTANCE, GAME_BASE) || base[0]!='M' || base[1]!='Z')
        return 0;
    pe=read32(base+0x3c);
    return pe>=0x40 && pe<=0x1000 && read32(base+pe)==0x4550u &&
        base[pe+4]==0x4c && base[pe+5]==1 &&
        base[pe+24]==0x0b && base[pe+25]==1 &&
        read32(base+pe+0x34)==GAME_BASE && read32(base+pe+0x50)==GAME_IMAGE_SIZE &&
        same(AT(const U8 *, RAIN_UPDATE_CALL),callsite,sizeof callsite) &&
        same(AT(const U8 *, NATIVE_TARGET),native_entry,sizeof native_entry) &&
        same(AT(const U8 *,0x006f6dd2),clock_add,sizeof clock_add) &&
        same(AT(const U8 *,0x006f6ddd),clock_store,sizeof clock_store);
}

static int supported_radio(void)
{
    static const U8 context[] = {
        0xe8,0x56,0xb6,0xff,0xff,0x8b,0xce,0xe8,0x5f,0xfa,0xfd,0xff,
        0xa1,0x00,0x87,0x8f,0x00,0x85,0xc0
    };
    static const U8 stock_tick[] = {0xa1,0x00,0x87,0x8f,0x00,0x85,0xc0,0x56,0x8b,0xf1};
    static const U8 current_method[] = {0xa1,0x30,0x22,0x99,0x00,0x85,0xc0,0x74,0x16,0x8b,0x80,0x88};
    static const U8 playing_method[] = {0x8b,0x44,0x24,0x04,0x8b,0x04,0x85,0x2c,0x22,0x99,0x00};
    static const U8 pending_method[] = {0x8b,0x44,0x24,0x08,0x33,0xc9,0x83,0xf8,0x04};
    static const U8 active_method[] = {0x8b,0x0d,0xe8,0x33,0x99,0x00,0x8b,0x01,0x3b,0xc1};
    static const U8 hash_method[] = {0x81,0xec,0x00,0x01,0x00,0x00,0x56,0x57};
    static const U8 id_method[] = {0x8b,0x41,0x04,0x8b,0x49,0x0c,0x8d,0x0c,0xc8};
    static const U8 render_scale[] = {0xe8,0xad,0x71,0xf6,0xff,0x83,0xf8,0x03,0x7c,0x12};
    static const U8 mode_getter[] = {0xa1,0x3c,0x2b,0x98,0x00,0xc3};
    static const U8 report_method[] = {
        0x51,0x8b,0x41,0x0c,0x51,0x68,0x6c,0x23,0x99,0x00,0x68,0xa4,0x1e,0x90,
        0x00,0x8d,0x4c,0x24,0x0c,0x51,0x6a,0x04,0x89,0x44,0x24,0x14,0xe8,0x01,
        0xa0,0xff,0xff,0x83,0xc4,0x18,0xc3
    };
    static const U8 event[] = {0xa7,0x5b,0xf4,0x54,0x6c,0x13,0x8b,0x00};
    static const U8 ready_method[] = {0x8a,0x41,0x35,0xc3};
    static const U8 find_method[] = {0x6a,0xff,0x68,0x68,0x01,0x88,0x00,0x64,0xa1};
    static const U8 pool_method[] = {0xa1,0xdc,0x22,0x99,0x00,0x8b,0x48,0x18,0x3b,0x48,0x2c};
    static const U8 vector_method[] = {0x8b,0x75,0x5c,0x3b,0x75,0x60,0x74,0x3f,0x8a,0x5c,0x24,0x48,0x8b,0x7e,0x04};
    static const U8 queue_method[] = {0x64,0xa1,0,0,0,0,0x6a,0xff,0x68,0x81,0x00,0x88,0x00};
    return same(AT(const U8 *, RADIO_TICK_CALL-7), context, sizeof context) &&
        same(AT(const U8 *, NATIVE_RADIO_TICK), stock_tick, sizeof stock_tick) &&
        same(AT(const U8 *, RADIO_IS_CURRENT), current_method, sizeof current_method) &&
        same(AT(const U8 *, RADIO_CHANNEL_BUSY), playing_method, sizeof playing_method) &&
        same(AT(const U8 *, RADIO_IS_PENDING), pending_method, sizeof pending_method) &&
        same(AT(const U8 *, RADIO_IS_ACTIVE), active_method, sizeof active_method) &&
        same(AT(const U8 *, RADIO_KEY_HASH), hash_method, sizeof hash_method) &&
        same(AT(const U8 *, RADIO_KEY_TO_ID), id_method, sizeof id_method) &&
        same(AT(const U8 *, 0x75837e), render_scale, sizeof render_scale) &&
        same(AT(const U8 *, 0x6bf530), mode_getter, sizeof mode_getter) &&
        same(AT(const U8 *, WEATHER_REPORT), report_method, sizeof report_method) &&
        same(AT(const U8 *, 0x901ea0), event, sizeof event) &&
        same(AT(const U8 *, 0x713b20), queue_method, sizeof queue_method) &&
        same(AT(const U8 *, 0x713b85), pool_method, sizeof pool_method) &&
        same(AT(const U8 *, 0x715405), vector_method, sizeof vector_method) &&
        same(AT(const U8 *, 0x7002b0), ready_method, sizeof ready_method) &&
        same(AT(const U8 *, FIND_UNIT), find_method, sizeof find_method) &&
        read32(AT(const U8 *, AIR_SPEAKER_VTABLE + 0x154)) == WEATHER_REPORT &&
        read32(AT(const U8 *, GROUND_SPEAKER_VTABLE + 0x154)) == WEATHER_REPORT &&
        read32(AT(const U8 *, AIR_SPEAKER_VTABLE + 0x60)) == 0x7002b0 &&
        read32(AT(const U8 *, GROUND_SPEAKER_VTABLE + 0x60)) == 0x7002b0;
}

static int replace_call(Addr source, Addr native, Addr dest)
{
    U8 *at = AT(U8 *, source);
    RainU32 original = (RainU32)(native - source - 5u);
    RainU32 rel = (RainU32)(dest - source - 5u);
    DWORD old, unused;
    HANDLE process;
    BOOL patched;
    unsigned i;
    if (!VirtualProtect(at, 5, 0x40u, &old))
        return 0;
    for (i=0;i<4;++i)
        at[i+1]=(U8)(rel>>(8u*i));
    process=GetCurrentProcess();
    patched=FlushInstructionCache(process,at,5);
    if (!patched) {
        for (i=0;i<4;++i)
            at[i+1]=(U8)(original>>(8u*i));
        FlushInstructionCache(process,at,5);
    }
    VirtualProtect(at,5,old,&unused);
    return patched != 0;
}

static int install_wet_grip(void)
{
    static const U8 stock[] = {0xd8,0x8e,0xec,0,0,0};
    static const U8 drive_after[] = {0xd8,0x4c,0x24,0x1c,0xd9,0x54,0x24,0x08};
    static const U8 limit_after[] = {0xd8,0x4c,0x24,0x28,0xd8,0x8e,0xd4,0,0,0};
    static const U8 wheel_call[] = {0x8b,0xcf,0xe8,0x93,0x31,0xff,0xff,0xd9,0x5c,0x24,0x64};
    static const U8 weather_update[] = {0xd8,0xa6,0x8c,0x02,0,0};
    static const U8 render_scale[] = {0xe8,0xad,0x71,0xf6,0xff,0x83,0xf8,0x03,0x7c,0x12};
    static const Addr sites[] = {TIRE_GRIP_DRIVE, TIRE_GRIP_LIMIT};
    DWORD old, unused;
    const unsigned span = TIRE_GRIP_LIMIT - TIRE_GRIP_DRIVE + sizeof stock;
    U8 *first = AT(U8 *, TIRE_GRIP_DRIVE);
    unsigned i, j;
    BOOL patched;
    HANDLE process;
    if (!same(first, stock, sizeof stock) ||
        !same(AT(const U8 *, TIRE_GRIP_LIMIT), stock, sizeof stock) ||
        !same(first + sizeof stock, drive_after, sizeof drive_after) ||
        !same(AT(const U8 *, TIRE_GRIP_LIMIT + sizeof stock), limit_after, sizeof limit_after) ||
        !same(AT(const U8 *, 0x6aa8f6), wheel_call, sizeof wheel_call) ||
        !same(AT(const U8 *, 0x7583c5), weather_update, sizeof weather_update) ||
        !same(AT(const U8 *, 0x75837e), render_scale, sizeof render_scale) ||
        !VirtualProtect(first, span, 0x40u, &old))
        return 0;
    for (i=0; i<2; ++i) {
        U8 *at = AT(U8 *, sites[i]);
        RainU32 rel = (RainU32)((Addr)tire_grip_hook - sites[i] - 5u);
        at[0] = 0xe8;
        for (j=0; j<4; ++j)
            at[j+1] = (U8)(rel >> (8u*j));
        at[5] = 0x90;
    }
    process = GetCurrentProcess();
    patched = FlushInstructionCache(process, first, span);
    if (!patched) {
        for (i=0; i<2; ++i)
            for (j=0; j<sizeof stock; ++j)
                AT(U8 *, sites[i])[j] = stock[j];
        FlushInstructionCache(process, first, span);
    }
    VirtualProtect(first, span, old, &unused);
    return patched != 0;
}

static int install_rear_lateral(void)
{
    static const U8 stock[] = {0xd8,0x8e,0xd8,0,0,0};
    static const U8 before[] = {0xd9,0x81,0xf0,0,0,0};
    static const U8 after[] = {0xd9,0x9e,0xd8,0,0,0,0xd9,0x81,0xec,0,0,0};
    static const U8 wheel_axle[] = {0xd1,0xf9,0x89,0x8e,0x0c,0x01,0,0};
    U8 *at = AT(U8 *, TIRE_LATERAL_FORCE);
    RainU32 rel = (RainU32)((Addr)rear_lateral_hook - TIRE_LATERAL_FORCE - 5u);
    DWORD old, unused;
    HANDLE process;
    BOOL patched;
    unsigned i;
    if (!same(at-sizeof before, before, sizeof before) ||
        !same(at, stock, sizeof stock) ||
        !same(at+sizeof stock, after, sizeof after) ||
        !same(AT(const U8 *, 0x69d9c0), wheel_axle, sizeof wheel_axle) ||
        !VirtualProtect(at, sizeof stock, 0x40u, &old))
        return 0;
    at[0] = 0xe8;
    for (i=0; i<4; ++i)
        at[i+1] = (U8)(rel >> (8u*i));
    at[5] = 0x90;
    process = GetCurrentProcess();
    patched = FlushInstructionCache(process, at, sizeof stock);
    if (!patched) {
        for (i=0; i<sizeof stock; ++i)
            at[i] = stock[i];
        FlushInstructionCache(process, at, sizeof stock);
    }
    VirtualProtect(at, sizeof stock, old, &unused);
    return patched != 0;
}

/* Normalize forward force demand by the wet longitudinal multiplier before
   the stock combined-slip clamp, then convert back to real force before
   wheel angular integration. Linear response below the limit is preserved,
   while the longitudinal saturation threshold comes sooner. Both reads
   must be installed together. They live on different protection pages. */
static int install_longitudinal(void)
{
    static const U8 stock[] = {0xd8,0x8e,0xdc,0,0,0};
    static const U8 demand_context[] = {
        0xd9,0x86,0x20,0x01,0,0,0xd8,0x8e,0xdc,0,0,0,0xd9,0x96,0xdc,0,0,0,0xd9,0xc0
    };
    static const U8 force_context[] = {
        0xd9,0x81,0xec,0,0,0,0xd8,0x8e,0xdc,0,0,0,0xd9,0x96,0xdc,0,0,0
    };
    static const Addr sites[] = {TIRE_LONG_DEMAND, TIRE_LONG_FORCE};
    static void (* const hooks[])(void) = {longitudinal_demand_hook, longitudinal_force_hook};
    DWORD old[2], unused;
    HANDLE process;
    unsigned i, j;
    BOOL a, b;
    if (!same(AT(const U8 *, TIRE_LONG_DEMAND-6), demand_context, sizeof demand_context) ||
        !same(AT(const U8 *, TIRE_LONG_FORCE-6), force_context, sizeof force_context))
        return 0;
    if (!VirtualProtect(AT(void *, sites[0]), sizeof stock, 0x40u, &old[0]))
        return 0;
    if (!VirtualProtect(AT(void *, sites[1]), sizeof stock, 0x40u, &old[1])) {
        VirtualProtect(AT(void *, sites[0]), sizeof stock, old[0], &unused);
        return 0;
    }
    process = GetCurrentProcess();
    for (i=0; i<2; ++i) {
        U8 *at = AT(U8 *, sites[i]);
        RainU32 rel = (RainU32)((Addr)hooks[i] - sites[i] - 5u);
        at[0] = 0xe8;
        for (j=0; j<4; ++j)
            at[j+1] = (U8)(rel >> (j*8u));
        at[5] = 0x90;
    }
    a = FlushInstructionCache(process, AT(void *, sites[0]), sizeof stock);
    b = FlushInstructionCache(process, AT(void *, sites[1]), sizeof stock);
    if (!a || !b) {
        for (i=0; i<2; ++i) {
            for (j=0; j<sizeof stock; ++j)
                AT(U8 *, sites[i])[j] = stock[j];
            FlushInstructionCache(process, AT(void *, sites[i]), sizeof stock);
        }
    }
    for (i=0; i<2; ++i)
        VirtualProtect(AT(void *, sites[i]), sizeof stock, old[i], &unused);
    return a && b;
}

static int install_speech_filter(void)
{
    static const U8 early[] = {0x6a,0xff,0x56,0xe8,0xe1,0xf8,0xff,0xff,0x83,0xc4,0x08,0x8d,0x4c};
    static const U8 late[] = {0x6a,0xff,0x56,0xe8,0x4e,0xab,0xff,0xff,0x83,0xc4,0x08,0x83,0xf8,0x02};
    static const U8 method[] = {0x64,0xa1,0x00,0x00,0x00,0x00,0x6a,0xff,0x68,0x38,0xfb,0x87,0x00};
    static const U8 count_test[] = {0xf6,0x44,0x24,0x50,0x02,0x74,0x5b};
    static const U8 gap_test[] = {0xf6,0x44,0x24,0x50,0x08,0x74,0x1f};
    int installed = 0;
    if (!same(AT(const U8 *, NATIVE_SPEECH_FILTER), method, sizeof method) ||
        !same(AT(const U8 *, 0x70c75f), count_test, sizeof count_test) ||
        !same(AT(const U8 *, 0x70c85a), gap_test, sizeof gap_test))
        return 0;
    if (same(AT(const U8 *, SPEECH_FILTER_QUEUE_CALL-3u), early, sizeof early))
        installed |= replace_call(SPEECH_FILTER_QUEUE_CALL, NATIVE_SPEECH_FILTER, (Addr)weather_speech_filter);
    if (same(AT(const U8 *, SPEECH_FILTER_READY_CALL-3u), late, sizeof late))
        installed |= replace_call(SPEECH_FILTER_READY_CALL, NATIVE_SPEECH_FILTER, (Addr)weather_speech_filter) << 1;
    return installed;
}

#include "weather_menu.h"

BOOL STDCALL DllMain(HINSTANCE module, DWORD reason, void *reserved)
{
    HINSTANCE pinned = module;
    (void)reserved;
    if (reason != 1u)
        return 1;
    DisableThreadLibraryCalls(module);
    if (!supported_image() || !GetModuleHandleExA(5u,AT(const char *,module),&pinned))
        return 1;
    module_handle=module;
    if (replace_call(RAIN_UPDATE_CALL, NATIVE_TARGET, (Addr)rain_target)) {
        radio_installed = supported_radio() ?
            replace_call(RADIO_TICK_CALL, NATIVE_RADIO_TICK, (Addr)radio_tick) : 0;
        if (radio_installed)
            filter_sites = install_speech_filter();
        grip_installed = install_wet_grip();
        if (grip_installed) {
            rear_lateral_installed = install_rear_lateral();
            longitudinal_installed = install_longitudinal();
            if (longitudinal_installed && rear_lateral_installed)
                ai_wet_pace_installed = install_ai_wet_pace();
        }
    }
    return 1;
}
