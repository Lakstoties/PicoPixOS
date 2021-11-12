#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812/generated/ws2812.pio.h"
#include "pico/multicore.h"

//
//  DEFAULTS
//
uint NUMBER_OF_PIXELS = 24;


//
//  Version String to Tag things and Check Against
//
const char OSVersionString[17] = "Pico Pix OS V1.0";

//
//  Utility Functions
//
void StringCopy(char* target, const char* source)
{
    int i = 0;
    while (source[i] != 0)
    {
        target[i] = source[i];
        i++;
    }
    target[i] = 0;
}

int StringLength(const char* source)
{
    int i = 0;
    while (source[i] != 0)
    {
        i++;
    }

    return i;
}

//
//  Master Settings Struct
//
struct SystemSettings
{
    char SaveVersionString[17];
    PIO pio;
    int stateMachine;
    uint LEDPin;
    uint pixelBufferSize;

    bool ProgramRunning;
};

//
//  Stored and Current Settings
//
struct SystemSettings* StoredSettings;
struct SystemSettings CurrentSettings;

void CheckLoadSettings()
{
    bool findValidSettings = false;

    if (findValidSettings)
    {
        //Load the settings
    }
    else
    {
        //Load the defaults
        StringCopy(CurrentSettings.SaveVersionString, OSVersionString);
        CurrentSettings.LEDPin = 0;
        CurrentSettings.pio = pio0;
        CurrentSettings.stateMachine = 0;
        CurrentSettings.pixelBufferSize = NUMBER_OF_PIXELS;

        CurrentSettings.ProgramRunning = false;
    }
}

//
//  Serial Interface Settings Struct
//
struct SerialInterfaceSettings
{
    //
    //  Menu Controls
    //
    int menuSelection;
    bool screenActive;

    //
    //  Drawing Controls
    //
    bool updateBackground;
    bool updateMenuFrames;
    bool updateMenuChoice;
    bool updateMenuScreen;

};

struct SerialInterfaceSettings CurrentSerial;

void InitSerialSettings()
{
    CurrentSerial.menuSelection = 0;
    CurrentSerial.screenActive = false;

    CurrentSerial.updateBackground = true;
    CurrentSerial.updateMenuFrames = true;
    CurrentSerial.updateMenuChoice = true;
    CurrentSerial.updateMenuScreen = true;

}

//
//
//  Serial Interface Constants
//
//

//
//  Terminal Attributes
//
const int TERMINAL_WIDTH = 80;
const int TERMINAL_HEIGHT = 24;

//
//  Background Attributes
//
const int BACKGROUND_BORDER_BOX_ROW_START = 1;
const int BACKGROUND_BORDER_BOX_COLUMN_START = 1;
const int BACKGROUND_BORDER_BOX_WIDTH = TERMINAL_WIDTH;
const int BACKGROUND_BORDER_BOX_HEIGHT = TERMINAL_HEIGHT;

//
//  Menu Attributes
//

//  Static
const int NUMBER_OF_MENU_ITEMS = 6;
const int MENU_BUTTON_COLUMN_START = 3;
const int MENU_BUTTON_WIDTH = 8;
const int MENU_BUTTON_HEIGHT = 3;
const int MENU_BUTTON_BORDER_ROW_START = 2;
const int MENU_BUTTON_BORDER_COLUMN_START = 2;
const int MENU_SCREEN_BORDER_ROW_START = 2;

//  Derived
const int MENU_BUTTON_ROW_START = MENU_BUTTON_BORDER_ROW_START + 1;
const int MENU_SCREEN_ROW_START = MENU_SCREEN_BORDER_ROW_START + 1;
const int MENU_BUTTON_BORDER_WIDTH = MENU_BUTTON_WIDTH + 2;
const int MENU_BUTTON_BORDER_HEIGHT = NUMBER_OF_MENU_ITEMS * MENU_BUTTON_HEIGHT + 2;
const int MENU_SCREEN_BORDER_COLUMN_START = MENU_BUTTON_BORDER_COLUMN_START + MENU_BUTTON_BORDER_WIDTH;
const int MENU_SCREEN_COLUMN_START = MENU_SCREEN_BORDER_COLUMN_START + 1;
const int MENU_SCREEN_BORDER_WIDTH = TERMINAL_WIDTH - MENU_SCREEN_BORDER_COLUMN_START;
const int MENU_SCREEN_BORDER_HEIGHT = TERMINAL_HEIGHT - MENU_SCREEN_BORDER_ROW_START;
const int MENU_SCREEN_WIDTH = TERMINAL_WIDTH - MENU_SCREEN_COLUMN_START - 1;
const int MENU_SCREEN_HEIGHT = TERMINAL_HEIGHT - MENU_SCREEN_ROW_START - 1;






