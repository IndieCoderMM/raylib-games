#include "raylib.h"

#define MAX_ROWS 4
#define MAX_COLS 11
#define MAX_PARTICLES 100
#define BALL_SPEED 200
#define PAD_SPEED 700
#define SPRITE_SIZE 32
#define BRICK_TIER 5
#define BRICK_WIDTH 64
#define BRICK_HEIGHT 32

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

void initBricks(Brick bricks[MAX_ROWS][MAX_COLS], int level);
void initParticleSystem(ParticleSystem *particleSystem, Rectangle area, float radius, float gravity, float fade, Color color);
void updateBall(Ball *ball);
void resetBall(Ball *ball, float x, float y);
bool paddleCollision(Ball *ball, Paddle *paddle);
void brickCollisions(Ball *ball, Brick bricks[MAX_ROWS][MAX_COLS], int *score, ParticleSystem *particleSys);
void drawBricks(Brick bricks[MAX_ROWS][MAX_COLS], int rows, int cols, Texture2D spriteSheet, Rectangle srcRect);
void drawParticleSystem(ParticleSystem *particleSystem);
void updateParticleSystem(ParticleSystem *particleSystem);
void drawHearts(Texture2D spriteSheet, Rectangle srcRect, float x, float y, int lives);
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
    // const Rectangle srcRectBall = {383, spriteSheet.height - SPRITE_SIZE, SPRITE_SIZE, SPRITE_SIZE};
    // const Rectangle srcRectPaddle = {254, spriteSheet.height - SPRITE_SIZE * 3, SPRITE_SIZE * 3, SPRITE_SIZE};
    const Rectangle srcRectBrick = {spriteSheet.width - BRICK_WIDTH * BRICK_TIER, 0, BRICK_WIDTH, BRICK_HEIGHT};
    const Rectangle srcRectHeart = {704, 352, SPRITE_SIZE, SPRITE_SIZE};

    // Loading SFX
    // Sound fxBounce = LoadSound("assets/buttonfx.wav");

    int level = 6;
    int lives = 3;
    int score = 0;
    bool playing = false;
    Color buttons[] = {WHITE, WHITE};
    bool activeBtn = false;
    bool victory = false;

    ParticleSystem particleSystem = {0};

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
    paddle.srcRect = (Rectangle){254, spriteSheet.height - SPRITE_SIZE * 3, paddle.width, paddle.height};

    Brick bricks[MAX_ROWS][MAX_COLS] = {0};
    initBricks(bricks, level);

    SetTargetFPS(60);
    // Main game loop
    while (!WindowShouldClose())
    {
        // Debug Code
        if (IsKeyPressed(KEY_R))
            initBricks(bricks, level);

        if (!playing)
        {
            // Start State
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN))
            {
                activeBtn = !activeBtn;
            }
            if (activeBtn)
            {
                buttons[0] = WHITE;
                buttons[1] = GOLD;
            }
            else
            {
                buttons[0] = GOLD;
                buttons[1] = WHITE;
            }

            if (IsKeyPressed(KEY_SPACE) && !activeBtn)
            {
                playing = true;
                resetBall(&ball, paddle.x, paddle.y - paddle.height);
            }
        }
        else
        {
            if (IsKeyPressed(KEY_SPACE))
            {
                // Restart
                if (lives <= 0)
                {
                    lives = 3;
                }
                else if (ball.speedX == 0 && ball.speedY == 0)
                {
                    // Resume
                    ball.speedX = BALL_SPEED;
                    ball.speedY = -BALL_SPEED;
                }
                else if (victory)
                {
                    victory = false;
                    level++;
                    resetBall(&ball, screenWidth / 2, screenHeight - 150);
                    initBricks(bricks, level);
                }
            }
            if (ball.speedX == 0 && ball.speedY == 0)
            {
                resetBall(&ball, paddle.x, paddle.y - paddle.height);
            }

            // Paddle Control
            if (IsKeyDown(KEY_RIGHT))
            {
                paddle.x += paddle.speed * GetFrameTime();
            }
            else if (IsKeyDown(KEY_LEFT))
            {
                paddle.x -= paddle.speed * GetFrameTime();
            }
            if (paddle.x < paddle.width / 2)
                paddle.x = paddle.width / 2;
            else if (paddle.x > screenWidth - paddle.width / 2)
                paddle.x = screenWidth - paddle.width / 2;

            // if (paddleCollision(&ball, &paddle))
            //     PlaySound(fxBounce);
            paddleCollision(&ball, &paddle);
            brickCollisions(&ball, bricks, &score, &particleSystem);
            updateBall(&ball);
            updateParticleSystem(&particleSystem);

            if (ball.y >= screenHeight + SPRITE_SIZE)
            {
                lives--;
                resetBall(&ball, screenWidth / 2, screenHeight - 140);
            }
            // victory will be false if a brick left
            victory = true;
            for (int i = 0; i < MAX_ROWS; i++)
            {
                for (int j = 0; j < MAX_COLS; j++)
                {
                    if (!(bricks[i][j].broken))
                        victory = false;
                }
            }
        }

        BeginDrawing();

        ClearBackground(SKYBLUE);
        if (!playing)
        {
            ClearBackground(SKYBLUE);
            DrawText("Break-it", screenWidth / 2 - MeasureText("Break-it", 200) / 2 - 3, 13, 200, DARKGRAY);
            DrawText("Break-it", screenWidth / 2 - MeasureText("Break-it", 200) / 2, 10, 200, MAROON);
            DrawText("Start", screenWidth / 2 - MeasureText("Start", 60) / 2, screenHeight / 2, 60, buttons[0]);
            DrawText("Highscores", screenWidth / 2 - MeasureText("Highscores", 60) / 2, screenHeight / 2 + 100, 60, buttons[1]);

            EndDrawing();
            continue;
        }
        if (lives <= 0)
        {
            ClearBackground(RAYWHITE);
            DrawText("Game Over!", screenWidth / 2 - MeasureText("Game Over!", 200) / 2, screenHeight / 2, 200, MAROON);
            EndDrawing();
            continue;
        }

        if (victory)
        {
            ClearBackground(RAYWHITE);
            DrawText("Level Clear!", screenWidth / 2 - 50, 30, 30, BLUE);
            EndDrawing();
            continue;
        }

        drawBricks(bricks, MAX_ROWS, MAX_COLS, spriteSheet, srcRectBrick);
        DrawText(TextFormat("Level: %d", level), screenWidth / 2 - MeasureText("Level: 1", 150) / 2, screenHeight / 2, 150, Fade(WHITE, 0.2));
        DrawTextureRec(spriteSheet, paddle.srcRect, (Vector2){paddle.x - paddle.width / 2, paddle.y - paddle.height / 2}, WHITE);
        DrawTextureRec(spriteSheet, ball.srcRect, (Vector2){ball.x - ball.radius, ball.y - ball.radius}, WHITE);
        DrawText(TextFormat("Scores: %d", score), 10, screenHeight - 50, 40, WHITE);
        drawHearts(spriteSheet, srcRectHeart, screenWidth - srcRectHeart.width * 4, screenHeight - srcRectHeart.height - 20, lives);

        if (ball.speedX == 0 && ball.speedY == 0 && lives > 0)
        {
            DrawText("Press SPACE to start...", screenWidth / 2 - MeasureText("Press SPACE to start...", 50) / 2, screenHeight / 2, 50, DARKGREEN);
        }
        drawParticleSystem(&particleSystem);
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
            bricks[i][j].y = bricks[i][j].height * i + bricks[i][j].height / 2 + 10;
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
    if (CheckCollisionCircleRec(center, ball->radius, paddle->collisionRect) && ball->y <= paddle->y - paddle->height / 2)
    {
        // ball.speedX *= -1;
        ball->y = paddle->y - paddle->height / 2 - ball->radius;
        ball->speedY *= -1;
        if (ball->x < paddle->x && ball->speedX < 0)
        {
            ball->speedX = -BALL_SPEED + (paddle->x - ball->x) * -5;
        }
        else if (ball->x > paddle->x && ball->speedX > 0)
        {
            ball->speedX = BALL_SPEED + (ball->x - paddle->x) * 5;
        }
        return true;
    }
    return false;
}

