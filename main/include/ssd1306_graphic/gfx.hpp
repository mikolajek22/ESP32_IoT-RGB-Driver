#ifndef _GFX_HPP
#define _GFX_HPP

#include "SSD1306.hpp"

#ifdef __cplusplus
extern "C" {

} // extern "C"
#endif

#ifdef __cplusplus

#define PIXEL_BLACK         SSD1306_COLOR_BLACK
#define PIXEL_WHITE         SSD1306_COLOR_WHITE
#define PIXEL_INVERSE       SSD1306_COLOR_INVERSE

#define DEFAULT_FONT_SIZE   1

class GFX{
    private:
        SSD1306 *ssd1306;
        const uint8_t* font;
        uint8_t size;

        void GFX_WriteLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
        void GFX_DrawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
        void GFX_DrawFilledCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color);

    public:
        GFX(SSD1306 *ssd1306) {
            this->ssd1306   = ssd1306;
            this->size      = DEFAULT_FONT_SIZE;
        }

        void GFX_DrawPixel(int16_t x, int16_t y, uint8_t color);
        void GFX_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
        void GFX_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
        void GFX_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
        void GFX_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
        void GFX_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
        void GFX_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
        void GFX_DrawFilledRect(int x, int y, uint16_t w, uint16_t h, uint8_t color);
        void GFX_DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
        void GFX_DrawFilledRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
        void GFX_DrawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
        void GFX_DrawFilledTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

        void GFX_DrawString(int x, int y, char* str, uint8_t color, uint8_t background);
        void GFX_DrawChar(int x, int y, char chr, uint8_t color, uint8_t background);

        void GFX_SetFontSize(uint8_t size_t);
        void GFX_SetFont(const uint8_t* font_t);
        void GFX_DrawImage(int x, int y, const uint8_t *img, uint8_t w, uint8_t h, uint8_t color);
        
};
#endif
#endif