//
//
//  Pixel Driving Functions
//
//

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t red, uint8_t green, uint8_t blue)
{
    return  ((uint32_t) (red) << 8)     |
            ((uint32_t) (green) << 16)  |
            ((uint32_t) (blue));
}

static inline void GRBtoColors(uint32_t GRBValue, uint8_t* red, uint8_t* green, uint8_t* blue)
{
    (*red) =     (GRBValue & 0x00FF00) >> 8;
    (*green) =   (GRBValue & 0xFF0000) >> 16;
    (*blue) =    (GRBValue & 0x0000FF);
}

struct PixelBufferStruct
{
    int* data;
    int size;
};

//  Pointer the pixel buffer
struct PixelBufferStruct CurrentPixelBuffer;

//  Effects
enum Effects
{
    RANDOM = 0
};

//  Current Effect Mode
int currentEffect = RANDOM;

//  Status Flags
bool PauseEffect = false;


int CurrentBrightnessMask = 0x07;
int CurrentBrightLevel = 5;


//
//
//  Pixel Program Stuff
//
//

void StartPIOPixelProgram ()
{
    //  Get the GPIO ready on the target pin
    gpio_init(CurrentSettings.LEDPin);

    //  Make the GPIO an output
    gpio_set_dir(CurrentSettings.LEDPin, GPIO_OUT);

    //  Get the program offset that we load up into the pio
    uint offset = pio_add_program(CurrentSettings.pio, &ws2812_program);

    // Fire off the state machine for the ws2812 program
    ws2812_program_init(CurrentSettings.pio, CurrentSettings.stateMachine, offset, CurrentSettings.LEDPin, 800000, false);

    //  Flag that the program is running
    CurrentSettings.ProgramRunning = true;
}

void StopPIOPixelProgram ()
{
    pio_sm_set_enabled(CurrentSettings.pio, CurrentSettings.stateMachine, false);
    CurrentSettings.ProgramRunning = false;
}





//
//
//  Color Stuff
//
//

struct RGBValues
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

const struct RGBValues COLOR_4BIT[16] =
        {
                {00,00,00},     //Black
                {192,00,00},    //Red
                {00,192,00},    //Green
                {192,192,00},   //Yellow
                {00,00,192},    //Blue
                {192,00,192},   //Magenta
                {00,192,192},   //Cyan
                {192,192,192},  //White
                {64,64,64},     //Bright Black
                {192,64,64},    //Bright Red
                {64,192,64},    //Bright Green
                {192,192,64},   //Bright Yellow
                {64,64,192},    //Bright Blue
                {192,64,192},   //Bright Magenta
                {64,192,192},   //Bright Cyan
                {192,192,192},  //Bright White
        };

const struct RGBValues MainMenuForeground = {64,192,64};
const struct RGBValues MainMenuBackground = {16,32,16};
const struct RGBValues BorderActiveForeground = {255,255,255};
const struct RGBValues BorderActiveBackground = {32,32,64};
const struct RGBValues BorderInactiveForeground = {64,64,64};
const struct RGBValues BorderInactiveBackground = {16,16,32};

//
//
//  Terminal Control Commands
//
//

void inline SetForegroundColor(char red, char green, char blue)
{
    printf("\x1b[38;2;%u;%u;%um", red, green, blue);
}

void inline SetForegroundValues (struct RGBValues color)
{
    SetForegroundColor(color.red, color.green, color.blue);
}

void inline SetBackgroundColor(char red, char green, char blue)
{
    printf("\x1b[48;2;%u;%u;%um", red, green, blue);
}

void inline SetBackgroundValues(struct RGBValues color)
{
    SetBackgroundColor(color.red, color.green, color.blue);
}

void inline SetCursorPosition(unsigned int row, unsigned int column)
{
    printf("\x1b[%u;%uH", row, column);
}

void inline TurnCursorOff()
{
    printf("\x1b[?25l");
}

void inline TurnCursorOn()
{
    printf("\x1b[?25h");
}

void inline MoveCursorUp(unsigned int spaces)
{
    printf("\x1b[%uA", spaces);
}

void inline MoveCursorDown(unsigned int spaces)
{
    printf("\x1b[%uB", spaces);
}

void inline MoveCursorForward(unsigned int spaces)
{
    printf("\x1b[%uC", spaces);
}

void inline MoveCursorBack(unsigned int spaces)
{
    printf("\x1b[%uD", spaces);
}

void inline ClearScreen()
{
    printf("\x1b[2J");
}

void inline ResetDisplayAttributes()
{
    printf("\x1b[0m");
}

//
//
//  Terminal Drawing Commands
//
//

