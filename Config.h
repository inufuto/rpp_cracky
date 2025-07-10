#pragma once

class Config {
public:
    static constexpr auto SystemClock = 157500; // (kHz)
    class Gpio {
    public:
        static constexpr auto Video = 15;
        static constexpr auto Sound = 28;
    };
};