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

// =============== MY VARIABLES (NOT DEFINED ELSEWHERE) =================

// NEW COLOR (not defined previously)
#define BROWN 0x964B

#define PLATFORM_WIDTH 25
#define PLATFORM_THICKNESS 8

#define NUMBER_OF_PLATFORMS 7
// y-distance between the upper-left corners of two platforms
#define DISTANCE_BETWEEN_PLATFORMS 20

#define CHARACTER_HEIGHT 45
#define CHARACTER_WIDTH 50
// Currently about 1/3 of the screen height
#define CHARACTER_JUMP_HEIGHT 80

// Score stuff
#define BASE_PLATFORM_HIT_SCORE 30
#define BROKEN_PLATFORM_HIT_SCORE 10
#define MOVING_PLATFORM_HIT_SCORE 50
#define DISAPPEARING_PLATFORM_HIT_SCORE 40
int score = 0;

int currentUpperHeight = 0;
int currentLowerHeight = RESOLUTION_Y - 1;

int topPlatformIndex = 0;

int lastHitHeight = -1;

bool moveScreen = false;

int currentJumpedAmount = 0;
bool countJump = false;

/*
 * Platform types:
 *  - REGULAR: Generates bounce like normal.
 *  - BROKEN: Breaks immediately and does NOT generate bounce. Character will fall through (useless!)
 *  - MOVING: Constantly moving in x direction.
 *  - DISAPPEARING: Generates bounce when landed on, then immedately disappears (cannot be reused)
 */
typedef enum platformType {
    REGULAR, 
    BROKEN, 
    MOVING, 
    DISAPPEARING
} platformType;

// NOTE: Platform may have NEGATIVE y_pos. This means it is still fully/partially ABOVE the screen,
// and should not be drawn yet.
typedef struct platform {
    int number;
    platformType type;
    short int color;
    
    // Position of UPPER LEFT of platform
    int x_pos;
    int y_pos;

    // Horizontal movement of platform (1 or -1 for MOVING, 0 for all other types)
    int deltaX;
    
    bool visible;
} platform;

typedef struct character {
    int deltaY; // Y direction of character, eiher 1 or -1
    int deltaX; // X direction of character, either 0 (no change), 1 or -1, determined by keyboard press
    int x_pos; // x position of character
    int y_pos; // y position of character
} character;

typedef struct projectile {
    int x_pos; // x-position of bullet (basically where it was shot from). Stays the same
    int y_pos; // y-position
} projectile;

// PUT THIS IN MAIN LATER
platform platforms[NUMBER_OF_PLATFORMS];

// Height is distance from the top 
platform generatePlatform(int height) {
    platform result;

    result.y_pos = height;
    // result.y_pos = height + (PLATFORM_THICKNESS - 1);

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
void platformHit(character* ch, int index) {
    if (!platforms[index].visible) {
        return;
    }

    switch (platforms[index].type) {
        case REGULAR:
            ch->deltaY = 1;
            score += BASE_PLATFORM_HIT_SCORE;
            break;
        case BROKEN:
            platforms[index].visible = false;
            score += BROKEN_PLATFORM_HIT_SCORE;
            break;
        case MOVING:
            ch->deltaY = 1;
            score += MOVING_PLATFORM_HIT_SCORE;
            break;
        case DISAPPEARING:
            platforms[index].visible = false;
            score += DISAPPEARING_PLATFORM_HIT_SCORE;
            ch->deltaY = 1;
            break;
        default:
            break;
    }
    lastHitHeight = index;
}

// When platform falls below visible area, move it to top and change its type
// Honestly doesn't need a function, just move this into main
// height = new position at top (should be topPlatform.y_pos + distance_between_platforms or something)
// Have it appear when the bottom platform disappears
void moveToTop(int index, int height) {
    platforms[index] = generatePlatform(height);
}

bool checkIfHit(character* ch, int index) {
    if (!platforms[index].visible) {
        return false;
    }

    int characterCentre = ((ch->x_pos + CHARACTER_WIDTH) + ch->x_pos) / 2;

    bool yHit = (ch->y_pos + CHARACTER_HEIGHT == platforms[index].y_pos);
    bool xHit = characterCentre >= platforms[index].x_pos && 
                characterCentre <= platforms[index].x_pos + PLATFORM_WIDTH - 1;

    if (xHit && yHit) {
        platformHit(ch, index);
        countJump = true;
        return true;
    }

    return false;
}

void createInitialPlatforms() {
    int currentHeight = 0;

    for (int i = 0; i < NUMBER_OF_PLATFORMS; ++i) {
        platforms[i] = generatePlatform(currentHeight);
        currentHeight += DISTANCE_BETWEEN_PLATFORMS;
    }
}

// WORK IN PROGRESS
// Change position of screen objects
void updateObjectPositions(character* ch) {
    // Update character
    ch->y_pos -= ch->deltaY;

    
    ch->x_pos += ch->deltaX;

    // Flip deltaY once certain jump height reached
    if (countJump) {
        currentJumpedAmount++;
    }

    if (currentJumpedAmount == CHARACTER_JUMP_HEIGHT) {
        ch->deltaY = -1;
        countJump = false;
        currentJumpedAmount = 0;
        ch->deltaX = 0;
    }

    // currentJumpedAmount = ch->deltaY == 1 ? currentJumpedAmount + 1 : currentJumpedAmount;
    

    // ch->deltaY = currentJumpedAmount == CHARACTER_JUMP_HEIGHT ? -1 : ch->deltaY;

    
    // Make screen move downwards if character's current jump will make it reach the 1/2 mark of the screen
    //TODO

    lastHitHeight = lastHitHeight != -1 ? lastHitHeight + 1 : lastHitHeight;

    // Update projectiles
    // Projectiles will disappear if 
    

    // Update platforms
    for (int i = 0; i < NUMBER_OF_PLATFORMS; ++i) {
        // Check if platform hit by character
        checkIfHit(ch, i);
        //
        // platforms[i].y_pos++;

        if (platforms[i].y_pos > currentLowerHeight) {
            moveToTop(i, platforms[topPlatformIndex].y_pos - DISTANCE_BETWEEN_PLATFORMS);
            topPlatformIndex = i;
        }


    }

    // Update
}