void inline DrawRow     (unsigned int row, unsigned int column, unsigned int width)
{
    SetCursorPosition(row, column);
    for (int i = 0; i < width; i++)
    {
        printf("═");
    }
}

void inline DrawColumn  (unsigned int row, unsigned int column, unsigned int height)
{
    SetCursorPosition(row, column);
    for (int i = 0; i < height; i++)
    {
        printf("║");
        SetCursorPosition(row + i,column);
    }
}

void inline DrawBox     (unsigned int row, unsigned int column, unsigned int width, unsigned int height)
{
    DrawRow(row, column+1, width-2);
    DrawRow(row+height-1, column+1, width-2);
    DrawColumn(row+1, column, height-1);
    DrawColumn(row+1, column+width-1, height-1);

    SetCursorPosition(row, column);
    printf("╔");

    SetCursorPosition(row, column+width-1);
    printf("╗");

    SetCursorPosition(row+height-1, column);
    printf("╚");

    SetCursorPosition(row+height-1, column+width-1);
    printf("╝");

}

void inline DrawFill    (unsigned int row, unsigned int column, unsigned int width, unsigned int height)
{
    //  Allocate a buffer
    char* tempBuffer = malloc(sizeof(char)*width + 1);

    //  Load up the buffer with spaces for writing to the screen
    for (int i = 0; i < width; i++)
    {
        tempBuffer[i] = ' ';
    }
    tempBuffer[width] = 0;

    //  Write the stuff out
    for (int i = 0; i < height; i++)
    {
        //  Move cursor into position
        SetCursorPosition(row + i, column);
        printf(tempBuffer);
    }

    //  Release the created buffer
    free(tempBuffer);
}

void inline DrawButton  (unsigned int row, unsigned int column, unsigned int width, unsigned int height, const char* text, bool select,
                         char redFore, char greenFore, char blueFore,
                         char redBack, char greenBack, char blueBack)
{
    //Length of the string
    int length = 0;

    //  Set the colors up
    //  Set Foreground Color
    SetForegroundColor(select ? redBack     : redFore,
                       select ? greenBack   : greenFore,
                       select ? blueBack    : blueFore);

    //  Set Background Color
    SetBackgroundColor(select ? redFore     : redBack,
                       select ? greenFore   : greenBack,
                       select ? blueFore    : blueBack);

    //  Draw Button Border
    DrawBox(row, column, width, height);

    //  Draw Fill
    DrawFill(row+1, column+1, width-2, height-2);

    //  Figure out the string length
    length = StringLength(text);

    //  Calculate how to center the text
    SetCursorPosition(row + 1 + (height - 2) / 2,
                      column + 1  + (width - 2 - length)/ 2);

    //  Write the text
    printf(text);
}

//
//
//  Print Different Objects
//
//

void PrintPixelBufferStatus (int row, int column, int width, int height, int startIndex)
{
    //  Temp Variables for color data
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    //  Variables to figure out how to arrange the pixel data within given height and width.
    //  Given the number of pixels in the buffer and the given width, how many rows are we going to have?
    int dataRows = 1 + (CurrentSettings.pixelBufferSize * 2) / width;

    //  If dataRows is larger than height, default to height
    if (dataRows > height)
    {
        dataRows = height;
    }

    //  Set the current row offset
    int currentRow = 0;

    //  For each row
    while (currentRow < dataRows)
    {
        //  Move cursor into position
        SetCursorPosition(row + currentRow, column);

        //  Print out all the columns
        for (int i = 0; i < (width / 2) && i < CurrentSettings.pixelBufferSize; i++)
        {
            GRBtoColors(CurrentPixelBuffer.data[(startIndex + i + (currentRow * width)) % CurrentSettings.pixelBufferSize], &red, &green, &blue);
            SetForegroundColor((red << CurrentBrightLevel) ^ 0xFF, (green << CurrentBrightLevel) ^ 0xFF, (blue << CurrentBrightLevel) ^ 0xFF);
            SetBackgroundColor(red << CurrentBrightLevel , green << CurrentBrightLevel, blue << CurrentBrightLevel);
            printf("%02i",i);
        }

        //  Move to the next row
        currentRow++;
    }
}

void DrawMainLogo(int row, int column, int colorRotateIndex)
{
    char logoString[17] = "Pico Pix OS V1.0";

    //Move the cursor to position
    SetCursorPosition(row, column);

    //Set Background Color
    SetBackgroundColor(0,0,0);

    //Set Foreground Color
    for (int i = 0; i < 16; i++)
    {
        SetForegroundColor(COLOR_4BIT[(i+colorRotateIndex)%16].red, COLOR_4BIT[(i+colorRotateIndex)%16].green, COLOR_4BIT[(i+colorRotateIndex)%16].blue);
        printf("%c", logoString[i]);
    }
};


