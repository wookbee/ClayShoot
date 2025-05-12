#include "raylib.h"
#include <math.h>
#include <stdlib.h>

#define HIT_SCORE               1000
#define AMMO_BONUS              200
#define SHOT_SPREAD             84
#define SHOT_TIME               0.6

#define ROUND_DELAY             6

#define ANGLE_RAND_MAX          64

#define CLAY_SPAWN_VARIANCE     24
#define CLAY_SCALE              4
#define CLAY_SCALE_MIN          2
#define CLAY_SCALE_MAX          8
#define CLAY_DESPAWN_DIST       10
#define CLAYS_SMASH_PARTICLES   8
#define CLAY_SMASH_BASE_SIZE    0.02
#define CLAY_SMASH_TIME         3.0
#define CLAY_RAISE_SPEED        36
#define CLAY_DROP_SPEED         40
#define CLAY_DROP_DIST          4
#define CLAY_SPEED_MULT         0.4
#define CLAY_SHADOW_DIST        16

#define FLASH_INTERVAL          2

long score =     0;
long highscore = 0;

int ROUNDS = 5;
int CLAYS_PER_ROUND = 3;

int hits[99];

int current_clay = 0;
float fire_angle = 0;

int shots = 0;
float shot_timer =  0;

float round_timer = 4;
float music_timer = 0;

float deltatime = 0;

enum game_state{INTRO, GAME, END};
enum game_state STATE;

bool display_bonus = false;

const int screenWidth =     256;
const int screenHeight =    192;

const char* savepath = "resources/save.txt";

struct Plate
{
    Vector2 ppos;
    float   pdist;
    bool    palive;
    int die_time;
};

struct Plate plates[99];

