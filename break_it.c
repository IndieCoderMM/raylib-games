#include "raylib.h"

#define SIZEOF(A) (sizeof(A) / sizeof(A[0]))
#define MAX_ROWS 4
#define MAX_COLS 11
#define MAX_PARTICLES 50
#define BALL_SPEED 250
#define PAD_SPEED 900
#define SPRITE_SIZE 32
#define BRICK_TIER 5
#define BRICK_WIDTH 64
#define BRICK_HEIGHT 32
#define MAX_EMITTERS 3
#define PADDLE_TOTAL 4

typedef struct Ball
{
    float x, y;
    float speedX, speedY;
    float radius;
    Rectangle srcRect;
} Ball;

typedef struct Paddle
{
    float x, y;
    float speed;
    float width, height;
    Rectangle collisionRect;
    Rectangle srcRect;
} Paddle;

typedef struct Brick
{
    float x, y;
    float width, height;
    Rectangle collisionRect;
    Rectangle srcRect;
    int health;
    int tier;
    bool broken;
} Brick;

typedef struct Particle
{
    Vector2 position;
    Color color;
    float alpha;
    float radius;
    bool active;
} Particle;

typedef struct ParticleSystem
{
    Particle particles[MAX_PARTICLES];
    Rectangle area;
    Color color;
    float radius;
    float gravity;
    float fade;
} ParticleSystem;

typedef struct Emitter
{
    ParticleSystem particleSys;
    bool active;
    float fade;
    float size;
    float gravity;
} Emitter;

typedef struct Button
{
    Color color;
    char *name;
    bool active;
} Button;

typedef struct Setting
{
    int paddle;
    int difficulty;
} Setting;

enum State
{
    MENU,
    SETTING,
    PLAY,
    GAMEOVER,
    VICTORY,
    SCORES,
    PAUSED
};

void initBricks(Brick bricks[MAX_ROWS][MAX_COLS], int level);
void initEmitter(Emitter *emitter, Rectangle area, Color color);
void initParticleSystem(ParticleSystem *particleSystem);
void updateBall(Ball *ball);
void resetBall(Ball *ball, float x, float y);
void paddleControl(Paddle *paddle);
bool paddleCollision(Ball *ball, Paddle *paddle);
void brickCollisions(Ball *ball, Brick bricks[MAX_ROWS][MAX_COLS], int *score, Emitter *emitter);
void drawBricks(Brick bricks[MAX_ROWS][MAX_COLS], int rows, int cols, Texture2D spriteSheet, Rectangle srcRect);
void drawParticleSystem(ParticleSystem particleSys);
void updateParticleSystem(Emitter *emitter);
void drawHearts(Texture2D spriteSheet, Rectangle srcRect, float x, float y, int lives);
void switchButtons(Button buttons[], int size, Color activeColor, Color normalColor);
void drawButtons(Button buttons[], int total, int fontSize);
Rectangle getRect(float x, float y, int width, int height);

void debugPrint(int val, int x, int y);

