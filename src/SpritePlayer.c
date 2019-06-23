// All player and game logic

#pragma bank 2
#include "main.h"
UINT8 bank_SPRITE_PLAYER = 2;

#include "Keys.h"
#include "SpriteManager.h"
#include "Sound.h"
#include "ZGBMain.h"

// Movement values for tweaking platforming feel
#define walkSpeed 130
#define runSpeed 200
#define walkIncrease 50
#define runIncrease 60
#define slowDownSpeed 8
#define respawnMoveSpeed 3
#define rockBreakSpeed 180

// Animation frames (number of frames followed by all the frames to play)
const UINT8 animIdle[] = {1, 1};
const UINT8 animWalk[] = {2, 2, 3};
const UINT8 animRun[] = {4, 11, 12, 13, 14};
const UINT8 animJumpUp[] = {1, 6};
const UINT8 animJumpPeak[] = {1, 5};
const UINT8 animJumpDown[] = {1, 7};
const UINT8 animRespawn[] = {3, 8, 9, 10};

typedef enum
{
    GROUNDED,   // When player is on ground
    ONAIR,      // When player is on air
    RESPAWN     // When player is moving to respawn point after hurt
} MOVE_STATE;

// Start from air to drop the player on the stage
MOVE_STATE moveState = ONAIR; 

// Player movement physics
INT16 accelY;
INT16 accelX;
UINT8 jumpPeak;
UINT8 runJump;

// Storing collisions around player after movement
UINT8 collisionX;
UINT8 collisionY;
UINT8 groundCollision;

// Plase to respawn when hurt
INT16 checkpointX = 10;
INT16 checkpointY = 96;

// Last checkpoint
UINT8 checkpointIndex = 0;

// When do we reach next checkpoint
INT16 nextCheckpoint = 376;

// Timer for playing step sounds
UINT8 stepSound = 0;

// Timer for rock break effect
UINT8 rockBreakFrames;

// General helpers
UINT8 i;
struct Sprite *spr;

// Call to reset player's state back to default
void ResetState()
{
    accelY = 0;
    accelX = 0;
    collisionX = 0;
    collisionY = 0;
    groundCollision = 0;
    jumpPeak = 0;
    runJump = 0;
    moveState = ONAIR;
}

// Set starting position and collider size
void Start_SPRITE_PLAYER()
{

    THIS->coll_x = 2;
    THIS->coll_y = 2;
    THIS->coll_w = 12;
    THIS->coll_h = 14;
    ResetState();
}

// Play stepping sound if we have ran enough from last time
void StepAudio()
{
    if (moveState == GROUNDED && stepSound > 12)
    {
        stepSound = 0;
        PlayFx(CHANNEL_4, 4, 0x05, 0x31, 0x78, 0x80);
    }
}