int main(void)
{
    InitWindow(screenWidth, screenHeight, "Clay Hunt");
    SetTargetFPS(60);
    HideCursor();
    
    // Sprites              ////////////////////////////////////////////////////////////////
    Texture tex_bg          = LoadTexture("resources/bg.png");
    Texture tex_trees          = LoadTexture("resources/trees.png");
    
    // Audio                ////////////////////////////////////////////////////////////////
    InitAudioDevice(); 
    // SFX
    Sound sfx_shoot         = LoadSound("resources/shoot.wav");
    Sound sfx_dry         = LoadSound("resources/dry_fire.wav");
    Sound sfx_clay_release  = LoadSound("resources/clay_release.wav");
    Sound sfx_clay_fly      = LoadSound("resources/clay_fly.wav");
    Sound sfx_clay_smash    = LoadSound("resources/clay_smash.wav");
    Sound sfx_perfect       = LoadSound("resources/perfect.wav");
    // Music
    Music music_intro       = LoadMusicStream("resources/intro.mp3");
    Music music_r_clear     = LoadMusicStream("resources/round_clear.mp3");
    
    // Func Declarations    ////////////////////////////////////////////////////////////////
    void RefreshShotUI();
    void Shoot(Sound sfx_shoot, Sound sfx_dry, Sound sfx_clay_smash);
    void LaunchTargets(Sound sfx_clay_release, Sound sfx_clay_fly, Music music_r_clear);
    
    STATE = INTRO;
    PlayMusicStream(music_intro);
    music_timer = GetMusicTimeLength(music_intro);
    
    SetRandomSeed((int)GetTime());
    
    
    // Load Highscore       ////////////////////////////////////////////////////////////////
    if(FileExists(savepath))
    {
        char* hstext = LoadFileText(savepath);
        highscore = atoi(hstext);
        UnloadFileText(hstext);
    }
    
    while (!WindowShouldClose()) // Update Loop
    {    
        // GAME LOGIC       ////////////////////////////////////////////////////////////////
        
        deltatime = GetFrameTime();
        
        switch(STATE)
        {
            case INTRO:
                UpdateMusicStream(music_intro);
                bool start = false;
                
                if(IsKeyPressed(KEY_ONE))   {CLAYS_PER_ROUND = 2; start = true;}
                if(IsKeyPressed(KEY_TWO))   {CLAYS_PER_ROUND = 3; start = true;}
                if(IsKeyPressed(KEY_THREE)) {CLAYS_PER_ROUND = 4; start = true;}
                
                if(GetMusicTimePlayed(music_intro) >= music_timer - .1){StopMusicStream(music_intro);}
                
                if(start)
                {
                    current_clay =  -CLAYS_PER_ROUND;
                    shots = CLAYS_PER_ROUND + 1;
                    for(int i = 0; i < CLAYS_PER_ROUND; i++)
                    { 
                        hits[i] = 0;
                        plates[i].ppos.x    = 0;
                        plates[i].ppos.y    = 0;
                        plates[i].pdist     = 0;
                        plates[i].palive     = false;
                        plates[i].die_time  = 0;
                    }
                    
                    StopMusicStream(music_intro);
                    STATE = GAME;
                }
                break;
            case GAME:
                if(current_clay == -CLAYS_PER_ROUND){ LaunchTargets(sfx_clay_release, sfx_clay_fly, music_r_clear);}
                
                if(shot_timer > 0){shot_timer -= 1.0f * deltatime;}
                if(IsMouseButtonPressed((0))){ Shoot(sfx_shoot, sfx_dry, sfx_clay_smash);}
                
                int alive = CLAYS_PER_ROUND;
                for(int i = 0; i < CLAYS_PER_ROUND; i++)
                {
                    if(!plates[i].palive){alive--; continue;}
                    
                    plates[i].pdist  += 1.0f * deltatime;
                    plates[i].ppos.x += (i % 2 == 0 ? 1 : -1) * fire_angle * deltatime;
                    plates[i].ppos.y -= (CLAY_RAISE_SPEED + (CLAY_SPEED_MULT * current_clay)) * deltatime;
                    
                    if(plates[i].pdist > CLAY_DROP_DIST)
                    {
                        plates[i].ppos.y += CLAY_DROP_SPEED * deltatime;
                        if(plates[i].pdist > CLAY_DESPAWN_DIST){plates[i].palive = false;}
                    }
                }
                
                if(alive <= 0)
                {
                    if(round_timer <= 0.0f)
                    {
                        round_timer = GetTime() + GetRandomValue(ROUND_DELAY / 2, ROUND_DELAY); 
                        StopSound(sfx_clay_fly); 
                        if(shots > 0)
                        {
                            int hits_t = 0;
                            for(int i = 0; i < CLAYS_PER_ROUND; i++)
                            {
                                if(hits[current_clay + i] == 1){hits_t++;}
                            }
                            if(hits_t == CLAYS_PER_ROUND){score += AMMO_BONUS * shots; PlaySound(sfx_perfect); display_bonus = true;} 
                        }
                    }
                    if(GetTime() > round_timer) LaunchTargets(sfx_clay_release, sfx_clay_fly, music_r_clear);
                }
                break;
            case END:
                UpdateMusicStream(music_r_clear);
                if(GetMusicTimePlayed(music_r_clear) >= music_timer - .1){StopMusicStream(music_r_clear);}
                if(IsKeyPressed(KEY_ENTER))
                {
                    score = 0;
                    current_clay = -CLAYS_PER_ROUND;
                    shots = CLAYS_PER_ROUND + 1;
                    for(int i = 0; i < CLAYS_PER_ROUND * ROUNDS; i++) {hits[i] = 0;}
                    
                    PlayMusicStream(music_intro);
                    music_timer = GetMusicTimeLength(music_intro);
                    STATE = INTRO;
                }
                break;
        }
        
        if(IsKeyPressed(KEY_F11)){ToggleFullscreen();}
    
    
        // GRAPHICS       ////////////////////////////////////////////////////////////////
        BeginDrawing();
            ClearBackground(BLACK);
            if(shot_timer < SHOT_TIME - 0.01f){DrawTexture(tex_bg, 0, 0, STATE == GAME ? WHITE : GRAY);} // Background graphic
            
            switch(STATE)
            {
                case INTRO:
                    DrawText("CLAY HUNT", 8, 8, 32, GOLD);
                    DrawText(TextFormat("HIGHSCORE: %d", highscore), 8, 38, 20, LIGHTGRAY); // Highscore Text
                    if((int)GetTime() % FLASH_INTERVAL == 1){DrawText("PRESS 1-3 TO SELECT\nDIFFICULTY", 8, screenHeight / 2, 20, YELLOW);}
                    DrawText("2025 WOOKBEE", 8, screenHeight - 18, 16, WHITE); 
                    break;
                case GAME:           
                    for(int i = 0; i < CLAYS_PER_ROUND; i++)
                    {
                        float platesize =               CLAY_SCALE / plates[i].pdist;
                        if(platesize > CLAY_SCALE_MAX)  platesize = CLAY_SCALE_MAX;
                        if(platesize < CLAY_SCALE_MIN)  platesize = CLAY_SCALE_MIN;
                        
                        if(plates[i].palive)
                        {
                            if(plates[i].pdist < CLAY_DESPAWN_DIST / 2.8f) DrawCircle(plates[i].ppos.x, plates[i].ppos.y + plates[i].pdist * CLAY_SHADOW_DIST, platesize / 1.2f, DARKGREEN); // shadow
                            
                            if(plates[i].pdist < CLAY_DESPAWN_DIST / 1.5f || (int)(GetTime() * 16) % FLASH_INTERVAL == 0)
                            {
                                DrawCircle(plates[i].ppos.x, plates[i].ppos.y + 2, platesize, DARKGRAY);
                                DrawCircle(plates[i].ppos.x, plates[i].ppos.y, platesize, plates[i].pdist > CLAY_DESPAWN_DIST / 4.0f ? (plates[i].pdist > CLAY_DESPAWN_DIST / 2.0f ? GRAY : LIGHTGRAY) : WHITE);
                            }
                        }
                        else
                        {
                            if(GetTime() - plates[i].die_time < CLAY_SMASH_TIME)
                            {
                                plates[i].ppos.y += CLAY_DROP_SPEED * deltatime;
                                for(int p = 0; p < CLAYS_SMASH_PARTICLES; p++){DrawCircle(plates[i].ppos.x + GetRandomValue(-platesize, platesize) * 2.0f, plates[i].ppos.y + GetRandomValue(-platesize, platesize), CLAY_SMASH_BASE_SIZE + platesize / 2, p % 2 == 1 ? (plates[i].pdist > CLAY_DESPAWN_DIST / 2.0f ? GRAY : LIGHTGRAY) : WHITE);}
                            }
                        }
                    }
                    if(shot_timer < SHOT_TIME - 0.01f && CLAYS_PER_ROUND > 2){DrawTexture(tex_trees, 0, 0, STATE == GAME ? WHITE : GRAY);} // Trees graphic
                    if(display_bonus){DrawText(TextFormat("Bonus: +%d", AMMO_BONUS * shots), 8, 42, 24, RED);}

                                      
                    DrawText(TextFormat("%d", score), 9, screenHeight - 26, 26, BLACK);    // Score
                    DrawText(TextFormat("%d", score), 8, screenHeight - 24, 26, WHITE);       // Score outline 
                    RefreshShotUI();
                    break;
                case END:
                    if((int)GetTime() % FLASH_INTERVAL == 1){DrawText("GAME OVER!", 8, 8, 32, YELLOW);}
                    int hits_t = 0;
                    int total = CLAYS_PER_ROUND * ROUNDS;
                    for(int i = 0; i < total; i++){if(hits[i]==1){hits_t++;}}
                    DrawText(TextFormat("SCORE:\t%d\nHITS:\t\t%d/%d", score, hits_t, total), 8, 38, 20, WHITE); // Score Text
                    DrawText("Press ENTER to\ncontinue.", 8, screenHeight - 64, 20, YELLOW);
                    break;
            }
            
            if(round_timer == 0.0f || STATE != GAME)
            {
                Vector2 mpos = GetMousePosition();
                Color cursor_colour = shot_timer > 0.0f || round_timer > 0.0f ? RED : BLACK;    // crosshair colour
                DrawLine(0.0f, mpos.y, screenWidth, mpos.y, cursor_colour);                     // crosshair hoz line
                DrawLine(mpos.x, 0.0f, mpos.x, screenHeight, cursor_colour);                    // crosshair vert line
            }
        EndDrawing();
    }

    // De-Initialization
    // Unload Textures  ////////////////////////////////////////////////////////////////
    UnloadTexture(tex_bg);
    
    // Unload Audio     ////////////////////////////////////////////////////////////////
    UnloadSound(sfx_shoot);
    UnloadSound(sfx_dry);
    UnloadSound(sfx_clay_release);
    UnloadSound(sfx_clay_fly);
    UnloadSound(sfx_clay_smash);
    UnloadSound(sfx_perfect);
    
    UnloadMusicStream(music_intro);
    UnloadMusicStream(music_r_clear);
    CloseAudioDevice();  
    
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

void Shoot(Sound sfx_shoot, Sound sfx_dry, Sound sfx_clay_smash)
{
    if(shot_timer > 0 || shots <= 0 || round_timer > 0.0f){PlaySound(sfx_dry); return;}
    
    Vector2 mpos = GetMousePosition();
    
    for(int i = 0; i < CLAYS_PER_ROUND; i++)
    {
        if(!plates[i].palive){continue;}
        
        float dist = ((plates[i].ppos.x-mpos.x)*(plates[i].ppos.x-mpos.x))+((plates[i].ppos.y-mpos.y)*(plates[i].ppos.y-mpos.y)); // sqr dist
        
        if(dist * plates[i].pdist < SHOT_SPREAD)
        {
            score += HIT_SCORE;
            hits[current_clay + i] = 1;
            PlaySound(sfx_clay_smash);
            plates[i].die_time = GetTime();
            plates[i].palive = false;
        }
    }
    
    shot_timer = SHOT_TIME;
    shots--;
    PlaySound(sfx_shoot);
}

void RefreshShotUI()
{
    for(int i = 0; i < CLAYS_PER_ROUND * ROUNDS; i++){DrawCircle(12 + 12 * i, 8, 4, hits[i] == 1 ? GREEN : (i > current_clay + CLAYS_PER_ROUND - 1) ? DARKGRAY : (i >= current_clay) ? GOLD : RED);} // HIT DISPLAY
    int y = 24; // Bullet Top
    for(int i = 0; i < shots; i++){int x = 8 + 8 * i; if(shots > 1 || (int)GetTime() % FLASH_INTERVAL == 1){ DrawRectangle(x, y, 6, 12, YELLOW); DrawTriangle((Vector2){x, y + 1}, (Vector2){x + 6, y + 1}, (Vector2){x + 3, y - 5}, YELLOW);}}  // SHOTS
}

void LaunchTargets(Sound clay_release, Sound clay_fly, Music music_clear)
{
    if(current_clay < (CLAYS_PER_ROUND * ROUNDS) - CLAYS_PER_ROUND){current_clay += CLAYS_PER_ROUND;}else
    {
        STATE = END;
        music_timer = GetMusicTimeLength(music_clear);
        PlayMusicStream(music_clear);
        if(score > highscore){SaveFileText(savepath, TextFormat("%d", score)); highscore = score;}
        return;
    }
    
    display_bonus = false;
    
    PlaySound(clay_release);
    PlaySound(clay_fly);
    
    shot_timer = SHOT_TIME - 0.01f;
    SetMousePosition(screenWidth / 2, screenHeight / 2);
    
    shots = CLAYS_PER_ROUND + 1;
    
    fire_angle = GetRandomValue(ANGLE_RAND_MAX / 4, ANGLE_RAND_MAX) / CLAYS_PER_ROUND;
    
    for(int i = 0; i < CLAYS_PER_ROUND; i++)
    {
        plates[i].ppos.x = i == 0 ? 0 : (screenWidth / i) + GetRandomValue(-CLAY_SPAWN_VARIANCE, CLAY_SPAWN_VARIANCE);
        plates[i].ppos.y = GetRandomValue(screenHeight, screenHeight + CLAY_SPAWN_VARIANCE);
        plates[i].pdist  = 0;
        plates[i].palive = true;
    }
    
    round_timer = 0.0f;
}