int main(void)
{
    const int screenWidth = 900;
    const int screenHeight = 550;

    InitWindow(screenWidth, screenHeight, "Wall Break");
    // InitAudioDevice();

    // Loading Sprites
    Texture2D spriteSheet = LoadTexture("assets/breakOutAssets.png");
    const Rectangle srcRectPaddle = {255, spriteSheet.height - SPRITE_SIZE * 1, SPRITE_SIZE * 3, SPRITE_SIZE};
    const Rectangle srcRectBrick = {spriteSheet.width - BRICK_WIDTH * BRICK_TIER, 0, BRICK_WIDTH, BRICK_HEIGHT};
    const Rectangle srcRectHeart = {704, 352, SPRITE_SIZE, SPRITE_SIZE};

    // Loading SFX
    // Sound fxBounce = LoadSound("assets/buttonfx.wav");

    int level = 1;
    int lives = 3;
    int score = 0;
    Button menuButtons[] = {
        (Button){WHITE, "Start", true},
        (Button){WHITE, "Leaderboard", false}};

    Emitter *emitters[MAX_EMITTERS];
    for (int e = 0; e < MAX_EMITTERS; e++)
    {
        emitters[e] = (Emitter *)MemAlloc(sizeof(Emitter));
        emitters[e]->active = false;
        emitters[e]->fade = 0.05f;
        emitters[e]->gravity = 0.09f;
        emitters[e]->size = 5.0f;
    }

    Ball ball;
    ball.x = screenWidth / 2;
    ball.y = screenHeight - 150;
    ball.speedX = 0;
    ball.speedY = 0;
    ball.radius = SPRITE_SIZE / 2;
    ball.srcRect = (Rectangle){383, spriteSheet.height - SPRITE_SIZE, SPRITE_SIZE, SPRITE_SIZE};

    Paddle paddle;
    paddle.x = screenWidth / 2;
    paddle.y = screenHeight - 80;
    paddle.speed = PAD_SPEED;
    paddle.width = SPRITE_SIZE * 3;
    paddle.height = SPRITE_SIZE;
    paddle.srcRect = srcRectPaddle;

    Brick bricks[MAX_ROWS][MAX_COLS] = {0};
    initBricks(bricks, level);

    enum State gameState = MENU;
    Setting gameSetting = {0, 0};
    SetTargetFPS(60);
    // Main game loop
    while (!WindowShouldClose())
    {
        // Debug Code
        if (IsKeyPressed(KEY_R))
            initBricks(bricks, level);
        // Menu Screen
        if (gameState == MENU)
        {
            menuButtons[0].name = "Start";
            menuButtons[1].name = "Leaderboard";
            switchButtons(menuButtons, SIZEOF(menuButtons), RED, WHITE);
            // Switch to Setting Screen
            if (IsKeyPressed(KEY_ENTER))
            {
                if (menuButtons[0].active)
                    gameState = SETTING;
                else
                    gameState = SCORES;
            }
        }
        // Paddle Select State
        else if (gameState == SETTING)
        {
            if (IsKeyPressed(KEY_RIGHT) && gameSetting.paddle < PADDLE_TOTAL - 1)
                gameSetting.paddle++;
            if (IsKeyPressed(KEY_LEFT) && gameSetting.paddle > 0)
                gameSetting.paddle--;

            paddle.srcRect = (Rectangle){srcRectPaddle.x, srcRectPaddle.y - paddle.height * gameSetting.paddle, paddle.width, paddle.height};
            if (IsKeyPressed(KEY_ENTER))
                gameState = PLAY;
        }
        // Gameover state
        else if (gameState == GAMEOVER)
        {
            menuButtons[0].name = "Play Again";
            menuButtons[1].name = "Main Menu";
            switchButtons(menuButtons, SIZEOF(menuButtons), GOLD, WHITE);
            if (IsKeyPressed(KEY_ENTER) && menuButtons[0].active)
            {
                lives = 3;
                score = 0;
                initBricks(bricks, level);
                resetBall(&ball, paddle.x, paddle.y - paddle.height);
                gameState = PLAY;
            }
        }
        // Victory State
        else if (gameState == VICTORY)
        {
            menuButtons[0].name = "Next Level";
            menuButtons[1].name = "Leaderboards";

            switchButtons(menuButtons, SIZEOF(menuButtons), VIOLET, BLACK);
            if (IsKeyPressed(KEY_ENTER) && menuButtons[0].active)
            {
                level++;
                initBricks(bricks, level);
                resetBall(&ball, screenWidth / 2, screenHeight - 150);
                gameState = PLAY;
            }
        }
        // Play State
        else if (gameState == PLAY)
        {
            if (IsKeyPressed(KEY_SPACE))
            {
                if (ball.speedX == 0 && ball.speedY == 0)
                {
                    ball.speedX = BALL_SPEED;
                    ball.speedY = BALL_SPEED;
                }
                else
                    gameState = PAUSED;
            }

            if (ball.speedX == 0 && ball.speedY == 0)
                resetBall(&ball, paddle.x, paddle.y - paddle.height);
            // Paddle Control
            paddleControl(&paddle);
            paddleCollision(&ball, &paddle);

            // if (paddleCollision(&ball, &paddle))
            //     PlaySound(fxBounce);

            // Get available emitter
            Emitter *freeEmitter;
            for (int e = MAX_EMITTERS - 1; e > 0; e--)
            {
                freeEmitter = emitters[0];
                if (!emitters[e]->active)
                {
                    emitters[e]->active = true;
                    freeEmitter = emitters[e];
                    e = 0;
                }
            }
            brickCollisions(&ball, bricks, &score, freeEmitter);
            updateBall(&ball);
            for (int e = 0; e < MAX_EMITTERS; e++)
            {
                if (!emitters[e]->active)
                    continue;
                updateParticleSystem(emitters[e]);
            }
            if (ball.y >= screenHeight + SPRITE_SIZE)
            {
                lives--;
                if (lives <= 0)
                    gameState = GAMEOVER;
                else
                    resetBall(&ball, screenWidth / 2, screenHeight - 140);
            }
            bool victory = true;
            for (int i = 0; i < MAX_ROWS; i++)
            {
                for (int j = 0; j < MAX_COLS; j++)
                {
                    if (!(bricks[i][j].broken))
                        victory = false;
                }
            }
            if (victory)
                gameState = VICTORY;
        }
        else if (gameState == PAUSED)
        {
            if (IsKeyPressed(KEY_SPACE))
            {
                gameState = PLAY;
            }
        }

        BeginDrawing();

        if (gameState == MENU)
        {
            ClearBackground(SKYBLUE);
            DrawText("Break-it", screenWidth / 2 - MeasureText("Break-it", 200) / 2 - 3, 13, 200, DARKGRAY);
            DrawText("Break-it", screenWidth / 2 - MeasureText("Break-it", 200) / 2, 10, 200, MAROON);
            drawButtons(menuButtons, SIZEOF(menuButtons), 70);
        }
        else if (gameState == SETTING)
        {
            ClearBackground(SKYBLUE);
            DrawText("Select Paddle", screenWidth / 2 - MeasureText("Select Paddle", 80) / 2, 10, 80, MAROON);
            DrawText("Press LEFT/RIGHT ARROW to change skin", screenWidth / 2 - MeasureText("Press LEFT/RIGHT ARROW to change skin", 30) / 2, screenHeight - 100, 30, DARKGRAY);
            DrawText("Press ENTER to continue", screenWidth / 2 - MeasureText("Press ENTER to continue", 30) / 2, screenHeight - 50, 30, DARKGRAY);
            DrawRectangle(screenWidth / 2 - paddle.width, screenHeight / 2 - paddle.height, paddle.width * 2, paddle.height * 3, DARKGRAY);
            DrawTextureRec(spriteSheet, paddle.srcRect, (Vector2){screenWidth / 2 - paddle.width / 2, screenHeight / 2}, WHITE);
        }
        else if (gameState == VICTORY)
        {
            ClearBackground(RAYWHITE);
            DrawText("Level Clear!", screenWidth / 2 - MeasureText("Level Clear!", 100) / 2, 30, 100, MAGENTA);
            drawButtons(menuButtons, SIZEOF(menuButtons), 70);
        }
        else if (gameState == PLAY)
        {
            ClearBackground(SKYBLUE);
            drawBricks(bricks, MAX_ROWS, MAX_COLS, spriteSheet, srcRectBrick);
            for (int e = 0; e < MAX_EMITTERS; e++)
            {
                drawParticleSystem(emitters[e]->particleSys);
            }
            DrawText(TextFormat("Level: %d", level), screenWidth / 2 - MeasureText("Level: 8", 150) / 2, screenHeight / 2, 150, Fade(WHITE, 0.2));

            DrawTextureRec(spriteSheet, ball.srcRect, (Vector2){ball.x - ball.radius, ball.y - ball.radius}, WHITE);
            DrawTextureRec(spriteSheet, paddle.srcRect, (Vector2){paddle.x - paddle.width / 2, paddle.y - paddle.height / 2}, WHITE);

            DrawText(TextFormat("Score: %d", score), 10, screenHeight - 50, 40, WHITE);
            drawHearts(spriteSheet, srcRectHeart, screenWidth - srcRectHeart.width * 4, screenHeight - srcRectHeart.height - 20, lives);
        }
        else if (gameState == GAMEOVER)
        {
            ClearBackground(DARKGRAY);
            DrawText("Game Over!", screenWidth / 2 - MeasureText("Game Over!", 150) / 2, 100, 150, RED);
            drawButtons(menuButtons, SIZEOF(menuButtons), 70);
        }
        else
        {
            DrawText("Paused II", screenWidth / 2 - MeasureText("Paused II", 100) / 2, screenHeight / 2, 100, DARKGREEN);
            DrawText("Press SPACE to resume...", screenWidth / 2 - MeasureText("Press SPACE to resume...", 50) / 2, screenHeight / 2 + 200, 50, DARKGREEN);
        }
        EndDrawing();
    }

    UnloadTexture(spriteSheet);
    // UnloadSound(fxBounce);
    CloseAudioDevice();
    CloseWindow(); // Close window and OpenGL context

    return 0;
}

