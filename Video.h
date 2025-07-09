#include "Config.h"

class Video
{
    class Color {
    private:
        uint16_t values[4];
        
        uint16_t To16(int32_t y, int32_t b_y, int32_t r_y) {
            auto s = (y * 1792 + b_y + r_y + 2 * 65536 + 32768) / 65536;
            return s < 0 ? 0 : s;
        }
    public:
        void SetRgb(uint8_t r, uint8_t g, uint8_t b);
        const auto& Values() const { return values; }
    };
private:
    static constexpr auto XResolution = 320;
    static constexpr auto YResolution = 200;
    static constexpr auto ColorCount = 16;
    static constexpr auto DmaChannelCount = 2;
    static constexpr auto SamplesPerRaster = 908; // 227 * 4
    static constexpr auto PwmCycle = Config::SystemClock * 1000 / 14318181;
    static constexpr auto RasterCount = 262;
    static constexpr auto VSyncRasterCount = 10;
    static constexpr auto BlankingRasterCount = 26 + 4;
    static constexpr auto HSyncLength = 68; // Approximately 4.7μsec
    static constexpr auto HStartPosition = HSyncLength + 128;
private:
    static Color colors[ColorCount];
    static Color backgroundColor;
    static int dmaChannels[DmaChannelCount];
    static uint16_t dmaBuffer[DmaChannelCount][SamplesPerRaster] __attribute__ ((aligned (4)));
    static volatile uint16_t currentRaster;
public:
    static void Initialize();
private:
    static void InitializeColors();
    static void InitializePwmDma();
    static void MakeDmaBuffer(uint16_t* pBuffer, uint16_t raster);
    static void Handler();
    static uint32_t InterruptBit(int channel) { return 1u << channel; }
};
