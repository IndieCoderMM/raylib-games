#include "raylib.h"

#define ROWS 4
#define COLS 13
#define BALL_SPEED 300
#define PAD_SPEED 700
#define WALL_TYPES 5

typedef struct Ball
{
    float x, y;
    float speedX, speedY;
    float radius;
    Rectangle sourceRect;
    Texture2D texture;
} Ball;

typedef struct Paddle
{
    float x, y;
    float speed;
    float width, height;
    Rectangle collisionRect;
    Rectangle sourceRect;
    Texture2D texture;
} Paddle;

typedef struct Brick
{
    float x, y;
    float width, height;
    Rectangle collisionRect;
    Rectangle sourceRect;
    Texture2D texture;
    int health;
    int tier;
    bool broken;
} Brick;

Rectangle getRect(float x, float y, int width, int height);
void updateBall(Ball *ball);
void resetBall(Ball *ball, float x, float y);
void drawBricks(Brick bricks[ROWS][COLS], int rows, int cols);
void initBricks(Brick bricks[ROWS][COLS], Texture2D texture, int level);
void paddleCollision(Ball *ball, Paddle *paddle);
void brickCollisions(Ball *ball, Brick bricks[ROWS][COLS], int *score);
void drawHearts(Texture2D heart, float x, float y, int lives);

void debugPrint(int val, int x, int y);

int main(void)
{
    const int screenWidth = 900;
    const int screenHeight = 550;

    InitWindow(screenWidth, screenHeight, "Wall Break");

    SetTargetFPS(60);

    int level = 1;
    int lives = 3;
    int score = 0;
    bool playing = false;
    Color buttons[] = {BLACK, BLACK};
    bool activeBtn = false;
    bool victory = false;

    Ball ball;
    ball.x = screenWidth / 2;
    ball.y = screenHeight - 150;

    ball.speedX = 0;
    ball.speedY = 0;
    ball.texture = LoadTexture("assets/ball_break.png");
    ball.radius = ball.texture.width / 2;
    ball.sourceRect = (Rectangle){0, ball.texture.height / 4 * 3, ball.radius * 2, ball.radius * 2};

    Paddle paddle;
    paddle.x = screenWidth / 2;
    paddle.y = screenHeight - 80;

    paddle.speed = PAD_SPEED;
    paddle.texture = LoadTexture("assets/paddle_break.png");
    paddle.width = paddle.texture.width;

    paddle.height = paddle.texture.height / 4;
    paddle.sourceRect = (Rectangle){0, paddle.height * 2, paddle.width, paddle.height};

    Texture2D brickTex = LoadTexture("assets/walls_break.png");
    Brick bricks[ROWS][COLS] = {0};
    initBricks(bricks, brickTex, level);

    Texture2D heart = LoadTexture("assets/hearts.png");

    // Main game loop
    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_R))
            initBricks(bricks, brickTex, level);
        if (!playing)
        {
            // Start State
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN))
            {
                activeBtn = !activeBtn;
            }
            if (activeBtn)
            {
                buttons[0] = BLACK;
                buttons[1] = RED;
            }
            else
            {
                buttons[0] = RED;
                buttons[1] = BLACK;
            }

            if (IsKeyPressed(KEY_SPACE) && !activeBtn)
                playing = true;
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
                    initBricks(bricks, brickTex, level);
                }
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

            paddleCollision(&ball, &paddle);
            brickCollisions(&ball, bricks, &score);
            updateBall(&ball);

            if (ball.y >= screenHeight)
            {
                lives--;
                resetBall(&ball, screenWidth / 2, screenHeight - 140);
            }
            // victory will be false if a brick left
            victory = true;
            for (int i = 0; i < ROWS; i++)
            {
                for (int j = 0; j < COLS; j++)
                {
                    if (!(bricks[i][j].broken))
                        victory = false;
                }
            }
        }
        BeginDrawing();

        ClearBackground(BLACK);
        if (!playing)
        {
            ClearBackground(RAYWHITE);
            DrawText("Break-it", screenWidth / 2 - MeasureText("Break-it", 100) / 2, 10, 100, BLUE);
            DrawText("Start", screenWidth / 2 - MeasureText("Start", 60) / 2, screenHeight / 2, 60, buttons[0]);
            DrawText("Highscores", screenWidth / 2 - MeasureText("Highscores", 60) / 2, screenHeight / 2 + 100, 60, buttons[1]);
            EndDrawing();
            continue;
        }
        if (lives <= 0)
        {
            ClearBackground(RAYWHITE);
            DrawText("Game Over!", screenWidth / 2 - MeasureText("Game Over!", 100) / 2, screenHeight / 2, 100, RED);
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

        // Draw Bricks
        drawBricks(bricks, ROWS, COLS);
        DrawText(TextFormat("Level: %d", level), screenWidth / 2 - MeasureText("Level: 1", 100) / 2, screenHeight / 2, 100, Fade(DARKGRAY, 0.2));
        DrawTextureRec(paddle.texture, paddle.sourceRect, (Vector2){paddle.x - paddle.width / 2, paddle.y - paddle.height / 2}, WHITE);
        DrawTextureRec(ball.texture, ball.sourceRect, (Vector2){ball.x - ball.radius, ball.y - ball.radius}, WHITE);
        // DrawText(TextFormat("Lives: %d", lives), screenWidth - 200, screenHeight - 50, 40, WHITE);
        DrawText(TextFormat("Scores: %d", score), 10, screenHeight - 50, 40, WHITE);
        drawHearts(heart, screenWidth - heart.width / 2 * 4, screenHeight - 50, lives);

        if (ball.speedX == 0 && ball.speedY == 0 && lives > 0)
        {
            DrawText("Press SPACE to start...", screenWidth / 2 - MeasureText("Press SPACE to start...", 30) / 2, screenHeight / 2, 30, GREEN);
        }

        EndDrawing();
    }

    CloseWindow(); // Close window and OpenGL context

    return 0;
}

