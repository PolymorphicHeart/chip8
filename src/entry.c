#include "chip8.h"
#include "beep.h"

#include <raylib.h>

int32_t main (int32_t argc, const char** argv)
{
    if (argc < 3)
    {
        printf("usage: <scale> <path to program>\n");
        return -1;
    }

    uint32_t scale = atoi(argv[1]);
    chip8_t chip8 = {0};

    {
        FILE* file = fopen(argv[2], "rb");

        if(file == NULL)
        {
            printf("error: failed to open specified program!\n");
            return -1;
        }

        chip8_init(&chip8, file);
        fclose(file);
    }

    SetTraceLogLevel(LOG_NONE);
    InitWindow(64 * scale, 32 * scale, "Chip8 Emulator");
    InitAudioDevice();

    Wave beep_wav = LoadWaveFromMemory(GetFileExtension("beep.mp3"), beep_get_data(), beep_get_size());
    Sound beep = LoadSoundFromWave(beep_wav);
    UnloadWave(beep_wav);

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {   
        chip8.keys[0x1] = IsKeyDown(KEY_ONE);
        chip8.keys[0x2] = IsKeyDown(KEY_TWO);
        chip8.keys[0x3] = IsKeyDown(KEY_THREE);
        chip8.keys[0xC] = IsKeyDown(KEY_FOUR);
        chip8.keys[0x4] = IsKeyDown(KEY_Q);
        chip8.keys[0x5] = IsKeyDown(KEY_W);
        chip8.keys[0x6] = IsKeyDown(KEY_E);
        chip8.keys[0xD] = IsKeyDown(KEY_R);
        chip8.keys[0x7] = IsKeyDown(KEY_A);
        chip8.keys[0x8] = IsKeyDown(KEY_S);
        chip8.keys[0x9] = IsKeyDown(KEY_D);
        chip8.keys[0xE] = IsKeyDown(KEY_F);
        chip8.keys[0xA] = IsKeyDown(KEY_Z);
        chip8.keys[0x0] = IsKeyDown(KEY_X);
        chip8.keys[0xB] = IsKeyDown(KEY_C);
        chip8.keys[0xF] = IsKeyDown(KEY_V);

        static Color st_color = PURPLE;
        static int32_t st_timer = 5;

        if (IsKeyPressed(KEY_P) && !chip8.step_flag) 
        {
            chip8_print_debug(&chip8);
            chip8.step_flag = true;
        }
        else if (IsKeyPressed(KEY_P) && chip8.step_flag) chip8.step_flag = false;

        if (!chip8.step_flag) 
        {
            chip8_exec_next(&chip8);
            chip8_dec_timers(&chip8);
        }
        else if (chip8.step_flag && (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER))) 
        {
            st_color = ORANGE;
            chip8_exec_next(&chip8);
            chip8_dec_timers(&chip8);
            chip8_print_debug(&chip8);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        for (int32_t x = 0; x < CHIP8_SCREEN_X; ++x)
        {
            for (int32_t y = 0; y < CHIP8_SCREEN_Y; ++y)
            {
                if (chip8.screen[x][y]) DrawRectangle(x * scale, y * scale, scale, scale, WHITE);
            }
        }

        if (chip8.step_flag)
        {
            if (IsKeyDown(KEY_ENTER)) st_timer = 5;
            else if (st_timer <= 0) st_color = PURPLE;
            else if (st_timer > 0) st_timer --;

            DrawRectangle(2, 2, MeasureText("Breakpoint Mode", 20) + 12, 32, (Color){255, 255, 255, 200});
            DrawRectangleLines(2, 2, MeasureText("Breakpoint Mode", 20) + 12, 32, BLACK);
            DrawText("Breakpoint Mode", 9, 7, 20, BLACK);
            DrawText("Breakpoint Mode", 8, 6, 20, st_color);
        }

        EndDrawing();

        if (chip8.reg_st > 0) PlaySound(beep);
    }
}