void ClearMenuScreenArea()
{
    char blankBuffer[MENU_SCREEN_WIDTH + 1];

    for (int i = 0; i < MENU_SCREEN_WIDTH; i++)
    {
        blankBuffer[i] = ' ';
    }
    blankBuffer[MENU_SCREEN_WIDTH] = 0;

    SetBackgroundColor(0,0,0);

    for (int i = 0; i < MENU_SCREEN_HEIGHT; i++)
    {
        SetCursorPosition(MENU_SCREEN_ROW_START + i, MENU_SCREEN_COLUMN_START);
        printf(blankBuffer);
    }
}

void DrawBackground()
{
    //Draw Outer Border Box
    DrawBox(BACKGROUND_BORDER_BOX_ROW_START,BACKGROUND_BORDER_BOX_COLUMN_START,BACKGROUND_BORDER_BOX_WIDTH,BACKGROUND_BORDER_BOX_HEIGHT);
}

void DrawMenuWindows()
{
    //
    //  Draw Window around Menu Buttons
    //

    //  Set Foreground and Background colors based on whether the screen is active (tabbed into)

    //  Menu Buttons Border is Inactive
    if (CurrentSerial.screenActive)
    {
        SetForegroundColor(BorderInactiveForeground.red, BorderInactiveForeground.green, BorderInactiveForeground.blue);
        SetBackgroundColor(BorderInactiveBackground.red, BorderInactiveBackground.green, BorderInactiveBackground.blue);
    }
    else
    {
        SetForegroundColor(BorderActiveForeground.red, BorderActiveForeground.green, BorderActiveForeground.blue);
        SetBackgroundColor(BorderActiveBackground.red, BorderActiveBackground.green, BorderActiveBackground.blue);
    }

    //
    //  Draw Window around Menu Screen
    //
    DrawBox(MENU_BUTTON_BORDER_ROW_START,MENU_BUTTON_BORDER_COLUMN_START,MENU_BUTTON_BORDER_WIDTH,MENU_BUTTON_BORDER_HEIGHT);


    //  Menu Buttons Border is Active
    if (!CurrentSerial.screenActive)
    {
        SetForegroundColor(BorderInactiveForeground.red, BorderInactiveForeground.green, BorderInactiveForeground.blue);
        SetBackgroundColor(BorderInactiveBackground.red, BorderInactiveBackground.green, BorderInactiveBackground.blue);
    }
    else
    {
        SetForegroundColor(BorderActiveForeground.red, BorderActiveForeground.green, BorderActiveForeground.blue);
        SetBackgroundColor(BorderActiveBackground.red, BorderActiveBackground.green, BorderActiveBackground.blue);
    }

    //
    //  Draw Window around Menu Screen
    //
    DrawBox(MENU_SCREEN_BORDER_ROW_START, MENU_SCREEN_BORDER_COLUMN_START, MENU_SCREEN_BORDER_WIDTH, MENU_SCREEN_BORDER_HEIGHT);
}


//
//  Section menu options
//
//
void DrawMenuButtons()
{
    //  Config Button
    DrawButton(MENU_BUTTON_ROW_START,MENU_BUTTON_COLUMN_START,MENU_BUTTON_WIDTH,MENU_BUTTON_HEIGHT,"Config", (CurrentSerial.menuSelection == 0) ? 1 : 0,
               MainMenuForeground.red, MainMenuForeground.green, MainMenuForeground.blue,
               MainMenuBackground.red, MainMenuBackground.green, MainMenuBackground.blue);

    //  Effect Button
    DrawButton(MENU_BUTTON_ROW_START + 3,MENU_BUTTON_COLUMN_START,MENU_BUTTON_WIDTH,MENU_BUTTON_HEIGHT,"Effect", (CurrentSerial.menuSelection == 1) ? 1 : 0,
               MainMenuForeground.red, MainMenuForeground.green, MainMenuForeground.blue,
               MainMenuBackground.red, MainMenuBackground.green, MainMenuBackground.blue);
    //  Inputs Button
    DrawButton(MENU_BUTTON_ROW_START + 6,MENU_BUTTON_COLUMN_START,MENU_BUTTON_WIDTH,MENU_BUTTON_HEIGHT,"Inputs", (CurrentSerial.menuSelection == 2) ? 1 : 0,
               MainMenuForeground.red, MainMenuForeground.green, MainMenuForeground.blue,
               MainMenuBackground.red, MainMenuBackground.green, MainMenuBackground.blue);

    //  Status Button
    DrawButton(MENU_BUTTON_ROW_START + 9,MENU_BUTTON_COLUMN_START,MENU_BUTTON_WIDTH,MENU_BUTTON_HEIGHT,"Status", (CurrentSerial.menuSelection == 3) ? 1 : 0,
               MainMenuForeground.red, MainMenuForeground.green, MainMenuForeground.blue,
               MainMenuBackground.red, MainMenuBackground.green, MainMenuBackground.blue);

    //  Commit Button
    DrawButton(MENU_BUTTON_ROW_START + 12,MENU_BUTTON_COLUMN_START,MENU_BUTTON_WIDTH,MENU_BUTTON_HEIGHT,"Commit", (CurrentSerial.menuSelection == 4) ? 1 : 0,
               MainMenuForeground.red, MainMenuForeground.green, MainMenuForeground.blue,
               MainMenuBackground.red, MainMenuBackground.green, MainMenuBackground.blue);

    //  Help Button
    DrawButton(MENU_BUTTON_ROW_START + 15,MENU_BUTTON_COLUMN_START,MENU_BUTTON_WIDTH,MENU_BUTTON_HEIGHT," Help ", (CurrentSerial.menuSelection == 5) ? 1 : 0,
               MainMenuForeground.red, MainMenuForeground.green, MainMenuForeground.blue,
               MainMenuBackground.red, MainMenuBackground.green, MainMenuBackground.blue);


}

