#include "gfx.hpp"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

void GFX::GFX_DrawPixel(int16_t x, int16_t y, uint8_t color) {
    ssd1306->SSD1306_DrawPixel(x, y, color);
}

void GFX::GFX_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    // Update in subclasses if desired!
    if (x0 == x1) {
        if (y0 > y1)
            _swap_int16_t(y0, y1);
            this->GFX_DrawFastVLine(x0, y0, y1 - y0 + 1, color);
        } else if (y0 == y1) {
            if (x0 > x1)
                _swap_int16_t(x0, x1);
                this->GFX_DrawFastHLine(x0, y0, x1 - x0 + 1, color);
        } else {
            this->GFX_WriteLine(x0, y0, x1, y1, color);
    }
}

void GFX::GFX_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    this->GFX_WriteLine(x, y, x, y + h - 1, color);
}

void GFX::GFX_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    this->GFX_WriteLine(x, y, x + w - 1, y, color);
}

void GFX::GFX_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    this->GFX_DrawPixel(x0, y0 + r, color);
    this->GFX_DrawPixel(x0, y0 - r, color);
    this->GFX_DrawPixel(x0 + r, y0, color);
    this->GFX_DrawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        this->GFX_DrawPixel(x0 + x, y0 + y, color);
        this->GFX_DrawPixel(x0 - x, y0 + y, color);
        this->GFX_DrawPixel(x0 + x, y0 - y, color);
        this->GFX_DrawPixel(x0 - x, y0 - y, color);
        this->GFX_DrawPixel(x0 + y, y0 + x, color);
        this->GFX_DrawPixel(x0 - y, y0 + x, color);
        this->GFX_DrawPixel(x0 + y, y0 - x, color);
        this->GFX_DrawPixel(x0 - y, y0 - x, color);
    }
}

void GFX::GFX_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    this->GFX_DrawFastVLine(x0, y0 - r, 2 * r + 1, color);
    this->GFX_DrawFilledCircleHelper(x0, y0, r, 3, 0, color);
}

void GFX::GFX_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    this->GFX_DrawFastHLine(x, y, w, color);
    this->GFX_DrawFastHLine(x, y + h - 1, w, color);
    this->GFX_DrawFastVLine(x, y, h, color);
    this->GFX_DrawFastVLine(x + w - 1, y, h, color);
}

void GFX::GFX_DrawFilledRect(int x, int y, uint16_t w, uint16_t h, uint8_t color) {
    for (int i=x; i<x+w; i++) {
    	this->GFX_DrawFastVLine(i, y, h, color);
    }
}

void GFX::GFX_DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if (r > max_radius)
    r = max_radius;
    // smarter version
    this->GFX_DrawFastHLine(x + r, y, w - 2 * r, color);         // Top
    this->GFX_DrawFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
    this->GFX_DrawFastVLine(x, y + r, h - 2 * r, color);         // Left
    this->GFX_DrawFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
    // draw four corners
    this->GFX_DrawCircleHelper(x + r, y + r, r, 1, color);
    this->GFX_DrawCircleHelper(x + w - r - 1, y + r, r, 2, color);
    this->GFX_DrawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
    this->GFX_DrawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

void GFX::GFX_DrawFilledRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if (r > max_radius)
    r = max_radius;
    // smarter version
    this->GFX_DrawFilledRect(x + r, y, w - 2 * r, h, color);
    // draw four corners
    this->GFX_DrawFilledCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
    this->GFX_DrawFilledCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

void GFX::GFX_DrawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    this->GFX_DrawLine(x0, y0, x1, y1, color);
    this->GFX_DrawLine(x1, y1, x2, y2, color);
    this->GFX_DrawLine(x2, y2, x0, y0, color);
}

