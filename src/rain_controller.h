#ifndef MW_RAIN_CONTROLLER_H
#define MW_RAIN_CONTROLLER_H

typedef unsigned int RainU32;
enum RainPhase { RAIN_DRY, RAIN_SCHEDULED, RAIN_MANUAL, RAIN_MANUAL_OFF };

struct RainController {
    /* Rain duration and dry break are both expressed in whole minutes. */
    RainU32 min_rain, max_rain, min_dry, max_dry, random_state;
    double until;
    float last_time;
    int observed_clock, script_was_active, suppress_script;
    enum RainPhase phase;
};

static void rain_configure(struct RainController *s, RainU32 min_rain, RainU32 max_rain,
    RainU32 min_dry, RainU32 max_dry, RainU32 seed)
{
    if (min_rain < 1u) min_rain=1u;
    if (max_rain > 60u) max_rain=60u;
    if (min_rain > 60u) min_rain=60u;
    if (min_dry < 1u) min_dry=1u;
    if (max_dry > 60u) max_dry=60u;
    if (min_dry > 60u) min_dry=60u;
    s->min_rain = min_rain;
    s->max_rain = max_rain < min_rain ? min_rain : max_rain;
    s->min_dry = min_dry;
    s->max_dry = max_dry < min_dry ? min_dry : max_dry;
    s->random_state = seed ? seed : 0xa51e0b97u;
}

static RainU32 rain_next_random(RainU32 *state)
{
    RainU32 r = *state;
    r ^= r << 13;
    r ^= r >> 17;
    r ^= r << 5;
    *state = r;
    return r;
}

static double rain_sample_seconds(struct RainController *s,
    RainU32 lower, RainU32 upper, double unit)
{
    if (lower == upper)
        return (double)lower * unit;
    return ((double)lower + (double)(upper - lower) *
        ((double)(rain_next_random(&s->random_state) - 1u) / 4294967294.0)) * unit;
}

static void rain_begin_dry(struct RainController *s, float now)
{
    s->phase = RAIN_DRY;
    s->until = (double)now + rain_sample_seconds(s, s->min_dry, s->max_dry, 60.0);
}

static void rain_begin_scheduled(struct RainController *s, float now)
{
    s->phase = RAIN_SCHEDULED;
    s->until = (double)now + rain_sample_seconds(s, s->min_rain, s->max_rain, 60.0);
}

static float rain_step(struct RainController *s, float game_seconds,
    float native_target, int scripted, int hotkey_pressed)
{
    float desired;
    /* The simulation clock pauses with the game and restarts for a new world. */
    if (!(game_seconds >= 0.0f && game_seconds < 3.4e38f)) {
        s->observed_clock = 0;
        return native_target;
    }
    if (!s->observed_clock || game_seconds < s->last_time) {
        s->observed_clock = 1;
        s->suppress_script = 0;
        rain_begin_dry(s, game_seconds);
    } else if (s->script_was_active && s->phase != RAIN_MANUAL && s->phase != RAIN_MANUAL_OFF) {
        s->until += (double)game_seconds - (double)s->last_time;
    }
    s->last_time = game_seconds;
    s->script_was_active = scripted;
    if (!scripted) {
        s->suppress_script = 0;
        if (s->phase != RAIN_MANUAL && s->phase != RAIN_MANUAL_OFF && (double)game_seconds >= s->until) {
            if (s->phase == RAIN_DRY)
                rain_begin_scheduled(s, game_seconds);
            else
                rain_begin_dry(s, game_seconds);
        }
    }
    if (s->phase == RAIN_MANUAL) desired = 1.0f;
    else if (s->phase == RAIN_MANUAL_OFF) desired = 0.0f;
    else if (scripted) desired = s->suppress_script ? 0.0f : native_target;
    else desired = s->phase == RAIN_SCHEDULED ? 1.0f : 0.0f;

    if (!hotkey_pressed)
        return desired;
    if (desired > 0.0f) {
        rain_begin_dry(s, game_seconds);
        s->suppress_script = scripted;
        return 0.0f;
    }
    s->phase = RAIN_MANUAL;
    return 1.0f;
}
#endif
