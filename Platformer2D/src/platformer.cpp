#include <raylib.h>
#include <raymath.h>

#define ROW 10
#define COL 20
#define TILESIZE 128.0f
#define P_VEL 200.0f
#define JUMP_FORCE 300.0f
#define G 400

typedef struct Tile
{
    int id;
    bool isBlock;
    Rectangle collisionRect;
} Tile;

class Player
{
public:
    Vector2 position = {0.0f, 0.0f};
    float speed = P_VEL;
    bool jumping = false;
    float size = 64;
    Texture2D spriteSheet = {0};
    float frameWidth = 0.0f;
    float frameHeight = 0.0f;
    Rectangle currentFrame = {0};

    Player(float x, float y)
    {
        position.x = x;
        position.y = y;
    }

    void loadSpriteSheet(const char *filename);

    void moveAndCollideY(Tile tileMap[ROW][COL], float dy);

    void moveAndCollideX(int tileData[ROW][COL], float dx);

    Rectangle getCollisionRect();

    void draw()
    {
        DrawTextureRec(spriteSheet, currentFrame, (Vector2){position.x - size / 2, position.y - size / 2}, WHITE);
    }

    void update(float delta);
};

Rectangle Player::getCollisionRect()
{
    return (Rectangle){position.x, position.y, size, size};
}

void Player::update(float delta)
{
    if (IsKeyDown(KEY_LEFT))
        position.x -= speed * delta;
    if (IsKeyDown(KEY_RIGHT))
        position.x += speed * delta;

    // if (IsKeyDown(KEY_SPACE) && !jumping)
    // {
    //     speed = -JUMP_FORCE;
    //     jumping = true;
    // }
}

void Player::moveAndCollideY(Tile tileMap[ROW][COL], float dy)
{
    position.y += dy;
    for (int i = 0; i < ROW; i++)
    {
        for (int j = 0; j < COL; j++)
        {
            if (!tileMap[i][j].isBlock)
                continue;
            if (CheckCollisionRecs(getCollisionRect(), tileMap[i][j].collisionRect))
            {
                position.y = tileMap[i][j].collisionRect.y - size;
            }
        }
    }
}

void Player::loadSpriteSheet(const char *filename)
{
    spriteSheet = LoadTexture(filename);
    frameWidth = spriteSheet.width / 4;
    frameHeight = spriteSheet.height / 2;
    currentFrame = (Rectangle){0, 0, frameWidth, frameHeight};
}

void generateTileMap(Tile tileMap[ROW][COL]);
void drawTileMap(Tile tilemap[ROW][COL], Texture2D tileSet);
void updateCamera(Camera2D *camera, Player *player, int width, int height, float delta);

void updateCameraBoundsPush(Camera2D *camera, Player *player, int width, int height, float delta);

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    Tile tileMap[ROW][COL];

    InitWindow(screenWidth, screenHeight, "Platformer 2D");

    Texture2D tileSet = LoadTexture("Platformer2D/resources/image/tilesets/platformTilesheet.png");

    Player player(screenWidth / 2, 10);
    player.loadSpriteSheet("Platformer2D/resources/image/characters/gameBoySheet.png");

    Camera2D camera = {0};
    camera.target = player.position;
    camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    generateTileMap(tileMap);

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        player.update(dt);
        player.moveAndCollideY(tileMap, G * dt);
        updateCameraBoundsPush(&camera, &player, screenWidth, screenHeight, dt);

        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode2D(camera);
        // DrawRectangle(screenWidth / 2, screenHeight / 2, 100, 100, RED);
        // for (int i = 0; i < 20; i++)
        // {
        //     DrawTextureRec(tileMap, grassTile, (Vector2){(float)tileSize * i, 100}, WHITE);
        // }
        drawTileMap(tileMap, tileSet);
        DrawRectangleRec(player.getCollisionRect(), GREEN);
        player.draw();
        EndMode2D();
        EndDrawing();
    }

    CloseWindow();

    return 0;
}

void updateCamera(Camera2D *camera, Player *player, int width, int height, float delta)
{
    static float minSpeed = 30;
    static float minEffectLength = 10;
    static float fractionSpeed = 0.8f;

    camera->offset = (Vector2){width / 2.0f, height / 2.0f};
    Vector2 diff = Vector2Subtract(player->position, camera->target);
    float length = Vector2Length(diff);

    if (length > minEffectLength)
    {
        float speed = fmaxf(fractionSpeed * length, minSpeed);
        camera->target = Vector2Add(camera->target, Vector2Scale(diff, speed * delta / length));
    }
}

void updateCameraBoundsPush(Camera2D *camera, Player *player, int width, int height, float delta)
{
    static Vector2 bbox = {0.2f, 0.2f};

    Vector2 bboxWorldMin = GetScreenToWorld2D((Vector2){
                                                  (1 - bbox.x) * 0.5f * width, (1 - bbox.y) * 0.5f * height},
                                              *camera);

    Vector2 bboxWorldMax = GetScreenToWorld2D((Vector2){(1 + bbox.x) * 0.5f * width, (1 + bbox.y) * 0.5f * height}, *camera);
    camera->offset = (Vector2){(1 - bbox.x) * 0.5f * width, (1 - bbox.y) * 0.5f * height};

    if (player->position.x < bboxWorldMin.x)
        camera->target.x = player->position.x;
    else if (player->position.y < bboxWorldMin.y)
        camera->target.y = player->position.y;
    else if (player->position.x > bboxWorldMax.x)
        camera->target.x = bboxWorldMin.x + (player->position.x - bboxWorldMax.x);
    else if (player->position.y > bboxWorldMax.y)
        camera->target.y = bboxWorldMin.y + (player->position.y - bboxWorldMin.y);
}

void generateTileMap(Tile tilemap[ROW][COL])
{

    for (int c = 0; c < COL; c++)
    {
        int rand = GetRandomValue(0, 4);

        for (int r = 0; r < ROW; r++)
        {
            if (rand == 0 || r < 1)
            {
                tilemap[r][c].isBlock = false;
                tilemap[r][c].id = -1;
                continue;
            }
            tilemap[r][c].isBlock = true;
            if (tilemap[r - 1][c].id == -1)
                tilemap[r][c].id = 0;
            else
                tilemap[r][c].id = 3;
            tilemap[r][c].collisionRect = (Rectangle){c * TILESIZE, r * TILESIZE, TILESIZE, TILESIZE};
        }
    }
}

void drawTileMap(Tile tilemap[ROW][COL], Texture2D tileSet)
{
    Rectangle srcRect = {0, 0, TILESIZE, TILESIZE};
    for (int r = 0; r < ROW; r++)
    {
        for (int c = 0; c < COL; c++)
        {
            if (tilemap[r][c].id == -1)
                continue;
            srcRect.x = TILESIZE * tilemap[r][c].id;

            DrawTextureRec(tileSet, srcRect, (Vector2){TILESIZE * c, TILESIZE * r}, WHITE);
        }
    }
}