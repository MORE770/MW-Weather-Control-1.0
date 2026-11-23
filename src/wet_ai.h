#ifndef MW_WET_AI_H
#define MW_WET_AI_H

/* A weather-dependent target-speed adjustment for racer and cop AI.
   The global tire physics already uses wheel grip for every simulated car.
   A dynamic speed readout helps the two active AI driver roles anticipate
   longer wet braking distances without changing their stored dry tuning.
   The two weights are deliberately moderate and bounded to avoid runaway
   reductions if the user sets very low wet-grip values. */
static float ai_wet_pace_multiplier(float wet_wheel_grip_scale, float weight)
{
    float loss, factor;
    if (!(wet_wheel_grip_scale >= 0.0f) ||
        !(wet_wheel_grip_scale <= 1.0f))
        return 1.0f;
    if (weight < 0.0f) weight = 0.0f;
    if (weight > 1.0f) weight = 1.0f;
    loss = 1.0f - wet_wheel_grip_scale;
    factor = 1.0f - loss * weight;
    if (factor < 0.75f) factor = 0.75f;
    return factor;
}

#endif