void initBricks(Brick bricks[MAX_ROWS][MAX_COLS], int level)
{
    int totalRows = level % MAX_ROWS;
    if (level >= MAX_ROWS)
        totalRows = MAX_ROWS;

    for (int i = 0; i < MAX_ROWS; i++)
    {
        int totalCols = GetRandomValue(MAX_COLS - 4, MAX_COLS);
        if (totalCols % 2 == 0)
            totalCols += 1;
        float margin = (GetScreenWidth() - BRICK_WIDTH * totalCols) / 2;

        bool skipped = GetRandomValue(0, 1) > 0;
        bool alternate = GetRandomValue(0, 1) > 0;
        int maxTier = level - 1;
        if (maxTier >= BRICK_TIER)
            maxTier = BRICK_TIER - 1;
        int tiers[] = {GetRandomValue(0, maxTier), GetRandomValue(0, maxTier)};
        int colorIndex = 0;
        for (int j = 0; j < MAX_COLS; j++)
        {
            if ((skipped && j % 2) || j >= totalCols || i >= totalRows)
            {
                bricks[i][j].broken = true;
                continue;
            }
            else
                bricks[i][j].broken = false;
            bricks[i][j].width = BRICK_WIDTH;
            bricks[i][j].height = BRICK_HEIGHT;
            bricks[i][j].x = bricks[i][j].width * j + bricks[i][j].width / 2 + margin;
            bricks[i][j].y = bricks[i][j].height * i + bricks[i][j].height / 2 + bricks[i][j].height;
            bricks[i][j].collisionRect = getRect(bricks[i][j].x, bricks[i][j].y, bricks[i][j].width, bricks[i][j].height);

            if (alternate)
                colorIndex = (colorIndex + 1) % 2;
            bricks[i][j].tier = tiers[colorIndex];
            bricks[i][j].health = 3;
        }
    }
}

