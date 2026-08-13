#pragma once
#include <cstdint>
#include <SDL.h>

class FrameLimiter {
    public:
    explicit FrameLimiter(double target_fps = 60.0988)
         : target_frame_time_(1.0 / target_fps),
           timer_frequency_(SDL_GetPerformanceFrequency()),
           frame_start_(0){};

    void start_frame();
    void end_frame();
        
    private:
    double target_frame_time_;
    uint64_t timer_frequency_;
    uint64_t frame_start_;

};