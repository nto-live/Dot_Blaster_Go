#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <joystick.h>
#include <nes.h>

// Screen dimensions for NES conio
#define SCREEN_WIDTH  32
#define SCREEN_HEIGHT 30

// Playable area boundaries
#define MIN_X 1
#define MAX_X 30
#define MIN_Y 2
#define MAX_Y 27

// NES APU Hardware Registers
#define APU_PULSE1_VOL   (*(volatile unsigned char*)0x4000)
#define APU_PULSE1_SWEEP (*(volatile unsigned char*)0x4001)
#define APU_PULSE1_LO    (*(volatile unsigned char*)0x4002)
#define APU_PULSE1_HI    (*(volatile unsigned char*)0x4003)
#define APU_STATUS       (*(volatile unsigned char*)0x4015)
#define APU_FRAME_CTR    (*(volatile unsigned char*)0x4017)

// Game variables
unsigned char px = 15; // Player X
unsigned char py = 14; // Player Y
unsigned char tx = 5;  // Target X
unsigned char ty = 5;  // Target Y
unsigned int score = 0;
unsigned int seed = 0;

// Music notes (periods at CPU clock / 16 / freq - 1)
// C Major Pentatonic melody: C5 (213), D5 (189), E5 (168), G5 (141), A5 (126)
const unsigned int melody[] = {
    213, 168, 141, 126, 141, 168, 189, 213,
    189, 168, 141, 213, 0,   141, 189, 141
};
#define MELODY_LENGTH (sizeof(melody) / sizeof(melody[0]))

unsigned char music_note_index = 0;
unsigned char music_tempo_counter = 0;
#define MUSIC_TEMPO 12 // 12 frames per note (5 notes per second at 60Hz)

// Sound effect variables
unsigned char sfx_timer = 0;

// Initialize APU sound chip
void init_apu(void) {
    APU_STATUS = 0x01;       // Enable Pulse channel 1
    APU_PULSE1_SWEEP = 0x08; // Disable sweeping
    APU_FRAME_CTR = 0x40;    // Disable APU Frame Counter IRQs
}

// Play note tone
void play_note(unsigned int period) {
    if (period == 0) {
        APU_PULSE1_VOL = 0x30; // Constant volume mode, Volume = 0 (silence)
    } else {
        APU_PULSE1_VOL = 0xbf; // 50% Duty cycle, constant volume mode, Volume = 15 (max)
        APU_PULSE1_LO = (unsigned char)(period & 0xFF);
        APU_PULSE1_HI = (unsigned char)((period >> 8) & 0x07);
    }
}

// Trigger collect sound effect
void trigger_collect_sfx(void) {
    sfx_timer = 8; // sound effect runs for 8 frames
}

// Update music and sound effects state machine
void update_audio(void) {
    // Process sound effects first
    if (sfx_timer > 0) {
        sfx_timer--;
        if (sfx_timer > 4) {
            // First part of chime: high pitch E6 (1318 Hz -> period 84)
            play_note(84);
        } else {
            // Second part of chime: higher pitch C7 (2093 Hz -> period 52)
            play_note(52);
        }
        return;
    }
    
    // Process background music
    if (music_tempo_counter > 0) {
        music_tempo_counter--;
    } else {
        unsigned int note_period = melody[music_note_index];
        play_note(note_period);
        
        music_note_index = (music_note_index + 1) % MELODY_LENGTH;
        music_tempo_counter = MUSIC_TEMPO;
    }
}

// Draw player
void draw_player(void) {
    gotoxy(px, py);
    textcolor(COLOR_GREEN);
    cputc('@');
}

// Clear player
void clear_player(void) {
    gotoxy(px, py);
    cputc(' ');
}

// Spawn a new target randomly
void spawn_target(void) {
    // Generate new random coordinates within boundaries
    tx = MIN_X + (rand() % (MAX_X - MIN_X + 1));
    ty = MIN_Y + (rand() % (MAX_Y - MIN_Y + 1));
    
    // Make sure target doesn't spawn exactly on the player
    if (tx == px && ty == py) {
        tx = (tx == MAX_X) ? MIN_X : tx + 1;
    }
}

// Draw target
void draw_target(void) {
    gotoxy(tx, ty);
    textcolor(COLOR_RED);
    cputc('*');
}