void Update_SPRITE_PLAYER()
{
    // Respawn
    // Move player to last checkpoint and reset them
    if (moveState == RESPAWN)
    {
        if (THIS->x > checkpointX)
        {
            if ((THIS->x - respawnMoveSpeed) > checkpointX)
            {
                THIS->x -= respawnMoveSpeed;
            }
            else
            {
                THIS->x--;
            }
        }
        else if (THIS->x < checkpointX)
        {
            if ((THIS->x + respawnMoveSpeed) < checkpointX)
            {
                THIS->x += respawnMoveSpeed;
            }
            else
            {
                THIS->x++;
            }
        }

        if (THIS->y > checkpointY)
        {
            if ((THIS->y - respawnMoveSpeed) > checkpointY)
            {
                THIS->y -= respawnMoveSpeed;
            }
            else
            {
                THIS->y--;
            }
        }
        else if (THIS->y < checkpointY)
        {
            if ((THIS->y + respawnMoveSpeed) < checkpointY)
            {
                THIS->y += respawnMoveSpeed;
            }
            else
            {
                THIS->y++;
            }
        }

        if (THIS->x == checkpointX && THIS->y == checkpointY)
        {
            ResetState();
        }
        return;
    }

    // Directional input
    // Flip sprite to correct direction
    // Increase x acceleration to pressed direction
    // Increase speacceleration & step sound more if player runs
    // Clamp running speed when grounded
    if (KEY_PRESSED(J_LEFT))
    {
        THIS->flags = S_FLIPX;

        if (KEY_PRESSED(J_B))
        {
            stepSound += 2;
            accelX -= runIncrease;
            if (accelX < -runSpeed && moveState == GROUNDED)
                accelX = -runSpeed;
        }
        else
        {
            stepSound++;
            accelX -= walkIncrease;
            if (accelX < -walkSpeed && moveState == GROUNDED)
                accelX = -walkSpeed;
        }
    }
    else if (KEY_PRESSED(J_RIGHT))
    {
        THIS->flags = 0;

        if (KEY_PRESSED(J_B))
        {
            stepSound += 2;
            accelX += runIncrease;
            if (accelX > runSpeed && moveState == GROUNDED)
                accelX = runSpeed;
        }
        else
        {
            stepSound++;
            accelX += walkIncrease;
            if (accelX > walkSpeed && moveState == GROUNDED)
                accelX = walkSpeed;
        }
    }

    // Jump
    // If we are on ground and A is pressed, jump
    // Jump sound is played on every successful jump
    // Jump is controlled by reaching high enought acceleration
    // or hitting a wall with head
    // In both cases, we set peak to true and start falling
    // Note that we can't start running when jumping,
    // but we can keep the running speed if we start the jump from run
    if (moveState == GROUNDED)
    {
        if (KEY_TICKED(J_A))
        {
            accelY = -150;
            jumpPeak = 0;
            moveState = ONAIR;
            runJump = KEY_PRESSED(J_B) ? 1 : 0;
            PlayFx(CHANNEL_1, 5, 0x71, 0x03, 0x44, 0xc8, 0x80);
        }
    }
    else if (moveState == ONAIR)
    {
        if (collisionY != 0)
        {
            jumpPeak = 1;
        }

        if (jumpPeak == 0 && KEY_PRESSED(J_A) && accelY > -350)
        {
            accelY -= 20;
        }
        else if (accelY < 300)
        {
            accelY += 20;
            jumpPeak = 1;
        }

        if (runJump)
        {
            if (accelX < -runSpeed)
                accelX = -runSpeed;
            if (accelX > runSpeed)
                accelX = runSpeed;
        }
        else
        {
            if (accelX < -walkSpeed)
                accelX = -walkSpeed;
            if (accelX > walkSpeed)
                accelX = walkSpeed;
        }
    }

    // Move player and check for collisions
    // Do two movements to get colliders from both directions
    collisionX = TranslateSprite(THIS, accelX / 100, 0);
    collisionY = TranslateSprite(THIS, 0, accelY / 100);

    // Check collision againts other sprites
    // Game only has rocks as other sprites so we can assume player hit a rock
    // If player was running, break the rock
    // Otherwise slide the player off the rock
    // On break, play audio, do freezeframe and init rock break effect
    SPRITEMANAGER_ITERATE(i, spr)
    {
        if (spr->type == SPRITE_ROCK)
        {
            if (CheckCollision(THIS, spr))
            {
                if (accelX > rockBreakSpeed || accelX < -rockBreakSpeed)
                {
                    PlayFx(CHANNEL_4, 5, 0x1c, 0xe5, 0x7a, 0xc0);
                    delay(40);
                    SpriteManagerRemoveSprite(spr);
                    stepSound = 0;
                    rockBreakFrames = 0;
                }
                else
                {
                    collisionX = 254;
                    if (THIS->x > spr->x)
                    {
                        TranslateSprite(THIS, 1, 0);
                    }
                    else
                    {
                        TranslateSprite(THIS, -1, 0);
                    }
                }
            }
        }
    }

    // Rock break effect
    // Flash screen quickly by doing palette shift
    if (rockBreakFrames < 10)
    {
        rockBreakFrames++;

        if (rockBreakFrames > 9)
        {
            BGP_REG = 0xE1;
            OBP0_REG = 0xE1;
            OBP1_REG = 0xE1;
        }
        else if (rockBreakFrames > 6)
        {
            BGP_REG = 0xF9;
            OBP0_REG = 0xF9;
            OBP1_REG = 0xF9;
        }
        else if (rockBreakFrames > 3)
        {
            BGP_REG = 0xFE;
            OBP0_REG = 0xFE;
            OBP1_REG = 0xFE;
        }
        else
        {
            BGP_REG = 0xFF;
            OBP0_REG = 0xFF;
            OBP1_REG = 0xFF;
        }
    }

    // X physics
    // Stop movement if we hit something
    // Otherwise drag
    if (collisionX != 0)
    {
        accelX = 0;
    }
    else if (!KEY_PRESSED(J_LEFT) && !KEY_PRESSED(J_RIGHT))
    {
        if (accelX > 0)
        {
            if (accelX > slowDownSpeed)
            {
                accelX -= slowDownSpeed;
            }
            else
            {
                accelX = 0;
            }
        }
        else if (accelX < 0)
        {
            if (accelX < -slowDownSpeed)
            {
                accelX += slowDownSpeed;
            }
            else
            {
                accelX = 0;
            }
        }
    }

    // Y physics
    // Drop down if we don't have ground under
    // Play audio on land
    // Start from step sound 6 on land to have start playing stepping audio when we walk quickly
    if (accelY > 0)
    {
        groundCollision = collisionY;

        if (groundCollision == 0)
        {
            moveState = ONAIR;
        }
        else
        {
            if (moveState == ONAIR)
            {
                PlayFx(CHANNEL_4, 4, 0x32, 0x71, 0x73, 0x80);
                stepSound = 6;
                moveState = GROUNDED;
            }
            accelY = 100;
        }
    }
    else
    {
        groundCollision = 0;
        moveState = ONAIR;
    }

    // Checkpoints
    // When checkpoint X is reached assign a new checkpoint
    // Hold up and press select to debug jump to next checkpoint
    // Note that last checkpoint sets impossible target for next checkpoint
    if (THIS->x > nextCheckpoint || (KEY_PRESSED(J_UP) && KEY_TICKED(J_SELECT)))
    {
        PlayFx(CHANNEL_1, 5, 0x73, 0xc8, 0x65, 0x20, 0x83);
        checkpointIndex++;

        if (checkpointIndex == 1)
        {
            nextCheckpoint = 918;
            checkpointX = 368;
            checkpointY = 8;
        }
        else if (checkpointIndex == 2)
        {
            nextCheckpoint = 1720;
            checkpointX = 918;
            checkpointY = 48;
        }
        else if (checkpointIndex == 3)
        {
            nextCheckpoint = 2048;
            checkpointX = 1720;
            checkpointY = 88;
        }
        else if (checkpointIndex == 4)
        {
            nextCheckpoint = 2344;
            checkpointX = 2048;
            checkpointY = 112;
        }
        else if (checkpointIndex == 5)
        {
            nextCheckpoint = 2656;
            checkpointX = 2332;
            checkpointY = 108;
        }
        else if (checkpointIndex == 6)
        {
            nextCheckpoint = 3104;
            checkpointX = 2656;
            checkpointY = 64;
        }
        else if (checkpointIndex == 7)
        {
            nextCheckpoint = 3200;
            checkpointX = 3104;
            checkpointY = 108;
        }
    }

    // Animation
    // Play correct animation based on current state & input
    if (moveState == GROUNDED)
    {
        if (accelX < 100 && accelX > -100)
        {
            SetSpriteAnim(THIS, animIdle, 1);
        }
        else if (KEY_PRESSED(J_B))
        {
            StepAudio();
            SetSpriteAnim(THIS, animRun, 15);
        }
        else
        {
            StepAudio();
            SetSpriteAnim(THIS, animWalk, 15);
        }
    }
    else
    {
        if (accelY > 60)
        {
            SetSpriteAnim(THIS, animJumpDown, 1);
        }
        else if (accelY < -60)
        {
            SetSpriteAnim(THIS, animJumpUp, 1);
        }
        else
        {
            SetSpriteAnim(THIS, animJumpPeak, 1);
        }
    }

    // Hurt
    // When certain tiles are hit or player falls off the screen, play sound and respawn them
    // Note that all hurting tiles are in sequence for easier check here
    if (THIS->y > 126 || KEY_TICKED(J_SELECT) || (collisionY > 46 && collisionY < 56) || (collisionX > 46 && collisionX < 56))
    {
        moveState = RESPAWN;
        THIS->flags = 0;
        SetSpriteAnim(THIS, animRespawn, 15);
        PlayFx(CHANNEL_1, 5, 0x73, 0x03, 0x4c, 0xfa, 0x80);
    }
}

void Destroy_SPRITE_PLAYER()
{
}