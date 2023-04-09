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

#define PROJECTILE_HEIGHT 15
#define PROJECTILE_WIDTH 5

#define ENEMY_HEIGHT 10
#define ENEMY_WIDTH 10
#define KILLED_ENEMY_SCORE 50

// Score stuff
#define BASE_PLATFORM_HIT_SCORE 30
#define BROKEN_PLATFORM_HIT_SCORE 10
#define MOVING_PLATFORM_HIT_SCORE 50
#define DISAPPEARING_PLATFORM_HIT_SCORE 40

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

typedef struct GameState {
    // When jump will take character past 1/2 of the y-height, reposition the platforms so it reaches 1/2 at max
    bool repositioning;
    int repositionAmount;

    int score;

    int topPlatformIndex;

    // Whether game is over or not 
    bool gameOver;
} GameState;
typedef struct enemy {
    int x_pos;
    int y_pos;
    int visible;
} enemy;

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

    bool hasEnemy;
    enemy enemy;
    
    bool visible;
} platform;

typedef struct character {
    int deltaY; // Y direction of character, eiher 1 or -1
    int deltaX; // X direction of character, either 0 (no change), 1 or -1, determined by keyboard press
    int x_pos; // x position of character
    int y_pos; // y position of character

    bool countJump; // Determines if character is "jumping"
    int jumpedAmount;
} character;

typedef struct projectile {
    int x_pos; // x-position of bullet (basically where it was shot from). Stays the same
    int y_pos; // y-position of bullet. Moves continuously upwards (y=y-1) each tick. May need to account for repositioning.
    bool visible;
} projectile;

// ===============================================
// The game's global variables.

GameState state;

platform platforms[NUMBER_OF_PLATFORMS];
character ch;

projectile proj;

// ================================================

// Initialize game's global variables with relevant values
void initializeGameState() {
    state.gameOver = false;
    state.repositioning = false;
    state.score = 0;

    ch.x_pos = RESOLUTION_X / 2;
    ch.y_pos = RESOLUTION_Y - CHARACTER_HEIGHT - 2;
    ch.countJump = 0;

    proj.visible = false;
    proj.x_pos = 0;
    proj.y_pos = 0;
}

// Height is distance from the top 
platform generatePlatform(int height) {
    platform result;

    result.y_pos = height;

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
        result.deltaX = 0;
    }
    else if (randNum < 70) {
        result.type = BROKEN;
        result.color = BROWN;
        result.deltaX = 0;
    }
    else if (randNum < 90) {
        result.type = MOVING;
        result.color = BLUE;

        if (result.x_pos + PLATFORM_WIDTH < RESOLUTION_X - 1) {
            result.deltaX = 1;
        }
        else {
            result.deltaX = -1;
        }
    }
    else {
        result.type = DISAPPEARING;
        result.color = WHITE;
        result.deltaX = 0;
    }

    // Determine if platform has an enemy standing on it
    randNum = rand() % (100 + 1);
    if (randNum < 80) {
        result.hasEnemy = false;
    }
    else {
        result.hasEnemy = true;
        result.enemy.visible = true;
        result.enemy.x_pos = result.x_pos + (PLATFORM_WIDTH / 2) - (ENEMY_WIDTH / 2);
        result.enemy.y_pos = result.y_pos - ENEMY_HEIGHT;
    }

    return result;
}

// ONLY CALL THIS FUNCTION IF CHAR HITS THE PLATFORM FROM ABOVE, NOT BELOW
void platformHit(int index) {
    if (!platforms[index].visible) {
        return;
    }

    switch (platforms[index].type) {
        case REGULAR:
            ch.deltaY = 1;
            state.score += BASE_PLATFORM_HIT_SCORE;
            break;
        case BROKEN:
            platforms[index].visible = false;
            state.score += BROKEN_PLATFORM_HIT_SCORE;
            break;
        case MOVING:
            ch.deltaY = 1;
            state.score += MOVING_PLATFORM_HIT_SCORE;
            break;
        case DISAPPEARING:
            platforms[index].visible = false;
            state.score += DISAPPEARING_PLATFORM_HIT_SCORE;
            ch.deltaY = 1;
            break;
        default:
            break;
    }

    ch.countJump = true;
    ch.jumpedAmount = 0;

    if (platforms[index].y_pos - CHARACTER_JUMP_HEIGHT <= RESOLUTION_Y / 2) {
        state.repositioning = true;

        // Remember: lower position means HIGHER y-value with the VGA display!
        state.repositionAmount = (RESOLUTION_Y / 2) - (platforms[index].y_pos - CHARACTER_JUMP_HEIGHT);
    }
}

