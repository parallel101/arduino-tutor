#pragma once

#include <Arduino.h>

struct Kalman
{
private:
    const float K0;
    const float R;
    const float Q = (K0 * K0 * R) / (1 - K0);

public:
    struct State {
        float P = 0;
        float x_low = 0;
    };

    explicit Kalman(float fs, float fc, float R = 1.0f)
        : K0(1 - exp((-2 * (float)M_PI) * (fc / fs)))
        , R(R)
    {
        assert(fc < 0.5f * fs);
    }

    void init(State &state, float z0 = 0.0f) {
        state.P = K0 * R;
        state.x_low = z0;
    }

    float highPass(State &state, float z) {
        state.P += Q;
        float K = state.P / (state.P + R);
        float r = z - state.x_low;
        state.x_low += K * r;
        state.P *= 1 - K;
        float y = z - state.x_low;
        return y;
    }

    float lowPass(State &state, float z) {
        state.P += Q;
        float K = state.P / (state.P + R);
        float r = z - state.x_low;
        state.x_low += K * r;
        state.P *= 1 - K;
        return state.x_low;
    }

    float allPass(State &state, float z) {
        (void)state;
        return z;
    }
};
