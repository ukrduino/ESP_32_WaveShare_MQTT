// mapping suggestion for ESP32, e.g. LOLIN32, see .../variants/.../pins_arduino.h for your board
// NOTE: there are variants with different pins for SPI ! CHECK SPI PINS OF YOUR BOARD
// BUSY -> 4, RST -> 16, DC -> 17, CS -> SS(5), CLK -> SCK(18), DIN -> MOSI(23), GND -> GND, 3.3V -> 3.3V

// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_3C.h>
#include <../Adafruit_GFX_Library/Fonts/FreeMono9pt7b.h>
#include <../Adafruit_GFX_Library/Fonts/FreeMonoBold9pt7b.h>



// select one and adapt to your mapping, can use full buffer size (full HEIGHT)
// 3-color e-papers
//GxEPD2_3C<GxEPD2_154c, GxEPD2_154c::HEIGHT> display(GxEPD2_154c(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
//GxEPD2_3C<GxEPD2_213c, GxEPD2_213c::HEIGHT> display(GxEPD2_213c(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
GxEPD2_3C<GxEPD2_290c, GxEPD2_290c::HEIGHT> display(GxEPD2_290c(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
//GxEPD2_3C<GxEPD2_270c, GxEPD2_270c::HEIGHT> display(GxEPD2_270c(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
//GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT> display(GxEPD2_420c(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));
//GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT> display(GxEPD2_750c(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

#include "bitmaps/Bitmaps3c128x296.h" // 2.9"  b/w/r

void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println("setup");
	display.init(115200);
	// first update should be full refresh
	helloWorld();
	delay(1000);
	// partial refresh mode can be used to full screen,
	// effective if display panel hasFastPartialUpdate
	helloFullScreenPartialMode();
	delay(1000);
	helloArduino();
	delay(1000);
	helloEpaper();
	delay(1000);
	showFont("FreeMonoBold9pt7b", &FreeMonoBold9pt7b);
	delay(1000);
	drawBitmaps();
	if (display.epd2.hasPartialUpdate)
	{
		showPartialUpdate();
		delay(1000);
	} // else // on GDEW0154Z04 only full update available, doesn't look nice
	  //drawCornerTest();
	  //showBox(16, 16, 48, 32, false);
	  //showBox(16, 56, 48, 32, true);
	display.powerOff();
	Serial.println("setup done");
}

void loop()
{
}

void helloWorld()
{
	//Serial.println("helloWorld");
	display.setRotation(1);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(GxEPD_BLACK);
	uint16_t x = (display.width() - 160) / 2;
	uint16_t y = display.height() / 2;
	display.setFullWindow();
	display.firstPage();
	do
	{
		display.fillScreen(GxEPD_WHITE);
		display.setCursor(x, y);
		display.println("Hello World!");
	} while (display.nextPage());
	//Serial.println("helloWorld done");
}

void helloFullScreenPartialMode()
{
	//Serial.println("helloFullScreenPartialMode");
	display.setPartialWindow(0, 0, display.width(), display.height());
	display.setRotation(1);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(GxEPD_BLACK);
	display.firstPage();
	do
	{
		uint16_t x = (display.width() - 160) / 2;
		uint16_t y = display.height() / 2;
		display.fillScreen(GxEPD_WHITE);
		display.setCursor(x, y);
		display.println("Hello World!");
		y = display.height() / 4;
		display.setCursor(x, y);
		display.println("full screen");
		y = display.height() * 3 / 4;
		if (display.width() <= 200) x = 0;
		display.setCursor(x, y);
		if (display.epd2.hasFastPartialUpdate)
		{
			display.println("fast partial mode");
		}
		else if (display.epd2.hasPartialUpdate)
		{
			display.println("slow partial mode");
		}
		else
		{
			display.println("no partial mode");
		}
	} while (display.nextPage());
	//Serial.println("helloFullScreenPartialMode done");
}

void helloArduino()
{
	//Serial.println("helloArduino");
	display.setRotation(1);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
	uint16_t x = (display.width() - 160) / 2;
	uint16_t y = display.height() / 4;
	display.setPartialWindow(0, y - 14, display.width(), 20);
	display.firstPage();
	do
	{
		display.fillScreen(GxEPD_WHITE);
		display.setCursor(x, y);
		display.println("Hello Arduino!");
	} while (display.nextPage());
	delay(1000);
	//Serial.println("helloArduino done");
}

void helloEpaper()
{
	//Serial.println("helloEpaper");
	display.setRotation(1);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
	uint16_t x = (display.width() - 160) / 2;
	uint16_t y = display.height() * 3 / 4;
	display.setPartialWindow(0, y - 14, display.width(), 20);
	display.firstPage();
	do
	{
		display.fillScreen(GxEPD_WHITE);
		display.setCursor(x, y);
		display.println("Hello E-Paper!");
	} while (display.nextPage());
	//Serial.println("helloEpaper done");
}

void showBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial)
{
	//Serial.println("showBox");
	display.setRotation(1);
	if (partial)
	{
		display.setPartialWindow(x, y, w, h);
	}
	else
	{
		display.setFullWindow();
	}
	display.firstPage();
	do
	{
		display.fillScreen(GxEPD_WHITE);
		display.fillRect(x, y, w, h, GxEPD_BLACK);
	} while (display.nextPage());
	//Serial.println("showBox done");
}