//
//  Items to use with config screens
//

//
//  Draw Raspberry Pi Pico Layout
//
void DrawPicoLayout(int row, int column)
{
    //
    //  Colors for things
    //
    struct RGBValues PicoBoardColor = {0,64,0};
    struct RGBValues GPIOPinColor = {96,96,0};
    struct RGBValues GNDPinColor = {0,0,0};
    struct RGBValues PowerPinColor = {128,0,0};
    struct RGBValues SelectionBackground = {32,192,32};
    struct RGBValues SelectionForeground = {255,255,255};
    struct RGBValues DefaultForeground = {192,192,192};
    struct RGBValues VREFPinColor = {192,64,0};
    struct RGBValues ResetPinColor = {64,64,64};

    //
    //  Row 1
    //

    //  Draw GPIO Pin 00
    SetCursorPosition(row, column);
    if (CurrentSettings.LEDPin == 0)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP00");

    //  Draw Row 1 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┬01─O██O─40┬");

    //  Draw VBUS Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PowerPinColor);
    printf ("VBUS");


    //
    //  Row 2
    //

    //  Draw GPIO Pin 01
    SetCursorPosition(row + 1, column);
    if (CurrentSettings.LEDPin == 1)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP01");

    //  Draw Row 2 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤02      39├");

    //  Draw VSYS Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PowerPinColor);
    printf ("VSYS");

    //
    //  Row 3
    //

    //  Draw GND Pin
    SetCursorPosition(row + 2, column);
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(GNDPinColor);
    printf ("GND ");

    //  Draw Row 3 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤03      38├");

    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(GNDPinColor);
    printf ("GND ");

    //
    // Row 4
    //

    //  Draw GPIO Pin 02
    SetCursorPosition(row + 3, column);
    if (CurrentSettings.LEDPin == 2)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP02");

    //  Draw Row 4 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤04      37├");

    //  Draw 3V3_Enable Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PowerPinColor);
    printf ("3V3E");

    //
    // Row 5
    //

    //  Draw GPIO Pin 03
    SetCursorPosition(row + 4, column);
    if (CurrentSettings.LEDPin == 3)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP03");

    //  Draw Row 5 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤05      36├");

    //  Draw 3V3 Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PowerPinColor);
    printf ("3V3 ");

    //
    //  Row 6
    //

    //  Draw GPIO Pin 04
    SetCursorPosition(row + 5, column);
    if (CurrentSettings.LEDPin == 4)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP04");

    //  Draw Row 6 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤06      35├");

    //  Draw VREF pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(VREFPinColor);
    printf ("VREF");

    //
    //  Row 7
    //

    //  Draw GPIO Pin 05
    SetCursorPosition(row + 6, column);
    if (CurrentSettings.LEDPin == 5)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP05");

    //  Draw Row 7 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤07      34├");

    if (CurrentSettings.LEDPin == 28)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP28");

    //
    //  Row 8
    //
    SetCursorPosition(row + 7, column);

    //  Draw GND Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(GNDPinColor);
    printf ("GND ");

    //  Draw Row 8 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤08      33├");

    //  Draw GND Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(GNDPinColor);
    printf ("GND ");

    //
    //  Row 9
    //

    //  Draw GPIO Pin 6
    SetCursorPosition(row + 8, column);
    if (CurrentSettings.LEDPin == 6)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP06");

    //  Draw Row 9 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤09      32├");

    //  Draw GPIO Pin 27
    if (CurrentSettings.LEDPin == 27)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP27");

    //
    //  Row 10
    //

    //  Draw GPIO Pin 07
    SetCursorPosition(row + 9, column);
    if (CurrentSettings.LEDPin == 7)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP07");

    //  Draw Row 10 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤10      31├");

    //  Draw GPIO Pin 26
    if (CurrentSettings.LEDPin == 26)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP26");

    //
    //  Row 11
    //

    //  Draw GPIO Pin 08
    SetCursorPosition(row + 10, column);
    if (CurrentSettings.LEDPin == 8)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP08");

    //  Draw Row 11 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤11      30├");

    //  Draw Run/Reset Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(ResetPinColor);
    printf ("RRST");

    //
    //  Row 12
    //

    //  Draw GPIO Pin 09
    SetCursorPosition(row + 11, column);
    if (CurrentSettings.LEDPin == 9)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP09");

    //  Draw Row 12 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤12      29├");

    //  Drawe GPIO Pin 22
    if (CurrentSettings.LEDPin == 22)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP22");

    //
    //  Row 13
    //
    SetCursorPosition(row + 12, column);

    //  Draw GND Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(GNDPinColor);
    printf ("GND ");

    //  Draw Row 13 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤13      28├");

    //  Draw GND Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(GNDPinColor);
    printf ("GND ");

    //
    //  Row 14
    //
    SetCursorPosition(row + 13, column);

    //  Draw GPIO Pin 10
    if (CurrentSettings.LEDPin == 10)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP10");

    //  Draw Row 14 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤14      27├");

    if (CurrentSettings.LEDPin == 21)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP21");

    //
    //  Row 15
    //
    SetCursorPosition(row + 14, column);

    //  Draw GPIO Pin 11
    if (CurrentSettings.LEDPin == 11)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP11");

    //  Draw Row 15 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤15      26├");

    //  Draw GPIO Pin 20
    if (CurrentSettings.LEDPin == 20)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP20");

    //
    //  Row 16
    //
    SetCursorPosition(row + 15, column);

    //  Draw GPIO Pin 12
    if (CurrentSettings.LEDPin == 12)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP12");

    //  Draw Row 16 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤16      25├");

    //  Draw GPIO Pin 19
    if (CurrentSettings.LEDPin == 19)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP19");

    //
    //  Row 17
    //
    SetCursorPosition(row + 16, column);

    //  Draw GPIO Pin 13
    if (CurrentSettings.LEDPin == 13)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP13");

    //  Draw Row 17 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤17      25├");


    //  Draw GPIO Pin 18
    if (CurrentSettings.LEDPin == 18)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP18");

    //
    //  Row 18
    //
    SetCursorPosition(row + 17, column);

    //  Draw GND Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(GNDPinColor);
    printf ("GND ");

    //  Draw Row 18 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤18      23├");

    //  Draw GND Pin
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(GNDPinColor);
    printf ("GND ");

    //
    //  Row 19
    //
    SetCursorPosition(row + 18, column);

    //  Draw GPIO Pin 14
    if (CurrentSettings.LEDPin == 14)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP14");

    //  Draw Row 19 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┤19      22├");

    if (CurrentSettings.LEDPin == 17)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP17");

    //
    //  Row 20
    //
    SetCursorPosition(row + 19, column);

    //  Draw Row 20 of chip
    if (CurrentSettings.LEDPin == 15)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP15");

    //  Draw Row 20 of chip
    SetForegroundValues(DefaultForeground);
    SetBackgroundValues(PicoBoardColor);
    printf ("┴20─o──o─21┴");

    if (CurrentSettings.LEDPin == 16)
    {
        SetForegroundValues(SelectionForeground);
        SetBackgroundValues(SelectionBackground);
    }
    else
    {
        SetForegroundValues(DefaultForeground);
        SetBackgroundValues(GPIOPinColor);
    }
    printf ("GP16");

}


