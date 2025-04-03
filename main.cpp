#include <SDL.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <ctime>
#include <bits/stdc++.h>
#include <fstream>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_SIZE = 40;
const int MAP_WIDTH = SCREEN_WIDTH / TILE_SIZE;
const int MAP_HEIGHT = SCREEN_HEIGHT / TILE_SIZE;

// Thêm các hằng số cho menu
const int MENU_WIDTH = 300;
const int MENU_HEIGHT = 200;
const SDL_Color MENU_COLOR = {50, 50, 50, 255};
const SDL_Color TEXT_COLOR = {255, 255, 255, 255};

class Wall {
public:
    int x, y;
    SDL_Rect rect;
    bool active;
    SDL_Texture* texture;

    Wall(int startX, int startY, SDL_Texture* tex, bool isActive = true)
        : x(startX), y(startY), active(isActive), texture(tex)
    {
        rect = {x, y, TILE_SIZE, TILE_SIZE};
    }

    void render(SDL_Renderer* renderer) {
        if(active) {
            if (texture) {
                // Vẽ texture nếu có
                SDL_RenderCopy(renderer, texture, NULL, &rect);
            } else {
                // Fallback: Vẽ màu nâu nếu không có texture
                SDL_SetRenderDrawColor(renderer, 150, 75, 0, 255);
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
};

class Bullet {
public:
    int x, y;
    int dx, dy;
    SDL_Rect rect;
    bool active;

    Bullet(int startX, int startY, int dirX, int dirY, bool isActive = true) {
        x = startX;
        y = startY;
        dx = dirX;
        dy = dirY;
        active = isActive;
        rect = {x, y, 10, 10}; // Square shape bullet
    }

    void move() {
        x += dx;
        y += dy;
        rect.x = x;
        rect.y = y;
        if (x < TILE_SIZE || x > SCREEN_WIDTH - TILE_SIZE ||
            y < TILE_SIZE || y > SCREEN_HEIGHT - TILE_SIZE) {
            active = false;
        }
    }

    void render(SDL_Renderer* renderer) {
        if (active) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderFillRect(renderer, &rect);
        }
    };
};

class PlayerTank {
public:
    int x, y;
    int dirX, dirY;
    SDL_Rect rect;
    vector<Bullet> bullets;
    SDL_Texture* texture;

    PlayerTank(int startX, int startY, SDL_Texture* tankTexture, int startDirX = 0, int startDirY = -1) {
        x = startX;
        y = startY;
        rect = {x, y, TILE_SIZE, TILE_SIZE};
        dirX = startDirX;
        dirY = startDirY; // Default direction up
        texture = tankTexture;
    }

    void move(int dx, int dy, const vector<Wall>& walls) {
        int newX = x + dx;
        int newY = y + dy;
        this->dirX = dx;
        this->dirY = dy;

        SDL_Rect newRect = {newX, newY, TILE_SIZE, TILE_SIZE};
        for(int i = 0; i < walls.size(); i++){
            if (walls[i].active && SDL_HasIntersection(&newRect, &walls[i].rect)) {
                return; // Prevent movememnt if colliding with a wall
            }
        }
        if (newX >= TILE_SIZE && newX <= SCREEN_WIDTH - TILE_SIZE * 2 &&
            newY >= TILE_SIZE && newY <= SCREEN_HEIGHT - TILE_SIZE * 2) {
            x = newX;
            y = newY;
            rect.x = x;
            rect.y = y;
        }
    }

    void shoot() {
        bullets.push_back(Bullet(x + TILE_SIZE / 2 - 5, y + TILE_SIZE / 2 - 5,
        this->dirX, this->dirY));
    }

    void updateBullets() {
        for (auto &bullet : bullets) {
            bullet.move();
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
        [](Bullet &b) { return !b.active; }), bullets.end());
    }

    void render(SDL_Renderer* renderer) {
        if (texture) {
            // Vẽ texture nếu có
            SDL_RenderCopy(renderer, texture, NULL, &rect);
        } else {
            // Fallback: Vẽ màu vàng nếu không có texture
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderFillRect(renderer, &rect);
        }
        // Vẽ đạn (giữ nguyên)
        for (auto &bullet : bullets) {
            bullet.render(renderer);
        }
    }
};

class EnemyTank {
public:
    int x, y;
    int dirX, dirY;
    int moveDelay, shootDelay;
    SDL_Rect rect;
    bool active;
    vector<Bullet> bullets;