// Draw static HUD
void draw_hud(void) {
    textcolor(COLOR_WHITE);
    gotoxy(1, 0);
    cprintf("SCORE: %u / 10", score);
    gotoxy(1, 1);
    chline(SCREEN_WIDTH - 2);
}

void show_title_screen(void) {
    unsigned char color_counter = 0;
    const unsigned char colors[] = {COLOR_RED, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE, COLOR_PURPLE};
    
    clrscr();
    init_apu();
    
    while (1) {
        waitvsync();
        update_audio();
        
        seed++; // Count frames to seed rand()
        color_counter++;
        
        // Cycle colors for the title
        textcolor(colors[(color_counter >> 3) % 5]);
        gotoxy(10, 10);
        cprintf("DOT BLASTER");
        
        // Flash PRESS START
        if ((color_counter >> 4) & 1) {
            textcolor(COLOR_WHITE);
            gotoxy(10, 16);
            cprintf("PRESS START");
        } else {
            gotoxy(10, 16);
            cprintf("           ");
        }
        
        // Read Controller 1
        if (JOY_START(joy_read(JOY_1))) {
            break;
        }
    }
    
    srand(seed);
    clrscr();
}

int main(void) {
    unsigned char joy;
    unsigned char move_cooldown = 0;
    unsigned char flash_timer = 0;
    
    // Install joystick driver
    joy_install(joy_static_stddrv);
    
    // Title screen
    show_title_screen();
    
    // Initialize game state
    score = 0;
    px = 15;
    py = 14;
    spawn_target();
    
    // Draw initial screen
    draw_hud();
    draw_target();
    draw_player();
    
    // Main Game Loop
    while (1) {
        waitvsync();
        update_audio();
        
        // Handle collect feedback (screen flash)
        if (flash_timer > 0) {
            flash_timer--;
            if (flash_timer == 0) {
                bgcolor(COLOR_BLACK);
                bordercolor(COLOR_BLACK);
            }
        }
        
        // Check for win condition (10 dots collected)
        if (score >= 10) {
            break;
        }
        
        // Check joystick input
        joy = joy_read(JOY_1);
        
        // Restrict movement speed by using a cooldown timer
        if (move_cooldown > 0) {
            move_cooldown--;
        } else {
            unsigned char moved = 0;
            unsigned char next_px = px;
            unsigned char next_py = py;
            
            if (JOY_UP(joy) && py > MIN_Y) {
                next_py--;
                moved = 1;
            }
            else if (JOY_DOWN(joy) && py < MAX_Y) {
                next_py++;
                moved = 1;
            }
            
            if (JOY_LEFT(joy) && px > MIN_X) {
                next_px--;
                moved = 1;
            }
            else if (JOY_RIGHT(joy) && px < MAX_X) {
                next_px++;
                moved = 1;
            }
            
            if (moved) {
                clear_player();
                px = next_px;
                py = next_py;
                draw_player();
                move_cooldown = 4; // move once every 4 frames (responsive but controllable)
            }
        }
        
        // Check collision with target
        if (px == tx && py == ty) {
            score++;
            trigger_collect_sfx();
            
            // Draw visual feedback (flash background/border)
            bgcolor(COLOR_WHITE);
            bordercolor(COLOR_WHITE);
            flash_timer = 2; // flash for 2 frames
            
            // Update HUD and spawn new target
            draw_hud();
            spawn_target();
            draw_target();
            draw_player(); // Redraw player since target clear might erase part of player
        }
    }
    
    // Game Win Screen
    clrscr();
    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLACK);
    
    textcolor(COLOR_GREEN);
    gotoxy(12, 12);
    cprintf("YOU WIN!");
    
    textcolor(COLOR_WHITE);
    gotoxy(8, 15);
    cprintf("FINAL SCORE: %u", score);
    
    // Play sweet victory fanfare!
    // C5 (213) -> E5 (168) -> G5 (141) -> C6 (106)
    play_note(213);
    for (joy = 0; joy < 12; ++joy) waitvsync();
    play_note(168);
    for (joy = 0; joy < 12; ++joy) waitvsync();
    play_note(141);
    for (joy = 0; joy < 12; ++joy) waitvsync();
    play_note(106);
    for (joy = 0; joy < 36; ++joy) waitvsync();
    play_note(0); // Silence APU
    
    // Loop forever
    while (1) {
        waitvsync();
    }
    
    joy_uninstall();
    return EXIT_SUCCESS;
}