//
//  Draw Config Screen
//
void DrawConfigScreen()
{
    // Locate to the right corner of the screen
    SetCursorPosition(MENU_SCREEN_ROW_START, MENU_SCREEN_COLUMN_START);

    //  Draw Pico Chip
    DrawPicoLayout(MENU_SCREEN_ROW_START, MENU_SCREEN_COLUMN_START);





    //  Pin to use config option

    // Number of pixels


}

//
//  Draw Effect Screen
//
void DrawEffectScreen()
{
    // Locate to the right corner of the screen
    SetCursorPosition(MENU_SCREEN_ROW_START, MENU_SCREEN_COLUMN_START);

    //  Write Stuff
    printf("Temporary Effect Screen Text!");
}

//
//  Draw Inputs Screen
//
void DrawInputsScreen()
{
    // Locate to the right corner of the screen
    SetCursorPosition(MENU_SCREEN_ROW_START, MENU_SCREEN_COLUMN_START);

    //  Write Stuff
    printf("Temporary Inputs Screen Text!");
}

//
//  Draw Status Screen
//
void DrawStatusScreen()
{
    // Locate to the right corner of the screen
    SetCursorPosition(MENU_SCREEN_ROW_START, MENU_SCREEN_COLUMN_START);

    //  Write Stuff
    printf("Pixel Buffer Status");
}