void drawBricks(Brick bricks[MAX_ROWS][MAX_COLS], int rows, int cols, Texture2D spriteSheet, Rectangle srcRect)
{
    int index;
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            if (bricks[i][j].broken)
                continue;
            index = 3 - bricks[i][j].health;
            bricks[i][j].srcRect = (Rectangle){srcRect.x + bricks[i][j].width * index, srcRect.y + bricks[i][j].height * bricks[i][j].tier, srcRect.width, srcRect.height};
            DrawRectangleRec(bricks[i][j].collisionRect, WHITE);
            DrawTextureRec(spriteSheet, bricks[i][j].srcRect, (Vector2){bricks[i][j].x - bricks[i][j].width / 2, bricks[i][j].y - bricks[i][j].height / 2}, WHITE);
        }
    }
}

void paddleControl(Paddle *paddle)
{
    if (IsKeyDown(KEY_RIGHT))
    {
        paddle->x += paddle->speed * GetFrameTime();
    }
    else if (IsKeyDown(KEY_LEFT))
    {
        paddle->x -= paddle->speed * GetFrameTime();
    }
    if (paddle->x < paddle->width / 2)
        paddle->x = paddle->width / 2;
    else if (paddle->x > GetScreenWidth() - paddle->width / 2)
        paddle->x = GetScreenWidth() - paddle->width / 2;
}