    EnemyTank(int startX, int startY, int startDirX = 0, int startDirY = 1, bool isActive = true) {
        moveDelay = 15; // Delay for movement
        shootDelay = 5; // Delay for shooting
        x = startX;
        y = startY;
        rect = {x, y, TILE_SIZE, TILE_SIZE};
        dirX = startDirX;
        dirY = startDirY;
        active = isActive;
    }

    void move(const vector<Wall>& walls) {
        if (--moveDelay > 0) return;
        moveDelay = 15;
        int r = rand() % 4;
        if (r == 0) { // Up
            this->dirX = 0;
            this->dirY = -5;
        }
        else if (r == 1) { // Down
            this->dirX = 0;
            this->dirY = 5;
        }
        else if (r == 2) { // Left
            this->dirY = 0;
            this->dirX = -5;
        }
        else if (r == 3) { // Right
            this->dirY = 0;
            this->dirX = 5;
        }

        int newX = x + this->dirX;
        int newY = y + this->dirY;

        SDL_Rect newRect = { newX, newY, TILE_SIZE, TILE_SIZE };
        for (const auto& wall : walls) {
            if (wall.active && SDL_HasIntersection(&newRect, &wall.rect)) {
                return;
            }
        }

        if (newX >= TILE_SIZE && newX <= SCREEN_WIDTH - TILE_SIZE * 2 &&
            newY >= TILE_SIZE && newY <= SCREEN_HEIGHT - TILE_SIZE * 2) {
                x = newX;
                y = newY;
                rect.x = x;
                rect.y = y;
        }
    }

    void shoot() {
        if (--shootDelay > 0) return;
        shootDelay = 5;
        bullets.push_back(Bullet(x + TILE_SIZE / 2 - 5, y + TILE_SIZE / 2 - 5,
        this->dirX, this->dirY));
    }

    void updateBullets() {
        for (auto &bullet : bullets) {
            bullet.move();
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
        [](Bullet &b) { return !b.active; }), bullets.end());
    }

    void render(SDL_Renderer* renderer) {
        if (active) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderFillRect(renderer, &rect);
            for (auto &bullet : bullets) {
                bullet.render(renderer);
            }
        }
    };
};

class Game {
public:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    bool inMenu;
    bool gamePaused;
    vector<Wall> walls;
    PlayerTank player;
    int enemyNumber = 3;
    vector<EnemyTank> enemies;
    SDL_Texture* wallTexture;
    SDL_Texture* playerTexture;
    Mix_Music* backgroundMusic;

    // Constructor
    Game(): player(((MAP_WIDTH - 1) / 2) * TILE_SIZE, (MAP_HEIGHT - 2) * TILE_SIZE, nullptr) {
        running = true;
        inMenu = true;
        gamePaused = false;

        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << endl;
            running = false;
        }

        window = SDL_CreateWindow("Battle City", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        if (!window) {
            cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << endl;
            running = false;
        }

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << endl;
            running = false;
        }

        // Load texture sau khi có renderer
        wallTexture = IMG_LoadTexture(renderer, "assets/wall.png");
        playerTexture = IMG_LoadTexture(renderer, "assets/player_tank.png");