void drawCornerTest()
{
	display.setFullWindow();
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(GxEPD_BLACK);
	for (uint16_t r = 0; r <= 4; r++)
	{
		display.setRotation(r);
		display.firstPage();
		do
		{
			display.fillScreen(GxEPD_WHITE);
			display.fillRect(0, 0, 8, 8, GxEPD_BLACK);
			display.fillRect(display.width() - 18, 0, 16, 16, GxEPD_BLACK);
			display.fillRect(display.width() - 25, display.height() - 25, 24, 24, GxEPD_BLACK);
			display.fillRect(0, display.height() - 33, 32, 32, GxEPD_BLACK);
			display.setCursor(display.width() / 2, display.height() / 2);
			display.print(display.getRotation());
		} while (display.nextPage());
		delay(2000);
	}
}

void showFont(const char name[], const GFXfont* f)
{
	display.setFullWindow();
	display.setRotation(0);
	display.setTextColor(GxEPD_BLACK);
	display.firstPage();
	do
	{
		drawFont(name, f);
	} while (display.nextPage());
}

void drawFont(const char name[], const GFXfont* f)
{
	//display.setRotation(0);
	display.fillScreen(GxEPD_WHITE);
	display.setTextColor(GxEPD_BLACK);
	display.setFont(f);
	display.setCursor(0, 0);
	display.println();
	display.println(name);
	display.println(" !\"#$%&'()*+,-./");
	display.println("0123456789:;<=>?");
	display.println("@ABCDEFGHIJKLMNO");
	display.println("PQRSTUVWXYZ[\\]^_");
	if (display.epd2.hasColor)
	{
		display.setTextColor(GxEPD_RED);
	}
	display.println("`abcdefghijklmno");
	display.println("pqrstuvwxyz{|}~ ");
}

void showPartialUpdate()
{
	// some useful background
	helloWorld();
	// use asymmetric values for test
	uint16_t box_x = 10;
	uint16_t box_y = 15;
	uint16_t box_w = 70;
	uint16_t box_h = 20;
	uint16_t cursor_y = box_y + box_h - 6;
	float value = 13.95;
	uint16_t incr = display.epd2.hasFastPartialUpdate ? 1 : 3;
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(GxEPD_BLACK);
	// show where the update box is
	for (uint16_t r = 0; r < 4; r++)
	{
		display.setRotation(r);
		display.setPartialWindow(box_x, box_y, box_w, box_h);
		display.firstPage();
		do
		{
			display.fillRect(box_x, box_y, box_w, box_h, GxEPD_BLACK);
			//display.fillScreen(GxEPD_BLACK);
		} while (display.nextPage());
		delay(2000);
		display.firstPage();
		do
		{
			display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
		} while (display.nextPage());
		delay(1000);
	}
	//return;
	// show updates in the update box
	for (uint16_t r = 0; r < 4; r++)
	{
		display.setRotation(r);
		display.setPartialWindow(box_x, box_y, box_w, box_h);
		for (uint16_t i = 1; i <= 10; i += incr)
		{
			display.firstPage();
			do
			{
				display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
				display.setCursor(box_x, cursor_y);
				display.print(value * i, 2);
			} while (display.nextPage());
			delay(500);
		}
		delay(1000);
		display.firstPage();
		do
		{
			display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
		} while (display.nextPage());
		delay(1000);
	}
}


void drawBitmaps()
{
	display.setFullWindow();
	drawBitmaps3c128x296();
}


struct bitmap_pair
{
	const unsigned char* black;
	const unsigned char* red;
};




void drawBitmaps3c128x296()
{
	bitmap_pair bitmap_pairs[] =
	{
		//{Bitmap3c128x296_1_black, Bitmap3c128x296_1_red},
		//{Bitmap3c128x296_2_black, Bitmap3c128x296_2_red},
		{ WS_Bitmap3c128x296_black, WS_Bitmap3c128x296_red }
	};

	if (display.epd2.panel == GxEPD2::GDEW029Z10)
	{
		for (uint16_t i = 0; i < sizeof(bitmap_pairs) / sizeof(bitmap_pair); i++)
		{
			display.firstPage();
			do
			{
				display.fillScreen(GxEPD_WHITE);
				display.drawInvertedBitmap(0, 0, bitmap_pairs[i].black, display.epd2.WIDTH, display.epd2.HEIGHT, GxEPD_BLACK);
				if (bitmap_pairs[i].red == WS_Bitmap3c128x296_red)
				{
					display.drawInvertedBitmap(0, 0, bitmap_pairs[i].red, display.epd2.WIDTH, display.epd2.HEIGHT, GxEPD_RED);
				}
				else display.drawBitmap(0, 0, bitmap_pairs[i].red, display.epd2.WIDTH, display.epd2.HEIGHT, GxEPD_RED);
			} while (display.nextPage());
			delay(2000);
		}
	}
}