void updateBall(Ball *ball)
{
    ball->x += ball->speedX * GetFrameTime();
    ball->y += ball->speedY * GetFrameTime();
    if (ball->y <= ball->radius)
    {
        ball->y = ball->radius;
        ball->speedY *= -1;
    }

    if (ball->x <= ball->radius)
    {
        ball->x = ball->radius;
        ball->speedX *= -1;
    }
    else if (ball->x >= GetScreenWidth() - ball->radius)
    {
        ball->x = GetScreenWidth() - ball->radius;
        ball->speedX *= -1;
    }
}

void resetBall(Ball *ball, float x, float y)
{
    ball->x = x;
    ball->y = y;
    ball->speedX = 0;
    ball->speedY = 0;
}

bool paddleCollision(Ball *ball, Paddle *paddle)
{
    Vector2 center = {ball->x, ball->y};
    paddle->collisionRect = getRect(paddle->x, paddle->y, paddle->width, paddle->height);
    if (CheckCollisionCircleRec(center, ball->radius, paddle->collisionRect) && ball->y <= paddle->y + paddle->height / 2)
    {

        // ball.speedX *= -1;
        ball->y = paddle->y - paddle->height / 2 - ball->radius;
        ball->speedY *= -1;

        if (ball->speedX < 0 && ball->x < paddle->x)
        {
            ball->speedX = -BALL_SPEED + (paddle->x - ball->x) * -10;
        }
        else if (ball->speedX > 0 && ball->x > paddle->x)
        {
            ball->speedX = BALL_SPEED + (ball->x - paddle->x) * 10;
        }

        return true;
    }
    return false;
}

void brickCollisions(Ball *ball, Brick bricks[MAX_ROWS][MAX_COLS], int *score, Emitter *emitter)
{
    Vector2 center = {ball->x, ball->y};
    Color colors[] = {RED, DARKBLUE, DARKGREEN, BROWN, YELLOW, PURPLE};
    int colorIndex = 0;
    for (int i = 0; i < MAX_ROWS; i++)
    {
        for (int j = 0; j < MAX_COLS; j++)
        {
            if (bricks[i][j].broken)
                continue;

            if (CheckCollisionCircleRec(center, 2 * ball->radius, bricks[i][j].collisionRect))
            {
                bool isSideCollision = (ball->x < (bricks[i][j].x - bricks[i][j].width / 2) || ball->x > (bricks[i][j].x + bricks[i][j].width / 2));
                if (isSideCollision)
                {
                    ball->speedX *= -1;
                    if (ball->x < (bricks[i][j].x))
                        ball->x -= ball->radius;
                    else
                        ball->x += ball->radius;
                }
                else
                {
                    ball->speedY *= -1;
                    if (ball->y < (bricks[i][j].y))
                        ball->y -= ball->radius;
                    else
                        ball->y += ball->radius;
                }

                if (bricks[i][j].tier == 0)
                {
                    bricks[i][j].health--;
                    *score += 1;
                    if (bricks[i][j].health <= 0)
                    {
                        bricks[i][j].broken = true;
                        *score += 1;
                    }
                }
                else if (bricks[i][j].tier > 0)
                {
                    *score += bricks[i][j].tier;
                    colorIndex = bricks[i][j].tier;
                    bricks[i][j].tier--;
                }

                initEmitter(emitter, bricks[i][j].collisionRect, colors[colorIndex]);
                return;
            }
        }
    }
    return;
}