        player.texture = playerTexture;
        // Kiểm tra lỗi
        if (!wallTexture || !playerTexture) {
            std::cerr << "Warning: Failed to load textures! Using fallback colors.\n";
        }

        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            cerr << "SDL_mixer could not initialize! Error: " << Mix_GetError() << endl;
            running = false;
        }

        backgroundMusic = Mix_LoadMUS("assets/background.mp3");
            if (!backgroundMusic) {
            cerr << "Failed to load background music! Error: " << Mix_GetError() << endl;
        }

        if (TTF_Init() == -1) {
            cerr << "SDL_ttf could not initialize! Error: " << TTF_GetError() << endl;
            running = false;
        }

        // Load font (thay đổi đường dẫn tới file font của bạn)
        TTF_Font* font = TTF_OpenFont("assets/font.ttf", 24);
            if (!font) {
            cerr << "Failed to load font! Error: " << TTF_GetError() << endl;
        }


        generateWalls();
        spawnEnemies();
    }

    // Hàm hiển thị menu
    void renderMenu() {
        // Vẽ nền menu
        SDL_Rect menuRect = {(SCREEN_WIDTH - MENU_WIDTH) / 2,
                             (SCREEN_HEIGHT - MENU_HEIGHT) / 2,
                             MENU_WIDTH, MENU_HEIGHT};
        SDL_SetRenderDrawColor(renderer, MENU_COLOR.r, MENU_COLOR.g, MENU_COLOR.b, MENU_COLOR.a);
        SDL_RenderFillRect(renderer, &menuRect);

        // Vẽ các nút (đơn giản chỉ là text)
        // Ở đây cần thêm SDL_ttf để hiển thị text đẹp hơn, nhưng để đơn giản tôi chỉ vẽ các hình chữ nhật
        SDL_Rect newGameBtn = {menuRect.x + 50, menuRect.y + 30, 200, 40};
        SDL_Rect loadGameBtn = {menuRect.x + 50, menuRect.y + 80, 200, 40};
        SDL_Rect exitBtn = {menuRect.x + 50, menuRect.y + 130, 200, 40};

        SDL_SetRenderDrawColor(renderer, 70, 70, 70, 255);
        SDL_RenderFillRect(renderer, &newGameBtn);
        SDL_RenderFillRect(renderer, &loadGameBtn);
        SDL_RenderFillRect(renderer, &exitBtn);

        // Cần thêm SDL_ttf để hiển thị text
        // Đây chỉ là minh họa, bạn nên thêm thư viện SDL_ttf để hiển thị text đẹp hơn
    }

    // Hàm xử lý sự kiện menu
    void handleMenuEvents(SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            int x, y;
            SDL_GetMouseState(&x, &y);

            SDL_Rect menuRect = {(SCREEN_WIDTH - MENU_WIDTH) / 2,
                                (SCREEN_HEIGHT - MENU_HEIGHT) / 2,
                                MENU_WIDTH, MENU_HEIGHT};

            // Kiểm tra click vào nút New Game
            SDL_Rect newGameBtn = {menuRect.x + 50, menuRect.y + 30, 200, 40};
            if (x >= newGameBtn.x && x <= newGameBtn.x + newGameBtn.w &&
                y >= newGameBtn.y && y <= newGameBtn.y + newGameBtn.h) {
                inMenu = false;
                resetGame();
            }

            // Kiểm tra click vào nút Load Game
            SDL_Rect loadGameBtn = {menuRect.x + 50, menuRect.y + 80, 200, 40};
            if (x >= loadGameBtn.x && x <= loadGameBtn.x + loadGameBtn.w &&
                y >= loadGameBtn.y && y <= loadGameBtn.y + loadGameBtn.h) {
                inMenu = false;
                loadGame();
            }

            // Kiểm tra click vào nút Exit
            SDL_Rect exitBtn = {menuRect.x + 50, menuRect.y + 130, 200, 40};
            if (x >= exitBtn.x && x <= exitBtn.x + exitBtn.w &&
                y >= exitBtn.y && y <= exitBtn.y + exitBtn.h) {
                running = false;
            }
        }
    }

    // Hàm lưu game
    void saveGame() {
        ofstream saveFile("savegame.dat", ios::binary);
        if (!saveFile) {
            cerr << "Cannot open save file!" << endl;
            return;
        }

        // Lưu trạng thái player
        saveFile.write(reinterpret_cast<char*>(&player.x), sizeof(player.x));
        saveFile.write(reinterpret_cast<char*>(&player.y), sizeof(player.y));
        saveFile.write(reinterpret_cast<char*>(&player.dirX), sizeof(player.dirX));
        saveFile.write(reinterpret_cast<char*>(&player.dirY), sizeof(player.dirY));

        // Lưu bullets của player
        size_t playerBulletCount = player.bullets.size();
        saveFile.write(reinterpret_cast<char*>(&playerBulletCount), sizeof(playerBulletCount));
        for (auto& bullet : player.bullets) {
            saveFile.write(reinterpret_cast<char*>(&bullet.x), sizeof(bullet.x));
            saveFile.write(reinterpret_cast<char*>(&bullet.y), sizeof(bullet.y));
            saveFile.write(reinterpret_cast<char*>(&bullet.dx), sizeof(bullet.dx));
            saveFile.write(reinterpret_cast<char*>(&bullet.dy), sizeof(bullet.dy));
            saveFile.write(reinterpret_cast<char*>(&bullet.active), sizeof(bullet.active));
        }

        // Lưu walls
        size_t wallCount = walls.size();
        saveFile.write(reinterpret_cast<char*>(&wallCount), sizeof(wallCount));
        for (auto& wall : walls) {
            saveFile.write(reinterpret_cast<char*>(&wall.x), sizeof(wall.x));
            saveFile.write(reinterpret_cast<char*>(&wall.y), sizeof(wall.y));
            saveFile.write(reinterpret_cast<char*>(&wall.active), sizeof(wall.active));
        }

        // Lưu enemies
        size_t enemyCount = enemies.size();
        saveFile.write(reinterpret_cast<char*>(&enemyCount), sizeof(enemyCount));
        for (auto& enemy : enemies) {
            saveFile.write(reinterpret_cast<char*>(&enemy.x), sizeof(enemy.x));
            saveFile.write(reinterpret_cast<char*>(&enemy.y), sizeof(enemy.y));
            saveFile.write(reinterpret_cast<char*>(&enemy.dirX), sizeof(enemy.dirX));
            saveFile.write(reinterpret_cast<char*>(&enemy.dirY), sizeof(enemy.dirY));
            saveFile.write(reinterpret_cast<char*>(&enemy.active), sizeof(enemy.active));

            // Lưu bullets của enemy
            size_t enemyBulletCount = enemy.bullets.size();
            saveFile.write(reinterpret_cast<char*>(&enemyBulletCount), sizeof(enemyBulletCount));
            for (auto& bullet : enemy.bullets) {
                saveFile.write(reinterpret_cast<char*>(&bullet.x), sizeof(bullet.x));
                saveFile.write(reinterpret_cast<char*>(&bullet.y), sizeof(bullet.y));
                saveFile.write(reinterpret_cast<char*>(&bullet.dx), sizeof(bullet.dx));
                saveFile.write(reinterpret_cast<char*>(&bullet.dy), sizeof(bullet.dy));
                saveFile.write(reinterpret_cast<char*>(&bullet.active), sizeof(bullet.active));
            }
        }

        saveFile.close();
        cout << "Game saved successfully!" << endl;
    }

    // Hàm load game
    void loadGame() {
        ifstream loadFile("savegame.dat", ios::binary);
        if (!loadFile) {
            cerr << "Cannot open load file!" << endl;
            resetGame();
            return;
        }

        // Xóa các đối tượng hiện tại
        walls.clear();
        enemies.clear();
        player.bullets.clear();

        // Load player
        loadFile.read(reinterpret_cast<char*>(&player.x), sizeof(player.x));
        loadFile.read(reinterpret_cast<char*>(&player.y), sizeof(player.y));
        loadFile.read(reinterpret_cast<char*>(&player.dirX), sizeof(player.dirX));
        loadFile.read(reinterpret_cast<char*>(&player.dirY), sizeof(player.dirY));
        player.rect = {player.x, player.y, TILE_SIZE, TILE_SIZE};

        // Load player bullets
        size_t playerBulletCount;
        loadFile.read(reinterpret_cast<char*>(&playerBulletCount), sizeof(playerBulletCount));
        for (size_t i = 0; i < playerBulletCount; ++i) {
            int x, y, dx, dy;
            bool active;
            loadFile.read(reinterpret_cast<char*>(&x), sizeof(x));
            loadFile.read(reinterpret_cast<char*>(&y), sizeof(y));
            loadFile.read(reinterpret_cast<char*>(&dx), sizeof(dx));
            loadFile.read(reinterpret_cast<char*>(&dy), sizeof(dy));
            loadFile.read(reinterpret_cast<char*>(&active), sizeof(active));
            player.bullets.push_back(Bullet(x, y, dx, dy, active));
        }

        // Load walls
        size_t wallCount;
        loadFile.read(reinterpret_cast<char*>(&wallCount), sizeof(wallCount));
        for (size_t i = 0; i < wallCount; ++i) {
            int x, y;
            bool active;
            loadFile.read(reinterpret_cast<char*>(&x), sizeof(x));
            loadFile.read(reinterpret_cast<char*>(&y), sizeof(y));
            loadFile.read(reinterpret_cast<char*>(&active), sizeof(active));
            walls.emplace_back(x, y, wallTexture, active);
        }

        // Load enemies
        size_t enemyCount;
        loadFile.read(reinterpret_cast<char*>(&enemyCount), sizeof(enemyCount));
        for (size_t i = 0; i < enemyCount; ++i) {
            int x, y, dirX, dirY;
            bool active;
            loadFile.read(reinterpret_cast<char*>(&x), sizeof(x));
            loadFile.read(reinterpret_cast<char*>(&y), sizeof(y));
            loadFile.read(reinterpret_cast<char*>(&dirX), sizeof(dirX));
            loadFile.read(reinterpret_cast<char*>(&dirY), sizeof(dirY));
            loadFile.read(reinterpret_cast<char*>(&active), sizeof(active));

            EnemyTank enemy(x, y, dirX, dirY, active);

            // Load enemy bullets
            size_t enemyBulletCount;
            loadFile.read(reinterpret_cast<char*>(&enemyBulletCount), sizeof(enemyBulletCount));
            for (size_t j = 0; j < enemyBulletCount; ++j) {
                int x, y, dx, dy;
                bool active;
                loadFile.read(reinterpret_cast<char*>(&x), sizeof(x));
                loadFile.read(reinterpret_cast<char*>(&y), sizeof(y));
                loadFile.read(reinterpret_cast<char*>(&dx), sizeof(dx));
                loadFile.read(reinterpret_cast<char*>(&dy), sizeof(dy));
                loadFile.read(reinterpret_cast<char*>(&active), sizeof(active));
                enemy.bullets.push_back(Bullet(x, y, dx, dy, active));
            }

            enemies.push_back(enemy);
        }

        loadFile.close();
        cout << "Game loaded successfully!" << endl;
    }

    // Hàm reset game
    void resetGame() {
        walls.clear();
        enemies.clear();
        player.bullets.clear();

        generateWalls();
        player = PlayerTank(((MAP_WIDTH - 1) / 2) * TILE_SIZE, (MAP_HEIGHT - 2) * TILE_SIZE, playerTexture);
        spawnEnemies();

        if (backgroundMusic) {
            Mix_PlayMusic(backgroundMusic, -1);  // -1 = lặp vô hạn
        }
    }

    void render() {
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255); // boundaries
        SDL_RenderClear(renderer); // delete color

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        for (int i = 1; i < MAP_HEIGHT - 1; ++i) {
            for (int j = 1; j < MAP_WIDTH - 1; ++j) {
                SDL_Rect tile = { j * TILE_SIZE, i * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                SDL_RenderFillRect(renderer, &tile);
            }
        }

        for(int i = 0; i < walls.size(); i++) {
            walls[i].render(renderer);
        }

        player.render(renderer);

        for (auto &enemy : enemies) {
            enemy.render(renderer);
        }

        SDL_RenderPresent(renderer);
    }

    void generateWalls(){
            for(int i = 3; i < MAP_HEIGHT - 3; i += 2) {
            for(int j = 3; j < MAP_WIDTH - 3; j += 2) {
                walls.push_back(Wall(j * TILE_SIZE, i * TILE_SIZE, wallTexture, true));
            }
        }
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        player.move(0, -5, walls);
                        break;
                    case SDLK_DOWN:
                        player.move(0, 5, walls);
                        break;
                    case SDLK_LEFT:
                        player.move(-5, 0, walls);
                        break;
                    case SDLK_RIGHT:
                        player.move(5, 0, walls);
                        break;
                    case SDLK_SPACE:
                        player.shoot();
                        break;
                    case SDLK_s: // Nhấn 's' để lưu game
                        saveGame();
                        break;
                    case SDLK_l: // Nhấn 'l' để load game
                        loadGame();
                        break;
                    case SDLK_p: // Nhấn 'p' để pause game
                        gamePaused = !gamePaused;
                        if (gamePaused) {
                            Mix_PauseMusic();  // Tạm dừng nhạc
                        } else {
                            Mix_ResumeMusic();  // Tiếp tục nhạc
                        }
                        break;
                    case SDLK_ESCAPE: // Nhấn ESC để vào menu
                        inMenu = true;
                        break;
                }
            }
        }
    }

    void update() {
        if (gamePaused || inMenu) return;

        player.updateBullets();

        for (auto& bullet : player.bullets) {
            for (auto& enemy : enemies) {
                if (enemy.active && SDL_HasIntersection(&bullet.rect, &enemy.rect)) {
                    enemy.active = false;
                    bullet.active = false;
                    break;
                }
            }
        }

        for(auto& enemy : enemies) {
            enemy.move(walls);
            enemy.updateBullets();
            if (rand() % 100 < 2) {
                enemy.shoot();
            }
        }

        for (auto& bullet : player.bullets){
            for (auto& wall : walls) {
                if (wall.active && SDL_HasIntersection(&bullet.rect, &wall.rect)) {
                    wall.active = false;
                    bullet.active = false;
                    break;
                }
            }
        }

        for (auto& bullet : player.bullets) {
            for (auto& wall : walls) {
                if (wall.active && SDL_HasIntersection(&bullet.rect, &wall.rect)) {
                    wall.active = false;
                    bullet.active = false;
                    break;
                }
            }
        }

        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
                [](EnemyTank &e) { return !e.active; } ),enemies.end());
        if (enemies.empty()) {
            running = false;
        }

        for (auto& enemy : enemies) {
            for (auto& bullet : enemy.bullets) {
                if (SDL_HasIntersection(&bullet.rect, &player.rect)) {
                    running = false;
                    return;
                }
            }
        }
    }

    void spawnEnemies() {
        enemies.clear();
        for (int i = 0; i < enemyNumber; ++i) {
            int ex, ey;
            bool validPosition = false;
            while (!validPosition) {
                ex = (rand() % (MAP_WIDTH - 2) + 1) * TILE_SIZE;
                ey = (rand() % (MAP_HEIGHT - 2) + 1) * TILE_SIZE;
                validPosition = true;
                for (const auto& wall : walls) {
                    if (wall.active && wall.x == ex && wall.y == ey) {
                        validPosition = false;
                        break;
                    }
                }
            }
            enemies.push_back(EnemyTank(ex, ey));
        }
    }

    void run() {
        while (running) {
            if (inMenu) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        running = false;
                    }
                    handleMenuEvents(event);
                }

                // Render menu
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                renderMenu();
                SDL_RenderPresent(renderer);
            } else {
                handleEvents();
                if (!gamePaused) {
                    update();
                }
                render();
            }
            SDL_Delay(16);
        }
    }

    ~Game() {
        if (backgroundMusic) {
            Mix_FreeMusic(backgroundMusic);  // Giải phóng nhạc
        }
        Mix_CloseAudio();
        if (wallTexture) SDL_DestroyTexture(wallTexture);
        if (playerTexture) SDL_DestroyTexture(playerTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main(int argc, char* argv[]) {
    srand(time(NULL));
    Game game;
    if (game.running) {
        game.run();
    }
    return 0;
}
