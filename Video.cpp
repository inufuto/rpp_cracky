#include <stdint.h>
#include <memory.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "Video.h"
#include "Sound.h"

void Video::Color::SetRgb(uint8_t r, uint8_t g, uint8_t b)
{
    int32_t y = (150 * g + 29 * b + 77 * r + 128) / 256;
    int32_t b_y_1 = (b - y) * 441;
	int32_t r_y_1 = (r - y) * 1361;
	int32_t b_y_2 = (b - y) * 764;
	int32_t r_y_2 = (r - y) * (-786);

    values[0] = To16(y, b_y_1, r_y_1);
    values[1] = To16(y, b_y_2, r_y_2);
    values[2] = To16(y, -b_y_1, -r_y_1);
    values[3] = To16(y, -b_y_2, -r_y_2);
}

Video::Color Video::colors[ColorCount];
Video::Color Video::backgroundColor;
int Video::dmaChannels[DmaChannelCount];
uint16_t Video::dmaBuffer[DmaChannelCount][SamplesPerRaster] __attribute__ ((aligned (4)));
volatile uint16_t Video::currentRaster;
volatile int Video::currentY;
volatile uint8_t *Video::pTileRow;
volatile int Video::yMod;
volatile uint8_t Video::lineBuffer[XResolution];
uint8_t Video::tileMap[XTileCount * YTileCount];
uint8_t Video::tilePattern[2 * TileHeight * 256];
Video::Sprite Video::sprites[SpriteCount];
uint8_t Video::spritePattern[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x70, 0x00,
	0x00, 0x77, 0x77, 0x00, 0x00, 0x87, 0x87, 0x00,
	0x00, 0x87, 0x87, 0x00, 0x00, 0x77, 0x77, 0x00,
	0x00, 0x07, 0x70, 0x00, 0x00, 0x22, 0x22, 0x00,
	0x02, 0x22, 0x22, 0x20, 0x02, 0x02, 0x20, 0x20,
	0x06, 0x02, 0x20, 0x60, 0x00, 0x01, 0x10, 0x00,
	0x00, 0x10, 0x01, 0x00, 0x00, 0x10, 0x01, 0x00,
	0x00, 0x10, 0x01, 0x00, 0x00, 0x60, 0x06, 0x00,
    //
    0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x00,
	0x03, 0x33, 0x33, 0x00, 0x03, 0x33, 0x33, 0x00,
	0x08, 0x38, 0x33, 0x00, 0x08, 0x38, 0x33, 0x00,
	0x33, 0x33, 0x33, 0x30, 0x33, 0x33, 0x33, 0x30,
	0x03, 0x33, 0x33, 0x00, 0x03, 0x33, 0x33, 0x00,
	0x00, 0x33, 0x33, 0x00, 0x00, 0x33, 0x33, 0x00,
	0x00, 0x33, 0x30, 0x00, 0x00, 0x03, 0x30, 0x00,
	0x00, 0x03, 0x33, 0x00, 0x00, 0x00, 0x33, 0x00,
};

void Video::Initialize()
{
    InitializeColors();
    ClearSprites();
    set_sys_clock_khz(Config::SystemClock, true);
    InitializePwmDma();
}

void Video::ClearSprites()
{
    for (auto& sprite : sprites) {
        sprite.y = YResolution;
    }
}

void Video::ShowSprite(uint8_t index, uint8_t x, uint8_t y, uint8_t pattern)
{
    auto& sprite = sprites[index];
    sprite.x = x;
    sprite.y = y;
    sprite.pattern = pattern;
}

void Video::InitializeColors()
{
    for (auto i = 0; i < 8; ++i) {
		colors[i].SetRgb(255 * ((i >> 1) & 1), 255 * ((i >> 2) & 1), 255 * (i & 1));
	}
	for (auto i = 0; i < 8; ++i) {
        colors[8 + i].SetRgb(128 * (i & 1), 128 * ((i >> 1) & 1), 128 * ((i >> 2) & 1));
	}
	for (auto i = 16; i < 256 ; ++i) {
        colors[i].SetRgb(255, 255, 255);
	}
	backgroundColor.SetRgb(0, 0, 0);
}

void Video::InitializePwmDma()
{
    gpio_set_function(Config::Gpio::Video, GPIO_FUNC_PWM);
    auto pwmSlice = pwm_gpio_to_slice_num(Config::Gpio::Video);

    auto pwmConfig = pwm_get_default_config();
    pwm_config_set_clkdiv(&pwmConfig, 1);

    pwm_init(pwmSlice, &pwmConfig, true);
    pwm_set_wrap(pwmSlice, PwmCycle - 1);

    auto pOut = reinterpret_cast<io_rw_16 *>(&pwm_hw->slice[pwmSlice].cc) + 1;
    for (auto i = 0; i < DmaChannelCount; ++i)
    {
        dmaChannels[i] = dma_claim_unused_channel(true);
    }
    uint32_t mask = 0;
    for (auto i = 0; i < DmaChannelCount; ++i)
    {
        auto dmaChannelConfig = dma_channel_get_default_config(dmaChannels[i]);
        channel_config_set_transfer_data_size(&dmaChannelConfig, DMA_SIZE_16);
        channel_config_set_read_increment(&dmaChannelConfig, true);
        channel_config_set_write_increment(&dmaChannelConfig, false);
        channel_config_set_dreq(&dmaChannelConfig, DREQ_PWM_WRAP0 + pwmSlice);
        auto nextChannel = dmaChannels[(i + 1) % DmaChannelCount];
        channel_config_set_chain_to(&dmaChannelConfig, nextChannel);
        dma_channel_configure(
            dmaChannels[i],
            &dmaChannelConfig,
            pOut,
            dmaBuffer[i],
            SamplesPerRaster,
            false);
        memset(dmaBuffer[i], SamplesPerRaster * sizeof(uint16_t), 0);
        MakeDmaBuffer(dmaBuffer[i], i);
        mask |= InterruptBit(dmaChannels[i]);
    }
    dma_set_irq0_channel_mask_enabled(mask ,true);
	irq_set_exclusive_handler(DMA_IRQ_0, Handler);
	irq_set_enabled(DMA_IRQ_0, true);
    currentRaster = 0;
	dma_channel_start(dmaChannels[0]);
}

