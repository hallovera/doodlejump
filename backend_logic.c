#include "address_map_arm.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Some of these constants already exist in cpulator.c
// Remove when integrating this code into it

#define RESOLUTION_X 320
#define RESOLUTION_Y 240
/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BLACK 0x0000

// NEW COLOR (not defined previously)
#define BROWN 0x964B

#define PLATFORM_WIDTH 25
#define PLATFORM_THICKNESS 8
// y-distance between the upper-left corners of two platforms
#define DISTANCE_BETWEEN_PLATFORMS 20

short int colours[] = {WHITE, GREEN, BLUE, ORANGE, YELLOW, RED, CYAN, PINK, GREY, MAGENTA, BROWN};

int currentHeightUpper = RESOLUTION_Y;
int currentHeightLower = 0;

/*
 * Platform types:
 *  - REGULAR: Generates bounce like normal.
 *  - BROKEN: Breaks immediately and does NOT generate bounce. Character will fall through (useless!)
 *  - MOVING: COnstantly moving in x direction.
 *  - DISAPPEARING: Generates bounce when landed on, then immedately disappears (cannot be reused)
 */
typedef enum platformType {
    REGULAR, 
    BROKEN, 
    MOVING, 
    DISAPPEARING
} platformType;

typedef struct platform {
    int number;
    platformType type;
    short int color;
    
    // Position of UPPER LEFT of platform
    int x_pos;
    int y_pos;
    
    bool visible;
} platform;

typedef struct character {
    int deltaY; // Y direction of character, eiher 1 or -1
    int x_pos; // x position of character
    int y_pos; // y position of character
} character;

// Height is distance from the bottom (currentHeightLower)
platform generatePlatform(int num, int height) {
    platform result;

    result.number = num;
    result.y_pos = height + (PLATFORM_THICKNESS - 1);

    // Put the platform at a random x position
    result.x_pos = rand() % (RESOLUTION_X - PLATFORM_WIDTH);
    
    // Random number between 0 and 100
    int randNum = rand() % (100 + 1);

    // Use randNum to determine if platform is shown or not.
    if (randNum < 30) {
        result.visible = false;
    }
    else {
        result.visible = true;
    }

    randNum = rand() % (100 + 1);
    // Randomly determine platform type.
    // Greater odds on becoming a regular old platform
    if (randNum < 60) {
        result.type = REGULAR;
        result.color = GREEN;
    }
    else if (randNum < 70) {
        result.type = BROKEN;
        result.color = BROWN;
    }
    else if (randNum < 90) {
        result.type = MOVING;
        result.color = BLUE;
    }
    else {
        result.type = DISAPPEARING;
        result.color = WHITE;
    }

    return result;
}

// ONLY CALL THIS FUNCTION IF CHAR HITS THE PLATFORM FROM ABOVE, NOT BELOW
void platformHit(character* ch, platform* plat) {
    if (!plat->visible) {
        return;
    }

    switch (plat->type) {
        case REGULAR:
            ch->deltaY = 1;
            break;
        case BROKEN:
            plat->visible = false;
            break;
        case MOVING:
            ch->deltaY = 1;
            break;
        case DISAPPEARING:
            plat->visible = false;
            ch->deltaY = -1;
            break;
        default:
            break;
    }
}

bool checkIfHit(character ch, platform* plat) {
    // IN PROGRESS

    return false;
}