void GFX::GFX_DrawFilledTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
    _swap_int16_t(y2, y1);
    _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
    _swap_int16_t(y0, y1);
    _swap_int16_t(x0, x1);
    }

    if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a)
    a = x1;
    else if (x1 > b)
    b = x1;
    if (x2 < a)
    a = x2;
    else if (x2 > b)
    b = x2;
    this->GFX_DrawFastHLine(a, y0, b - a + 1, color);
    return;
    }

    int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
    dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2)
    last = y1; // Include y1 scanline
    else
    last = y1 - 1; // Skip it

    for (y = y0; y <= last; y++) {
    a = x0 + sa / dy01;
    b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
    _swap_int16_t(a, b);
    this->GFX_DrawFastHLine(a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
    a = x1 + sa / dy12;
    b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if (a > b)
    _swap_int16_t(a, b);
    this->GFX_DrawFastHLine(a, y, b - a + 1, color);
    }
}

void GFX::GFX_DrawChar(int x, int y, char chr, uint8_t color, uint8_t background) {
    if(chr > 0x7E) return; // chr > '~'
    
    for(uint8_t i=0; i<font[1]; i++ ) // Each column (Width)
    {
        uint8_t line = (uint8_t)font[(chr-0x20) * font[1] + i + 2]; // Takie this line, (chr-0x20) = move 20 chars back,

        for(int8_t j=0; j<font[0]; j++, line >>= 1) // For each pixel in column
        {
            if(line & 1) // Check last pixel in line
            {
                if(size == 1)
                    this->GFX_DrawPixel(x+i, y+j, color); // Draw this pixel
                else
                    this->GFX_DrawFilledRect(x+i*size, y+j*size, size, size, color); // Or bigger pixel
            }
            else if(background == 0)
            {
                if(size == 1)
                    this->GFX_DrawPixel(x+i, y+j, background); // Draw black BG
                else
                    this->GFX_DrawFilledRect(x+i*size, y+j*size, size, size, background); // Or bigger
            }
        }
    }
}

void GFX::GFX_DrawString(int x, int y, char* str, uint8_t color, uint8_t background) {
    int x_tmp = x;

	char znak;
	znak = *str;

	while(*str++)
	{
		this->GFX_DrawChar(x_tmp, y, znak, color, background); // Draw current char

		x_tmp += ((uint8_t)font[1] * size) + 1; // Move X drawing pointer do char width + 1 (space)

		if(background == 0) // Draw black space if needed
		{
			for(uint8_t i=0; i<(font[0]*size); i++)
			{
				this->GFX_DrawPixel(x_tmp-1, y+i, PIXEL_BLACK);
			}
		}

		znak = *str; // Next char
	}
}


void GFX::GFX_SetFontSize(uint8_t size_t) {
    if(size_t != 0) {
        size = size_t;
    }
}

void GFX::GFX_SetFont(const uint8_t* font_t) {
    this->font = font_t;
}

void GFX::GFX_DrawImage(int x, int y, const uint8_t *img, uint8_t w, uint8_t h, uint8_t color) {
    uint8_t i, j, byteWidth = (w + 7) / 8;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            if (img[j * byteWidth + i / 8] & 128 >> (i&7)) {
                this->GFX_DrawPixel(x + i, y + j, color);
            }
        }
    }
}


/* ============ PRIVATE ============ */

void GFX::GFX_WriteLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {

    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0 <= x1; x0++) {
        if (steep) {
            this->GFX_DrawPixel(y0, x0, color);
        } else {
            this->GFX_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
        y0 += ystep;
        err += dx;
        }
    }
}

void GFX::GFX_DrawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (cornername & 0x4) {
            this->GFX_DrawPixel(x0 + x, y0 + y, color);
            this->GFX_DrawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            this->GFX_DrawPixel(x0 + x, y0 - y, color);
            this->GFX_DrawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            this->GFX_DrawPixel(x0 - y, y0 + x, color);
            this->GFX_DrawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            this->GFX_DrawPixel(x0 - y, y0 - x, color);
            this->GFX_DrawPixel(x0 - x, y0 - y, color);
        }
    }
}

void GFX::GFX_DrawFilledCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;

    delta++; // Avoid some +1's in the loop

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT drawing mode.
        if (x < (y + 1)) {
            if (corners & 1)
                this->GFX_DrawFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
            if (corners & 2)
                this->GFX_DrawFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
        }
        if (y != py) {
            if (corners & 1) {
                this->GFX_DrawFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
            }
            if (corners & 2) {
                this->GFX_DrawFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
                py = y;
            } 
        }
        px = x;
    }
}