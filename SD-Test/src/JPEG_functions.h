#include "K32.h"
#include <JPEGDecoder.h>

/*====================================================================================
  This sketch contains support functions to render the Jpeg images.
  Created by Bodmer 15th Jan 2017
  ==================================================================================*/

// Return the minimum of two values a and b
#define minimum(a, b) (((a) < (b)) ? (a) : (b))

//====================================================================================
//   Print information decoded from the Jpeg image
//====================================================================================
void jpegInfo()
{
    LOG(F("==============="));
    LOG(F("JPEG image info"));
    LOG(F("==============="));
    LOG(F("Width      :"));
    LOG(JpegDec.width);
    LOG(F("Height     :"));
    LOG(JpegDec.height);
    LOG(F("Components :"));
    LOG(JpegDec.comps);
    LOG(F("MCU / row  :"));
    LOG(JpegDec.MCUSPerRow);
    LOG(F("MCU / col  :"));
    LOG(JpegDec.MCUSPerCol);
    LOG(F("Scan type  :"));
    LOG(JpegDec.scanType);
    LOG(F("MCU width  :"));
    LOG(JpegDec.MCUWidth);
    LOG(F("MCU height :"));
    LOG(JpegDec.MCUHeight);
    LOG(F("==============="));
}

//====================================================================================
//   Decode and render the Jpeg image onto the TFT screen
//====================================================================================
void jpegRender(int xpos, int ypos)
{

    // retrieve information about the image
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;

    uint16_t *pImg; // Pointer for the returned image block

    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
    uint32_t min_h = minimum(mcu_h, max_y % mcu_h);

    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();

    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;

    // read each MCU block until there are no more
    while (JpegDec.read())
    {

        // save a pointer to the image block
        pImg = JpegDec.pImage; // Pointer to block

        // calculate where the image block should be drawn on the screen
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // check if the image block size needs to be changed for the right and bottom edges
        if (mcu_x + mcu_w <= max_x)
            win_w = mcu_w;
        else
            win_w = min_w;
        if (mcu_y + mcu_h <= max_y)
            win_h = mcu_h;
        else
            win_h = min_h;

        // calculate how many pixels must be drawn
        uint32_t mcu_pixels = win_w * win_h;

        // draw image MCU block only if it will fit on the screen
        // if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
        // {
        //     // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
        //     tft.setAddrWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);

        //     // Write all MCU pixels to the TFT window
        //     // while (mcu_pixels--) tft.pushColor(*pImg++); // Send to TFT 16 bits at a time
        //     tft.pushColors(pImg, 0, mcu_pixels); // Send the whole buffer, this is faster
        // }

        // // Stop drawing blocks if the bottom of the screen has been reached,
        // // the abort function will close the file
        // else if ((mcu_y + win_h) >= tft.height())
        JpegDec.abort();
    }

    // calculate how long it took to draw the image
    drawTime = millis() - drawTime;

    // print the results to the serial port
    LOG("Total render time was    : ");
    LOG(drawTime);
    LOG(" ms");
    LOG("=====================================");
}

//====================================================================================
//   Opens the image file and prime the Jpeg decoder
//====================================================================================
void drawJpeg(const char *filename, int xpos, int ypos)
{

    // Open the named file (the Jpeg decoder library will close it)
    File jpegFile = SD.open(filename, FILE_READ); // or, file handle reference for SD library

    if (!jpegFile)
    {
        LOG("ERROR: File = ");
        LOG(filename);
        LOG(" not found!");
        return;
    }

    LOG("===========================");
    LOG("Drawing file: ");
    LOG(filename);
    LOG("===========================");

    // Use one of the following methods to initialise the decoder:
    //boolean decoded = JpegDec.decodeSdFile(jpegFile); // Pass the SD file handle to the decoder,
    boolean decoded = JpegDec.decodeSdFile(filename); // or pass the filename (String or character array)

    if (decoded)
    {
        // print information about the image to the serial port
        jpegInfo();
        // render the image onto the screen at given coordinates
        jpegRender(xpos, ypos);
    }
    else
    {
        LOG("Jpeg file format not supported!");
    }
}

//====================================================================================
//   Open a Jpeg file and send it to the Serial port in a C array compatible format
//====================================================================================
void createArray(const char *filename)
{

    // Open the named file
    //fs::File jpgFile = SPIFFS.open( filename, "r");    // File handle reference for SPIFFS
    File jpgFile = SD.open(filename, FILE_READ); // or, file handle reference for SD library

    if (!jpgFile)
    {
        LOG(" ERROR: File ");
        LOG(filename);
        LOG(" not found!");
        return;
    }

    uint8_t data;
    byte line_len = 0;
    LOG(" ");
    LOG("// Generated by a JPEGDecoder library example sketch:");
    LOG("// https://github.com/Bodmer/JPEGDecoder");
    LOG(" ");
    LOG("#if defined(__AVR__)");
    LOG("  #include <avr/pgmspace.h>");
    LOG("#endif");
    LOG(" ");
    LOG("const uint8_t ");
    while (*filename != '.')
        LOG(*filename++);
    LOG("[] PROGMEM = {"); // PROGMEM added for AVR processors, it is ignored by Due

    while (jpgFile.available())
    {

        data = jpgFile.read();
        LOG("0x");
        if (abs(data) < 16)
            LOG("0");
        LOGHEX(data);
        LOG(","); // Add value and comma
        line_len++;
        if (line_len >= 32)
        {
            line_len = 0;
            LOG();
        }
    }

    LOG("};\r\n");
    jpgFile.close();
}
//====================================================================================

void displayJpegMatrix(const char *path)
{
    File jpgFile = SD.open(path, FILE_READ);
    if (JpegDec.decodeSdFile(path))
    {
        uint32_t mcuPixels = JpegDec.MCUWidth * JpegDec.MCUHeight;
        uint8_t row = 0;
        uint8_t col = 0;

        while (JpegDec.read())
        {
            uint16_t *pImg = JpegDec.pImage;
            for (uint8_t i = 0; i < mcuPixels; i++)
            {
                // Extract the red, green, blue values from each pixel
                uint8_t b = uint8_t((*pImg & 0x001F) << 3);   // 5 LSB for blue
                uint8_t g = uint8_t((*pImg & 0x07C0) >> 3);   // 6 'middle' bits for green
                uint8_t r = uint8_t((*pImg++ & 0xF800) >> 8); // 5 MSB for red
                // Calculate the matrix index (column and row)
                col = JpegDec.MCUx * 8 + i % 8;
                row = JpegDec.MCUy * 8 + i / 8;

                LOGF("i = %d\n", i);
                LOGF("r = %d\n", r);
                LOGF("g = %d\n", g);
                LOGF("b = %d\n", b);
                // Set the matrix pixel to the RGB value
                // pixels.setPixelColor(remap[row][col], pixels.Color(r,g,b));
            }
        }
        // pixels.show();
    }
}