//
//  Draw Status Screen Active Monitor
//
void DrawStatusScreenActive()
{
    PrintPixelBufferStatus(MENU_SCREEN_ROW_START + 1, MENU_SCREEN_COLUMN_START, MENU_SCREEN_WIDTH, MENU_SCREEN_HEIGHT, 0);
}


//
//  Draw Commit Screen
//
void DrawCommitScreen()
{
    // Locate to the right corner of the screen
    SetCursorPosition(MENU_SCREEN_ROW_START, MENU_SCREEN_COLUMN_START);

    //  Write Stuff
    printf("Temporary Commit Screen Text!");
}

//
//  Draw Help Screen
//
void DrawHelpScreen()
{
    SetForegroundColor(255,255,255);
    SetBackgroundColor(0,0,0);

    // Locate to the right corner of the screen and write
    SetCursorPosition(MENU_SCREEN_ROW_START, MENU_SCREEN_COLUMN_START);
    printf("Help Screen");

    SetCursorPosition(MENU_SCREEN_ROW_START + 1, MENU_SCREEN_COLUMN_START);
    printf ("Arrow Keys - Move selection.");

    SetCursorPosition(MENU_SCREEN_ROW_START + 2, MENU_SCREEN_COLUMN_START);
    printf ("Tab - Switch between Menu and Screen.");

    SetCursorPosition(MENU_SCREEN_ROW_START + 3, MENU_SCREEN_COLUMN_START);
    printf("P - Pause Effect.");

    SetCursorPosition(MENU_SCREEN_ROW_START + 4, MENU_SCREEN_COLUMN_START);
    printf("R - Redraw Screen entirely.");
}



//
//
//  Core Functions
//
//
void ProcessInput()
{
    char input = getchar_timeout_us(0);

    switch (input)
    {
        //
        //  Process Escape Code
        //
        case '\x1b':
            //
            //  Check to see if there's a '[' following for a proper escape sequence
            //
            if (getchar_timeout_us(0) != '[')
            {
                return;
            }

            input = getchar_timeout_us(0);
            switch (input)
            {
                //  Process Up
                case 'A':
                    if (CurrentSerial.screenActive)
                    {
                    }
                    else
                    {
                        //  Change Menu Selection
                        CurrentSerial.menuSelection = (CurrentSerial.menuSelection + (NUMBER_OF_MENU_ITEMS - 1)) % NUMBER_OF_MENU_ITEMS;

                        //  Signal a Semi-Active Element Redraw
                        CurrentSerial.updateMenuScreen = true;
                        CurrentSerial.updateMenuChoice = true;
                    }


                    break;

               //  Process Down
                case 'B':
                    if (CurrentSerial.screenActive)
                    {
                    }
                    else
                    {
                        //  Change Menu Selection
                        CurrentSerial.menuSelection = (CurrentSerial.menuSelection + 1) % NUMBER_OF_MENU_ITEMS;

                        //  Signal a Semi-Active Element Redraw
                        CurrentSerial.updateMenuScreen = true;
                        CurrentSerial.updateMenuChoice = true;
                    }


                    break;

               //  Process Left

               //  Process Right

               //  Default Catchall
                default:
                    return;
            }
            break;
        //
        //  Tab Switch Mode
        //
        case '\t':
            if (CurrentSerial.screenActive)
            {
                CurrentSerial.screenActive = false;
            }
            else
            {
                CurrentSerial.screenActive = true;
            }
            //  Signal a Semi-Active Element Redraw
            CurrentSerial.updateMenuFrames = true;

            break;

        //
        //  Redraw Screen
        //
        case 'r':
        case 'R':
            CurrentSerial.updateMenuScreen = true;
            CurrentSerial.updateMenuChoice = true;
            CurrentSerial.updateMenuFrames = true;
            CurrentSerial.updateBackground = true;
            break;

        //
        //  Effect Pause
        //
        case 'p':
        case 'P':
            if (PauseEffect)
            {
                PauseEffect = false;
            }
            else
            {
                PauseEffect = true;
            }
            break;

        //
        //  Start/Stop NeoPixel Program
        //
        case 's':
        case 'S':
            if (CurrentSettings.ProgramRunning)
            {
                StopPIOPixelProgram();
            }
            else
            {
                StartPIOPixelProgram();
            }
            break;

        //  Default Catchall
        default:
            return;
    };
}



