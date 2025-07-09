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
    static constexpr auto TileWidth = 4;
    static constexpr auto TileHeight = 8;
    static constexpr auto XTileCount = 40;
    static constexpr auto YTileCount = 25;
    static constexpr auto XResolution = TileWidth * XTileCount;
    static constexpr auto YResolution = TileHeight * YTileCount;
    static constexpr auto ColorCount = 16;

    static constexpr auto DmaChannelCount = 2;
    static constexpr auto SamplesPerRaster = 908; // 227 * 4
    static constexpr auto PwmCycle = Config::SystemClock * 1000 / 14318181;
    static constexpr auto RasterCount = 262;
    static constexpr auto VSyncRasterCount = 10;
    static constexpr auto BlankingRasterCount = 26 + 4;
    static constexpr auto HSyncLength = 68; // Approximately 4.7μsec
    static constexpr auto HStartPosition = HSyncLength + 120;
private:
    static Color colors[ColorCount];
    static Color backgroundColor;
    static int dmaChannels[DmaChannelCount];
    static uint16_t dmaBuffer[DmaChannelCount][SamplesPerRaster] __attribute__ ((aligned (4)));
    static volatile uint16_t currentRaster;
    static volatile int currentY;
    static volatile uint8_t *pTileRow;
    static volatile int yMod;
    static volatile uint8_t lineBuffer[];
    static uint8_t tileMap[];
    static uint8_t tilePattern[];
public:
    static void Initialize();
    static auto TileMap() { return tileMap; }
    static auto TilePattern() { return tilePattern; }
private:
    static void InitializeColors();
    static void InitializePwmDma();
    static void MakeDmaBuffer(uint16_t* pBuffer, uint16_t raster);
    static void Handler();
    static uint32_t InterruptBit(int channel) { return 1u << channel; }
};