void switchButtons(Button buttons[], int size, Color activeColor, Color normalColor)
{
    // Select menu buttons
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN))
    {
        for (int i = 0; i < size; i++)
        {
            buttons[i].active = !buttons[i].active;
        }
    }
    for (int i = 0; i < size; i++)
    {
        if (buttons[i].active)
            buttons[i].color = activeColor;
        else
            buttons[i].color = normalColor;
    }
}

void drawButtons(Button buttons[], int total, int fontSize)
{
    for (int i = 0; i < total; i++)
    {
        DrawText(buttons[i].name, GetScreenWidth() / 2 - MeasureText(buttons[i].name, fontSize) / 2, GetScreenHeight() / 2 + (fontSize + 15) * i, fontSize, buttons[i].color);
    }
}

void initEmitter(Emitter *emitter, Rectangle area, Color color)
{

    emitter->active = true;
    emitter->particleSys.radius = emitter->size;
    emitter->particleSys.color = color;
    emitter->particleSys.fade = emitter->fade;
    emitter->particleSys.gravity = emitter->gravity;
    emitter->particleSys.area = area;
    initParticleSystem(&emitter->particleSys);
}

void initParticleSystem(ParticleSystem *particleSystem)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        particleSystem->particles[i].position = (Vector2){GetRandomValue(particleSystem->area.x, particleSystem->area.x + particleSystem->area.width), GetRandomValue(particleSystem->area.y, particleSystem->area.y + particleSystem->area.height)};
        particleSystem->particles[i].color = particleSystem->color;
        particleSystem->particles[i].alpha = 1.0f;
        particleSystem->particles[i].radius = particleSystem->radius;
        particleSystem->particles[i].active = true;
    }
}

void updateParticleSystem(Emitter *emitter)
{

    emitter->active = false;
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (!emitter->particleSys.particles[i].active)
            continue;
        emitter->active = true;
        emitter->particleSys.particles[i].position.x += GetRandomValue(-3, 3);
        emitter->particleSys.particles[i].position.y += emitter->particleSys.gravity;
        emitter->particleSys.particles[i].alpha -= emitter->particleSys.fade;
        if (emitter->particleSys.particles[i].alpha <= 0.0f)
            emitter->particleSys.particles[i].active = false;
    }
}

void drawParticleSystem(ParticleSystem particleSys)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (!particleSys.particles[i].active)
            continue;
        DrawRectangle(particleSys.particles[i].position.x, particleSys.particles[i].position.y, particleSys.radius, particleSys.radius, Fade(particleSys.particles[i].color, particleSys.particles[i].alpha));
        // DrawCircle(particleSystem->particles[i].position.x, particleSystem->particles[i].position.y, particleSystem->particles[i].radius, Fade(particleSystem->particles[i].color, particleSystem->particles[i].alpha));
    }
}

void drawHearts(Texture2D spriteSheet, Rectangle srcRect, float x, float y, int lives)
{
    Rectangle heart;
    for (int i = 0; i < 3; i++)
    {
        if (lives - i > 0)
        {
            heart = srcRect;
        }
        else
        {
            heart = (Rectangle){srcRect.x, srcRect.y + 32 * 2, srcRect.width, srcRect.height};
        }
        Vector2 pos = {x + (heart.width + 2) * i, y};
        DrawTextureRec(spriteSheet, heart, pos, WHITE);
    }
}

void debugPrint(int val, int x, int y)
{
    DrawText(TextFormat("%d", val), x, y, 30, WHITE);
}

Rectangle getRect(float x, float y, int width, int height)
{
    return (Rectangle){x - width / 2, y - height / 2, width, height};
}