// When platform falls below visible area, move it to top and change its type
// height = new position at top (should be topPlatform.y_pos + distance_between_platforms or something)
// Have it appear when the bottom platform disappears
void moveToTop(int index, int height) {
    platforms[index] = generatePlatform(height);
}

bool checkIfHit(int index) {
    if (!platforms[index].visible) {
        return false;
    }

    int characterCentre = ((ch.x_pos + CHARACTER_WIDTH) + ch.x_pos) / 2;

    bool yHit = (ch.y_pos + CHARACTER_HEIGHT == platforms[index].y_pos);
    bool xHit = characterCentre >= platforms[index].x_pos && 
                characterCentre <= platforms[index].x_pos + PLATFORM_WIDTH - 1;

    if (xHit && yHit) {
        platformHit(index);
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

// Create projectile right above the character
projectile createProjectile() {
    projectile result;

    result.visible = true;
    result.x_pos = ch.x_pos + (CHARACTER_WIDTH / 2);
    result.y_pos = ch.y_pos;

    return result;
}

bool checkIfGameOver() {
    bool characterHitBottom = ch.y_pos >= RESOLUTION_Y - CHARACTER_HEIGHT - 1;
    bool characterHitEnemy = false;

    for (int i = 0; i < NUMBER_OF_PLATFORMS; ++i) {
        if (platforms[i].hasEnemy && platforms[i].enemy.visible) {
            characterHitEnemy = true;
        }
    }

    if (characterHitBottom || characterHitEnemy) {
        state.gameOver = true;
        return true;
    } 
    return false;
}

void gameOver() {
    // PLACEHOLDER BEHAVIOUR
    while (true) {
        printf("Game over\n");
    }
}

bool checkIfProjectileHit() {
    for (int i = 0; i < NUMBER_OF_PLATFORMS; ++i) {
        if (!platforms[i].hasEnemy || !platforms[i].enemy.visible) {
            continue;
        }

        bool xHit = proj.x_pos >= platforms[i].x_pos && proj.x_pos <= platforms[i].x_pos + PLATFORM_WIDTH;
        bool yHit = proj.y_pos == platforms[i].y_pos + PLATFORM_THICKNESS;

        if (xHit && yHit) {
            proj.visible = false;

            platforms[i].enemy.visible = false;
            platforms[i].hasEnemy = false;
            state.score += KILLED_ENEMY_SCORE;
            return true;
        }
        return false;
    }
}

// ============= UPDATE OBJECT POSITIONS ===============

void updateCharacterPosition() {
    if (!state.repositioning) {
        ch.y_pos -= ch.deltaY;
    }

    if (ch.countJump) {
        ch.jumpedAmount++;
    }

    if (ch.jumpedAmount == CHARACTER_JUMP_HEIGHT) {
        ch.deltaY = -1;
        ch.countJump = false;
        ch.jumpedAmount = 0;
        ch.deltaX = 0;
    }

    ch.x_pos += ch.deltaX;
}

// Also updates enemy positions, if one is on it
void updatePlatformPositions() {
    for (int i = 0; i < NUMBER_OF_PLATFORMS; ++i) {
        checkIfHit(i);

        if (state.repositioning) {
            platforms[i].y_pos++;

            if (platforms[i].hasEnemy) {
                platforms[i].enemy.y_pos++;
            }
        }

        if (platforms[i].deltaX != 0) {
            platforms[i].x_pos += platforms[i].deltaX;
            platforms[i].enemy.x_pos += platforms[i].deltaX;

            if (platforms[i].deltaX == 1 && platforms[i].x_pos + PLATFORM_WIDTH >= RESOLUTION_X - 1) {
                platforms[i].deltaX = -1;
            }
            else if (platforms[i].deltaX == -1 && platforms[i].x_pos == 0) {
                platforms[i].deltaX = 1;
            }
        }

        if (platforms[i].y_pos > RESOLUTION_Y - 1) {
            moveToTop(i, platforms[state.topPlatformIndex].y_pos - DISTANCE_BETWEEN_PLATFORMS);
            state.topPlatformIndex = i;
        }
    }
}

void updateProjectilePosition() {
    if (!proj.visible) {
        return;
    }

    if (!state.repositioning) {
        // Move each projectile up one y-pixel
        proj.y_pos--;
    }
    
    checkIfProjectileHit();  

    // Check if projectile hit the top of the screen
    if (proj.y_pos == 0) {
        proj.visible = false;
    }  
}

// Change position of screen objects
void updatePositions() {
    updateCharacterPosition();
    updatePlatformPositions();
    updateProjectilePosition();

    // May want to move into main func later
    if (checkIfGameOver()) {
        gameOver();
    }

    if (state.repositioning) {
        state.repositionAmount--;

        if (state.repositionAmount == 0) {
            state.repositioning = false;
        }
    }
}