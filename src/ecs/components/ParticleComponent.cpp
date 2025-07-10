#include "ParticleComponent.h"
#include <algorithm>
#include <cmath>

// Helper function to interpolate between two colors
SDL_Color lerpColor(const SDL_Color& a, const SDL_Color& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return {
        static_cast<Uint8>(a.r + (b.r - a.r) * t),
        static_cast<Uint8>(a.g + (b.g - a.g) * t),
        static_cast<Uint8>(a.b + (b.b - a.b) * t),
        static_cast<Uint8>(a.a + (b.a - a.a) * t)
    };
}

// Helper function to interpolate between two floats
float lerpFloat(float a, float b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return a + (b - a) * t;
}

SDL_Color ParticleEmitterComponent::interpolateColor(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    
    // If no color curve, use simple start->end interpolation
    if (colorCurve.empty()) {
        return lerpColor(startColor, endColor, t);
    }
    
    // Find the two curve points to interpolate between
    if (colorCurve.size() == 1) {
        return colorCurve[0].color;
    }
    
    // Find the segment
    for (size_t i = 0; i < colorCurve.size() - 1; ++i) {
        if (t >= colorCurve[i].time && t <= colorCurve[i + 1].time) {
            float segmentT = (t - colorCurve[i].time) / (colorCurve[i + 1].time - colorCurve[i].time);
            return lerpColor(colorCurve[i].color, colorCurve[i + 1].color, segmentT);
        }
    }
    
    // If t is beyond the last point, return the last color
    return colorCurve.back().color;
}

float ParticleEmitterComponent::interpolateSize(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    
    // If no size curve, return constant size
    if (sizeCurve.empty()) {
        return 1.0f;
    }
    
    // Find the two curve points to interpolate between
    if (sizeCurve.size() == 1) {
        return sizeCurve[0].size;
    }
    
    // Find the segment
    for (size_t i = 0; i < sizeCurve.size() - 1; ++i) {
        if (t >= sizeCurve[i].time && t <= sizeCurve[i + 1].time) {
            float segmentT = (t - sizeCurve[i].time) / (sizeCurve[i + 1].time - sizeCurve[i].time);
            return lerpFloat(sizeCurve[i].size, sizeCurve[i + 1].size, segmentT);
        }
    }
    
    // If t is beyond the last point, return the last size
    return sizeCurve.back().size;
}

void ParticleEmitterComponent::resetEmission() {
    emissionTimer = 0.0f;
    emissionTime = 0.0f;
}
