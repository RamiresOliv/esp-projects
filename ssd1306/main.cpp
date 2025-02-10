#include <Arduino.h>
#include <Wire.h>
#include "ssd1306.h"
#include "font.h"
#include "images.h"

#define SSD1306_ADDR 0x3C  // Endereço I2C do display
#define SDA_PIN 8
#define SCL_PIN 9
unsigned char ssd1306_buffer[1024];

void ssd1306_command(unsigned char c) {
    Wire.beginTransmission(SSD1306_ADDR);
    Wire.write(0x00); // bit 7 is 0 for Co bit (data bytes only), bit 6 is 0 for DC (data is a command))
    Wire.write(c);
    Wire.endTransmission();
}

void ssd1306_clear() {
    memset(ssd1306_buffer, 0, 1024); // make every bit a 0, memset in string.h
}

void ssd1306_update() {
    ssd1306_command(SSD1306_PAGEADDR);
    ssd1306_command(0);
    ssd1306_command(0xFF);
    ssd1306_command(SSD1306_COLUMNADDR);
    ssd1306_command(0);
    ssd1306_command(SSD1306_WIDTH - 1);
    for (unsigned char page = 0; page < 8; page++) {
        ssd1306_command(0xB0 + page);    // Select the page to write to
        for (unsigned char col = 0; col < SSD1306_WIDTH; col++) {
            Wire.beginTransmission(SSD1306_ADDR);
            Wire.write(0x40);  // Send pixel data
            Wire.write(ssd1306_buffer[col + (page * SSD1306_WIDTH)]);  // Send the data byte for this column
            Wire.endTransmission();
        }
    }
}

void debugBuffer() {
    for (int i = 0; i < 1024; i++) {
        Serial.print(ssd1306_buffer[i], HEX);
        Serial.print(" ");
        if ((i + 1) % 16 == 0) {
            Serial.println();
        }
    }
}

void ssd1306_setup() {
    ssd1306_command(SSD1306_DISPLAYOFF);
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);
    ssd1306_command(0x80);
    ssd1306_command(SSD1306_SETMULTIPLEX);
    ssd1306_command(0x3F); // Multiplex ratio (pode precisar de ajustes dependendo da tela)
    ssd1306_command(SSD1306_SETDISPLAYOFFSET);
    ssd1306_command(0x0);
    ssd1306_command(SSD1306_SETSTARTLINE);
    ssd1306_command(SSD1306_CHARGEPUMP);
    ssd1306_command(0x14); // Para telas OLED de 128x64, pode ser necessário
    ssd1306_command(SSD1306_MEMORYMODE);
    ssd1306_command(0x00); // Horizontal addressing mode
    ssd1306_command(SSD1306_SEGREMAP | 0x1); // Inverter de mapeamento de segmentos socorro
    ssd1306_command(SSD1306_COMSCANDEC);
    ssd1306_command(SSD1306_SETCOMPINS);
    ssd1306_command(0x12); // Se o display for 128x64
    ssd1306_command(SSD1306_SETCONTRAST);
    ssd1306_command(0xCF); // Contraste máximo
    ssd1306_command(SSD1306_SETPRECHARGE);
    ssd1306_command(0xF1);
    ssd1306_command(SSD1306_SETVCOMDETECT);
    ssd1306_command(0x40); // VCOMH deselected voltage
    ssd1306_command(SSD1306_DISPLAYON);

    ssd1306_clear();
    ssd1306_update();
}

void ssd1306_drawPixel(unsigned char x, unsigned char y, unsigned char color) {
    if ((x < 0) || (x >= SSD1306_WIDTH) || (y < 0) || (y >= SSD1306_HEIGHT)) {
        return;
    }

    if (color == 1) {
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 7));
    }
    else {
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
    }
}

void drawString(int Px, int Py, String str) {
    int xOffeset = 0;
    int yOffeset = 0;
    for (int i = 0; i < str.length(); i++) {
        char character = str.charAt(i);

        const char* charF = ASCII[character - 0x20];

        for (int x = 0; x < 5; x++) {
            for (int j = 0; j < 7; j++) {
                ssd1306_drawPixel((x + xOffeset + Px), (j + yOffeset + Py), ((charF[x]) >> j) & 1);
            }
        }
        if ((xOffeset + 9) >= 128 || xOffeset >= 128) {
            xOffeset = 0;
            yOffeset += 8;
        }
        else {
            xOffeset += 6;
        }
    }
}

void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
    int16_t w, int16_t h, uint16_t color) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t b = 0;

    for (int16_t j = 0; j < h; j++, y++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7)
                b <<= 1;
            else
                b = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if (b & 0x80)
                ssd1306_drawPixel(x + i, y, color);
        }
    }
}

void drawCircle(int xc, int yc, int r) {
    for (float angle = 0; angle < 2 * PI; angle += 0.1) { // Incrementos pequenos para suavidade
        int x = xc + r * cos(angle);
        int y = yc + r * sin(angle);
        ssd1306_drawPixel(x, y, 1);
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);
    ssd1306_setup();

    drawBitmap(0, 22, epd_bitmap_Selection, 121, 21, 1);
    drawBitmap(124, 1, epd_bitmap_bar_handle, 121, 21, 1);
    bool jump = true;
    for (int y = 0; y < 64; y++) {
        if (!jump) {
            ssd1306_drawPixel(125, y, 1);
        }
        jump = !jump;
    }

    drawBitmap(3, 2, epd_bitmap_what, 16, 16, 1);
    drawBitmap(3, 24, epd_bitmap_pudim, 121, 21, 1);
    drawBitmap(3, 46, epd_bitmap_mistery_box, 121, 21, 1);

    drawString(25, 8, "WHAT?");
    drawString(25, 29, "PUDIM");
    drawString(25, 51, "MISTERY BOX");
    ssd1306_update();
}

void loop() {
}