void brickCollisions(Ball *ball, Brick bricks[MAX_ROWS][MAX_COLS], int *score, ParticleSystem *particleSys)
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

                initParticleSystem(particleSys, bricks[i][j].collisionRect, 10, 0.5, 0.05, colors[colorIndex]);
                return;
            }
        }
    }
    return;
}

void initParticleSystem(ParticleSystem *particleSystem, Rectangle area, float radius, float gravity, float fade, Color color)
{
    particleSystem->area = area;
    particleSystem->radius = radius;
    particleSystem->color = color;
    particleSystem->fade = fade;
    particleSystem->gravity = gravity;
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        particleSystem->particles[i].position = (Vector2){GetRandomValue(particleSystem->area.x, particleSystem->area.x + particleSystem->area.width), GetRandomValue(particleSystem->area.y, particleSystem->area.y + particleSystem->area.height)};
        particleSystem->particles[i].color = particleSystem->color;
        particleSystem->particles[i].alpha = 0.5f;
        particleSystem->particles[i].radius = particleSystem->radius;
        particleSystem->particles[i].active = true;
    }
}

void updateParticleSystem(ParticleSystem *particleSystem)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (!particleSystem->particles[i].active)
            continue;
        particleSystem->particles[i].position.x += GetRandomValue(-3, 3);
        particleSystem->particles[i].position.y += particleSystem->gravity;
        particleSystem->particles[i].alpha -= particleSystem->fade;
        if (particleSystem->particles[i].alpha <= 0.0f)
            particleSystem->particles[i].active = false;
    }
}

void drawParticleSystem(ParticleSystem *particleSystem)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (!particleSystem->particles[i].active)
            continue;
        DrawRectangle(particleSystem->particles[i].position.x, particleSystem->particles[i].position.y, particleSystem->radius, particleSystem->radius, Fade(particleSystem->particles[i].color, particleSystem->particles[i].alpha));
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
        Vector2 pos = {x + heart.width * i, y};
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