//
//
//  Serial Interface Function - Second Core Used
//
//
void serialUSBInterface()
{
    int logoColorIndex = 0;

    //
    //  Main Loop
    //
    while (true)
    {
        //
        //  Background Layer
        //
        if (CurrentSerial.updateBackground)
        {
            //  Reset Any Attributes before drawing.
            ResetDisplayAttributes();

            //  Turn off the terminal cursor
            TurnCursorOff();

            //  Clear Screen
            ClearScreen();

            //Draw the background
            DrawBackground();

            //
            //  Reset Flag
            //
            CurrentSerial.updateBackground = false;
        }

        //
        //  Menu Frames
        //
        if (CurrentSerial.updateMenuFrames)
        {
            //  Draw Menu Windows
            DrawMenuWindows();

            //
            //  Reset Flag
            //
            CurrentSerial.updateMenuFrames = false;
        }

        //
        //  Menu Choice
        //
        if (CurrentSerial.updateMenuChoice)
        {
            //  Draw menu buttons
            DrawMenuButtons();

            //  Clear any old Screen data
            ClearMenuScreenArea();

            //  Draw Appropriate Menu Screen

            //  Draw Config Screen
            if (CurrentSerial.menuSelection == 0)
            {
                DrawConfigScreen();
            }

                //  Draw Effect Screen
            else if (CurrentSerial.menuSelection == 1)
            {
                DrawEffectScreen();
            }

                //  Draw Inputs Screen
            else if (CurrentSerial.menuSelection == 2)
            {
                DrawInputsScreen();
            }

                //  Draw Status Screen
            else if (CurrentSerial.menuSelection == 3)
            {
                DrawStatusScreen();
            }

                //  Draw Commit Screen
            else if (CurrentSerial.menuSelection == 4)
            {
                DrawCommitScreen();
            }

                //  Draw Help Screen
            else
            {
                DrawHelpScreen();
            }

            //
            //  Reset Flag
            //
            CurrentSerial.updateMenuChoice = false;
        }

        //
        //  Menu Screen Items
        //
        if (CurrentSerial.updateMenuScreen)
        {



            //
            //  Reset Flag
            //
            CurrentSerial.updateMenuScreen = false;
        }


        //
        //  Active Element Draw and Polling Zone
        //

            //Draw the Pico Pix OS Logo
            DrawMainLogo(1, 2, logoColorIndex);
            logoColorIndex = (logoColorIndex + 1) % 16;

            //  If Status Screen
            if (CurrentSerial.menuSelection == 3)
            {
                //Draw Active Status Screen components
                DrawStatusScreenActive();
            }

        //
        //  Update Rate for Serial System
        //
        sleep_ms(100);

        //
        //  Poll and Process Input
        //
        ProcessInput();
    }
}

void InitPixelBuffer()
{
    CurrentPixelBuffer.data = NULL;
    CurrentPixelBuffer.size = 0;
}

void NewPixelBuffer(int size)
{
    //Check current pixel buffer
    CurrentPixelBuffer.data = malloc(sizeof(int) * size);
    CurrentPixelBuffer.size = size;
}




//
//
//  Main Function of the program
//
//
int main()
{
    //
    //  Check and Load Settings
    //
    CheckLoadSettings();

    //  Init the Pixel Buffer
    InitPixelBuffer();

    //Get the second core running the USB Serial handling code
    multicore_launch_core1(serialUSBInterface);

    //Initialize all stdio io stuff
    stdio_init_all();

    //  Start the PIO stuff for LED driving
    StartPIOPixelProgram();

    // Size out a new pixel buffer
    NewPixelBuffer(CurrentSettings.pixelBufferSize);

    while (1)
    {
        //  Do the current Effect
        if (!PauseEffect)
        {
            switch (currentEffect)
            {
                case RANDOM:
                    for (int i = 0; i < CurrentPixelBuffer.size; i++)
                    {
                        CurrentPixelBuffer.data[i] = rand()% 0xFFFFFF & (CurrentBrightnessMask | CurrentBrightnessMask << 8 | CurrentBrightnessMask << 16);
                    }
            }
        }


        //  Write stuff out
        for (int i = 0; i < CurrentPixelBuffer.size; i++)
        {
            // Write the pattern out
            put_pixel(CurrentPixelBuffer.data[i]);
        }

        sleep_ms(100);
    }
return 0;

}