Rectangle getRect(float x, float y, int width, int height)
{
    return (Rectangle){x - width / 2, y - height / 2, width, height};
}

void drawBricks(Brick bricks[ROWS][COLS], int rows, int cols)
{
    int index;
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            if (bricks[i][j].broken)
                continue;
            index = 3 - bricks[i][j].health;
            bricks[i][j].sourceRect = (Rectangle){bricks[i][j].width * index, bricks[i][j].height * bricks[i][j].tier, bricks[i][j].width, bricks[i][j].height};
            DrawRectangleRec(bricks[i][j].collisionRect, WHITE);
            DrawTextureRec(bricks[i][j].texture, bricks[i][j].sourceRect, (Vector2){bricks[i][j].x - bricks[i][j].width / 2, bricks[i][j].y - bricks[i][j].height / 2}, WHITE);
        }
    }
}

void initBricks(Brick bricks[ROWS][COLS], Texture2D texture, int level)
{
    float frameWidth = texture.width / 5;
    float frameHeight = texture.height / 6;
    int totalRows = level % ROWS;
    if (level >= ROWS)
        totalRows = ROWS;

    for (int i = 0; i < ROWS; i++)
    {
        int totalCols = GetRandomValue(COLS - 4, COLS);
        if (totalCols % 2 == 0)
            totalCols += 1;
        float margin = (GetScreenWidth() - frameWidth * totalCols) / 2;

        bool skipped = GetRandomValue(0, 1) > 0;
        bool alternate = GetRandomValue(0, 1) > 0;
        int maxTier = level - 1;
        if (maxTier > WALL_TYPES)
            maxTier = WALL_TYPES;
        int tiers[] = {GetRandomValue(0, maxTier), GetRandomValue(0, maxTier)};
        int colorIndex = 0;
        for (int j = 0; j < COLS; j++)
        {
            if ((skipped && j % 2) || j >= totalCols || i >= totalRows)
            {
                bricks[i][j].broken = true;
                continue;
            }
            else
                bricks[i][j].broken = false;
            bricks[i][j].width = frameWidth;
            bricks[i][j].height = frameHeight;
            bricks[i][j].x = bricks[i][j].width * j + bricks[i][j].width / 2 + margin;
            bricks[i][j].y = bricks[i][j].height * i + bricks[i][j].height / 2 + 10;
            bricks[i][j].collisionRect = getRect(bricks[i][j].x, bricks[i][j].y, bricks[i][j].width, bricks[i][j].height);

            if (alternate)
                colorIndex = (colorIndex + 1) % 2;
            bricks[i][j].tier = tiers[colorIndex];
            bricks[i][j].health = 3;
            bricks[i][j].texture = texture;
            bricks[i][j].sourceRect = (Rectangle){0, 0, bricks[i][j].width, bricks[i][j].height};
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

void paddleCollision(Ball *ball, Paddle *paddle)
{
    Vector2 center = {ball->x, ball->y};
    paddle->collisionRect = getRect(paddle->x, paddle->y, paddle->width, paddle->height);
    if (CheckCollisionCircleRec(center, ball->radius, paddle->collisionRect) && ball->y <= paddle->y)
    {
        // ball.speedX *= -1;
        ball->y = paddle->y - paddle->height / 2 - ball->radius;
        ball->speedY *= -1;
        if (ball->x < paddle->x && ball->speedX < 0)
        {
            ball->speedX = -BALL_SPEED + (paddle->x - ball->x) * -7;
        }
        else if (ball->x > paddle->x && ball->speedX > 0)
        {
            ball->speedX = BALL_SPEED + (ball->x - paddle->x) * 7;
        }
    }
}

void brickCollisions(Ball *ball, Brick bricks[ROWS][COLS], int *score)
{
    Vector2 center = {ball->x, ball->y};
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            if (bricks[i][j].broken)
                continue;

            if (CheckCollisionCircleRec(center, 2 * ball->radius, bricks[i][j].collisionRect))
            {
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
                    bricks[i][j].tier--;
                }

                if (ball->x < bricks[i][j].x - bricks[i][j].width / 2 && ball->speedX > 0)
                {
                    // ball->x = bricks[i][j].x - bricks[i][j].width / 2 - ball->radius;
                    ball->speedX *= -1;
                }
                else if (ball->x > bricks[i][j].x + bricks[i][j].width / 2 && ball->speedX < 0)
                {
                    // ball->x = bricks[i][j].x + bricks[i][j].width / 2 + ball->radius;
                    ball->speedX *= -1;
                }
                else if (ball->y < bricks[i][j].y - bricks[i][j].height / 2 && ball->speedY > 0)
                {
                    // Ball coming from above
                    // ball->y = bricks[i][j].y - bricks[i][j].height / 2 - ball->radius;
                    ball->speedY *= -1;
                }
                else
                {
                    // Ball below
                    // ball->y = bricks[i][j].y + bricks[i][j].height / 2 + ball->radius;
                    ball->speedY *= -1;
                }
            }
        }
    }
}

void drawHearts(Texture2D heart, float x, float y, int lives)
{
    Rectangle heartSource;
    for (int i = 0; i < 3; i++)
    {
        if (lives - i > 0)
        {
            heartSource = (Rectangle){0, 0, heart.width / 2, heart.height / 3};
        }
        else
        {
            heartSource = (Rectangle){0, heart.height / 3 * 2, heart.width / 2, heart.height / 3};
        }
        Vector2 pos = {x + heart.width / 2 * i, y};
        DrawTextureRec(heart, heartSource, pos, WHITE);
    }
}

void debugPrint(int val, int x, int y)
{
    DrawText(TextFormat("%d", val), x, y, 30, WHITE);
}