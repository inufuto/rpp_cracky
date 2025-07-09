#include <stdint.h>
#include <memory.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "Video.h"

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

void Video::Initialize()
{
    InitializeColors();
    set_sys_clock_khz(Config::SystemClock, true);

    InitializePwmDma();
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
        p += HStartPosition;
        for (auto i = 0; i < XResolution / 2; ++i) {
            const auto& color = colors[(i >> 2) & 15];//
            *p++ = color.Values()[0];
            *p++ = color.Values()[1];
            *p++ = color.Values()[2];
            *p++ = color.Values()[3];
        }
    }
    else if (
        raster == VSyncRasterCount + BlankingRasterCount + YResolution ||
        raster == VSyncRasterCount + BlankingRasterCount + YResolution + 1
    ) {
        p += HStartPosition;
        for (auto i = 0; i < XResolution * 2; ++i) {
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
    if (++currentRaster >= RasterCount) {
        currentRaster = 0;
    }
}
