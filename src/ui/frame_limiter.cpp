#include "frame_limiter.hpp"

void FrameLimiter::start_frame() {
    frame_start_ = SDL_GetPerformanceCounter();
}

void FrameLimiter::end_frame() {
    uint64_t frame_end = SDL_GetPerformanceCounter();

    double elapsed_seconds = static_cast<double>(frame_end - frame_start_) / timer_frequency_;

    if (elapsed_seconds < target_frame_time_) {
        // [TO-DO] Currently only relies on high-resolution hardware counter
        // so we either optimize this or replace this completely when we implement audio-driven sync
        while ((static_cast<double>(SDL_GetPerformanceCounter() - frame_start_) / timer_frequency_) < target_frame_time_) {
            // fine spin loop to catch remaining ms 
        }
    }
}