void Video::MakeDmaBuffer(uint16_t* pBuffer, uint16_t raster)
{
    auto p = pBuffer;
    if (raster < 2) {
        // VSync
        for (auto i = 0; i < SamplesPerRaster - HSyncLength; ++i) {
            *p++ = 0;
        }
        while (p < pBuffer + SamplesPerRaster) {
            *p++ = 2;
        } 
    }
    else if(raster == VSyncRasterCount || raster == VSyncRasterCount + 1) {
        // HSync & Color Burst
        for (auto i = 0; i < HSyncLength; ++i) {
            *p++ = 0;
        }
        for (auto i = 0; i < 8; ++i) {
            *p++ = 2;
        }
        for (auto i = 0; i < 9; ++i) {
            *p++ = 2;
            *p++ = 1;
            *p++ = 2;
            *p++ = 3;
        }
        while (p < pBuffer + SamplesPerRaster) {
            *p++ = 2;
        } 
    }
    else if (
        raster >= VSyncRasterCount + BlankingRasterCount &&
        raster < VSyncRasterCount + BlankingRasterCount + YResolution
    ) {
        if (raster == VSyncRasterCount + BlankingRasterCount) {
            currentY = 0;
            pTileRow = tileMap;
            yMod = 0;
        }
        {
            auto pLine = lineBuffer;
            auto pTile = pTileRow;
            for (auto tileX = 0; tileX < XTileCount; ++tileX) {
                const auto* pPattern = tilePattern + (static_cast<uint16_t>(*pTile++) << 4);
                pPattern += ((currentY & 7) << 1);
                auto b = *pPattern++;
                *pLine++ = b >> 4;
                *pLine++ = b & 0x0f;
                b = *pPattern++;
                *pLine++ = b >> 4;
                *pLine++ = b & 0x0f;
            }
        }
        {
            auto horizontalCount = 0;
            auto pSprite = sprites + SpriteCount;
            for (auto i = 0; i < SpriteCount; ++i) {
                --pSprite;
                uint8_t yOffset = currentY - pSprite->y;
                if (yOffset < SpriteHeight) {
                    uint8_t x = pSprite->x;
                    auto pPattern = spritePattern +
                        (static_cast<uint16_t>(pSprite->pattern) << 6);
                    pPattern += yOffset << 2;
                    for (auto j = 0; j < SpriteWidth / 2; ++j) {
                        auto b = *pPattern; //0x44; //
                        if (x < XResolution) {
                            auto dot = b >> 4;
                            if (dot != 0) {
                                lineBuffer[x] = dot;
                            }
                        }
                        ++x;
                        if (x < XResolution) {
                            auto dot = b & 0x0f;
                            if (dot != 0) {
                                lineBuffer[x] = dot;
                            }
                        }
                        ++x;
                        ++pPattern;
                    }
                    if (++horizontalCount >= MaxHorizontalSpriteCount) {
                        break;
                    }
                }
            }
        }
        {
            auto pLine = lineBuffer;
            p += HStartPosition;
            for (auto i = 0; i < XResolution; ++i) {
                const auto& color = colors[*pLine++];
                *p++ = color.Values()[0];
                *p++ = color.Values()[1];
                *p++ = color.Values()[2];
                *p++ = color.Values()[3];
            }
        }
        ++currentY;
        if (++yMod >= TileHeight) {
            pTileRow += XTileCount;
            yMod = 0;
        }
    }
    else if (
        raster == VSyncRasterCount + BlankingRasterCount + YResolution ||
        raster == VSyncRasterCount + BlankingRasterCount + YResolution + 1
    ) {
        p += HStartPosition;
        for (auto i = 0; i < XResolution * 4; ++i) {
            *p++ = 2;
        }
    }
}

void Video::Handler()
{
    volatile auto status = dma_hw->ints0;
    dma_hw->ints0 = status; // Clear IRQ

    for (auto i = 0; i < DmaChannelCount; ++i) {
        if ((status & InterruptBit(dmaChannels[i])) != 0) {
            MakeDmaBuffer(dmaBuffer[i], currentRaster);
		    dma_channel_set_read_addr(dmaChannels[i], dmaBuffer[i], false);
        }
    }
    if (++currentRaster == RasterCount) {
        currentRaster = 0;
        Sound::MelodyHandler();
    }
}
