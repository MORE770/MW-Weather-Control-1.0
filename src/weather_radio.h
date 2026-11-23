#ifndef MW_WEATHER_RADIO_H
#define MW_WEATHER_RADIO_H

/* Keep measured rain and reporting separate. A new wet period is only
   armed after the renderer has actually returned close to dry. */
struct WeatherRadioRain {
    RainU32 serial;
    float last_time;
    int was_wet;
    int observed_time;
};

struct WeatherRadioReport {
    RainU32 serial;
    float retry_at;
    unsigned attempts, missed;
    RainU32 last_gap;
    int queued;
};

enum { RADIO_GAP_MIN = 20, RADIO_GAP_MAX = 45, RADIO_SHARED_GAP = 12 };

static RainU32 radio_draw_gap(struct WeatherRadioReport *report, RainU32 *rng)
{
    RainU32 previous = report->last_gap;
    unsigned choices = RADIO_GAP_MAX - RADIO_GAP_MIN + 1u - (previous != 0);
    RainU32 next = RADIO_GAP_MIN + rain_next_random(rng) % choices;
    if (previous && next >= previous)
        ++next;
    report->last_gap = next;
    return next;
}

/* The game stores 1/8 of the target intensity in render mode 3. */
static float radio_normalize_rain(float amount, RainU32 mode)
{
    return mode >= 3u ? amount*8.0f : amount;
}

static void radio_note_rain(struct WeatherRadioRain *rain, float now, float intensity)
{
    if (rain->observed_time && now < rain->last_time) {
        ++rain->serial;
        rain->was_wet = 0;
    }
    rain->last_time = now;
    rain->observed_time = 1;
    if (intensity > 0.25f)
        rain->was_wet = 1;
    else if (intensity <= 0.05f && rain->was_wet) {
        rain->was_wet = 0;
        ++rain->serial;
    }
}

static int radio_report_due(struct WeatherRadioReport *report, RainU32 serial, float now)
{
    if (report->serial != serial) {
        report->serial = serial;
        report->attempts = 0;
        report->missed = 0;
        report->queued = 0;
        report->retry_at = 0.0f;
    }
    if (now < report->retry_at)
        return 0;
    if (report->queued || report->attempts >= 4u) {
        report->attempts = 0;
        report->queued = 0;
    }
    return 1;
}

static void radio_record_attempt(struct WeatherRadioReport *report, float now, int queued,
    RainU32 gap)
{
    ++report->attempts;
    if (queued)
        report->retry_at = now + (float)gap;
    else
        report->retry_at = now + (report->attempts >= 4u && gap > 6u ? (float)gap : 6.0f);
    report->queued = queued;
}

static void radio_note_result(struct WeatherRadioReport *report, RainU32 serial,
    float now, int played)
{
    if (report->serial != serial)
        return;
    if (played)
        report->missed = 0;
    else if (report->missed < 2u) {
        ++report->missed;
        if (report->retry_at > now + 6.0f)
            report->retry_at = now + 6.0f;
    }
}

#endif
