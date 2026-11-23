#ifndef MW_WET_GRIP_H
#define MW_WET_GRIP_H

enum { WET_GRIP_DEFAULT = 80, WET_SOAK_DEFAULT = 5, WET_DRY_DEFAULT = 30, WET_REAR_LATERAL_DEFAULT = 78, WET_LONGITUDINAL_DEFAULT = 75 };

struct WetGrip {
    float wetness, last_time, loss, rear_loss, longitudinal_loss, soak_rate, dry_rate;
    int observed;
};

static void wet_grip_configure(struct WetGrip *s, RainU32 percent, RainU32 soak, RainU32 dry, RainU32 rear_lateral, RainU32 longitudinal)
{
    if (percent < 50u) percent = 50u;
    if (percent > 100u) percent = 100u;
    if (soak < 1u) soak = 1u;
    if (soak > 60u) soak = 60u;
    if (dry < 1u) dry = 1u;
    if (dry > 60u) dry = 60u;
    if (rear_lateral < 50u) rear_lateral = 50u;
    if (rear_lateral > 100u) rear_lateral = 100u;
    if (longitudinal < 50u) longitudinal = 50u;
    if (longitudinal > 100u) longitudinal = 100u;
    s->longitudinal_loss = (float)(100u - longitudinal) / 100.0f;
    s->rear_loss = (float)(100u - rear_lateral) / 100.0f;
    s->loss = (float)(100u - percent) / 100.0f;
    s->soak_rate = 1.0f / (float)soak;
    s->dry_rate = 1.0f / (float)dry;
}

static float wet_grip_step(struct WetGrip *s, float now, float intensity)
{
    float target, dt, next;
    if (!(now >= 0.0f && now < 3.4e38f)) {
        s->observed = 0;
        s->wetness = 0.0f;
        return 1.0f;
    }
    if (!s->observed || now < s->last_time) {
        s->observed = 1;
        s->last_time = now;
        s->wetness = 0.0f;
        return 1.0f;
    }
    dt = now - s->last_time;
    s->last_time = now;
    /* Ignore tiny residual rain. Reach the full effect before the engine's
       asymptotic approach to maximum rain has finished. */
    target = !(intensity > 0.05f && intensity < 64.0f) ? 0.0f :
        intensity >= 0.85f ? 1.0f : (intensity - 0.05f) / 0.80f;
    if (dt > 0.0f) {
        if (target > s->wetness) {
            next = s->wetness + dt * s->soak_rate;
            s->wetness = next >= target - 0.000001f ? target : next;
        } else if (target < s->wetness) {
            next = s->wetness - dt * s->dry_rate;
            s->wetness = next <= target + 0.000001f ? target : next;
        }
    }
    return 1.0f - s->loss * s->wetness;
}

#endif
