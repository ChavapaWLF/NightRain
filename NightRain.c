/*
 * 彩色雨夜荷塘 - 高级3D特效版
 * 一个美丽的夜雨荷塘景色模拟程序
 * 包含电闪雷鸣、动态天气和荷叶水珠效果
 */

#include <SDL2/SDL.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
 
// 窗口大小和模拟参数常量
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_RAINDROPS 1000              // 增加雨滴上限以支持暴雨场景
#define MAX_RIPPLES 500                 // 涟漪上限
#define MAX_SPLASHES 300                // 溅射水珠上限
#define MAX_LIGHTNING 5                 // 最大同时出现的闪电数
#define POND_HEIGHT (WINDOW_HEIGHT * 2 / 3)  // 荷塘位于窗口高度的2/3处
#define RIPPLE_LIFETIME 2000            // 涟漪生命周期（毫秒）
#define SPLASH_LIFETIME 800             // 溅射水珠生命周期（毫秒）
#define LIGHTNING_LIFETIME 500          // 闪电生命周期（毫秒）
#define RAINDROP_FALL_SPEED_MIN 200
#define RAINDROP_FALL_SPEED_MAX 500     // 增加最大下落速度
#define RIPPLE_SPEED 30                 // 涟漪扩散速度
#define STARS_COUNT 300                 // 星星数量
#define MOUNTAIN_COUNT 5                // 山的数量
#define REED_COUNT 20                   // 芦苇数量
#define LOTUS_PAD_COUNT 25              // 荷叶数量
#define LOTUS_FLOWER_COUNT 8            // 荷花数量
#define MAX_CLOUD_LAYERS 7              // cloud layer number

// 天气状态枚举
typedef enum {
    WEATHER_LIGHT_RAIN,    // 和风细雨
    WEATHER_MEDIUM_RAIN,   // 中雨
    WEATHER_HEAVY_RAIN,    // 暴风骤雨
    WEATHER_THUNDERSTORM,  // 雷暴
    WEATHER_COUNT          // 枚举计数
} WeatherState;

// 雨滴结构体
typedef struct {
    float x;              // X坐标
    float y;              // Y坐标
    float z;              // Z坐标 (0-1, 0=远, 1=近)
    float speed_y;        // 垂直下落速度
    float speed_x;        // 水平速度（受风影响）
    SDL_Color color;      // 雨滴颜色
    int size;             // 雨滴基础大小
    bool active;          // 雨滴是否激活
    bool in_water;        // 雨滴是否已入水
    Uint32 creation_time; // 雨滴创建时间
    Uint32 water_time;    // 雨滴入水时间
} Raindrop;

// 涟漪结构体
typedef struct {
    float x;              // X坐标
    float y;              // Y坐标
    float z;              // Z坐标 (0-1, 0=远, 1=近)
    float radius;         // 当前半径
    float max_radius;     // 最大半径
    SDL_Color color;      // 涟漪颜色
    Uint32 creation_time; // 涟漪创建时间
    bool active;          // 涟漪是否激活
} Ripple;

// 溅射水珠结构体 - 用于荷叶上的雨滴溅射效果
typedef struct {
    float x;              // X坐标
    float y;              // Y坐标
    float z;              // Z坐标 (0-1, 0=远, 1=近)
    float speed_x;        // X方向速度
    float speed_y;        // Y方向速度
    float size;           // 水珠大小
    SDL_Color color;      // 水珠颜色
    Uint32 creation_time; // 创建时间
    bool active;          // 是否激活
} Splash;

// 闪电结构体
typedef struct {
    int segments;         // 闪电段数
    int points[20][2];    // 闪电路径点 [seg][x/y]
    int width;            // 闪电宽度
    Uint8 brightness;     // 亮度
    Uint32 creation_time; // 创建时间
    int duration;         // 持续时间（毫秒）
    bool active;          // 是否激活
    int type;             // 闪电类型 (0=主闪电, 1=分支)
} Lightning;

// 星星结构体
typedef struct {
    int x;                // X坐标
    int y;                // Y坐标
    float z;              // Z坐标 (0-1, 0=远, 1=近)
    float brightness;     // 亮度
    float twinkle_speed;  // 闪烁速度
} Star;

// 山结构体
typedef struct {
    int x_offset;         // X偏移
    float z;              // Z深度 (0-1, 0=远, 1=近)
    int height;           // 山高
    int width;            // 山宽
    SDL_Color color;      // 山的颜色
} Mountain;

// 芦苇结构体
typedef struct {
    float x;              // X坐标
    float y;              // Y坐标底部位置
    float z;              // Z坐标 (0-1, 0=远, 1=近)
    int height;           // 高度
    float sway_offset;    // 摇摆偏移初始相位
    float sway_speed;     // 摇摆速度
} Reed;

// 荷叶结构体
typedef struct {
    float x;              // X坐标中心
    float y;              // Y坐标中心
    float z;              // Z坐标 (0-1, 0=远, 1=近)
    float radius;         // 荷叶半径
    float wave_phase;     // 波动相位
    float wave_speed;     // 波动速度
    float tilt_angle;     // 倾斜角度
    SDL_Color color;      // 颜色
    SDL_Texture *texture;
} LotusPad;

// 荷花结构体
typedef struct {
    float x;              // X坐标
    float y;              // Y坐标
    float z;              // Z坐标 (0-1, 0=远, 1=近)
    float size;           // 大小
    float sway_phase;     // 摇摆相位
    SDL_Color color;      // 颜色
    int petal_count;      // 花瓣数量
} LotusFlower;

/* struct to moniter performance */
typedef struct {
    Uint64 freq;           // 计时器频率
    Uint64 frame_start;    // 帧开始时间
    Uint64 frame_end;      // 帧结束时间
    double input_time;
    double frame_time;     // 本帧耗时（秒）
    double avg_frame_time; // 平均帧时间（滑动平均）
    double physics_time;   // 物理计算耗时
    double render_time;    // 渲染耗时
    int frame_count;       // 帧计数器
} PerformanceStats;

// 全局变量 global para
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
Raindrop raindrops[MAX_RAINDROPS];
Ripple ripples[MAX_RIPPLES];
Splash splashes[MAX_SPLASHES];
Lightning lightnings[MAX_LIGHTNING];
SDL_Texture *moon_texture = NULL; //use texture to improve performance
Star stars[STARS_COUNT];
Mountain mountains[MOUNTAIN_COUNT];
SDL_Texture *cloud_textures[MAX_CLOUD_LAYERS];  // use texture to improve performance
int cloud_offsets[MAX_CLOUD_LAYERS];    
Reed reeds[REED_COUNT];
LotusPad lotus_pads[LOTUS_PAD_COUNT];
LotusFlower lotus_flowers[LOTUS_FLOWER_COUNT];
PerformanceStats perf;

int raindrop_count = 0;
int ripple_count = 0;
int splash_count = 0;
int lightning_count = 0;
Uint32 last_raindrop_time = 0;
Uint32 last_lightning_time = 0;
int raindrop_interval = 100;            // 雨滴生成间隔（毫秒），会根据天气变化
float camera_x = 0.0f;                  // 摄像机X位置，用于视角移动
float camera_target_x = 0.0f;           // 摄像机目标X位置
bool camera_moving = false;             // 摄像机是否在移动

// 风和天气系统变量
float wind_strength = 0.0f;             // 风力强度 (-1.0 到 1.0)
float target_wind_strength = 0.0f;      // 目标风力强度
float wind_change_speed = 0.02f;        // 风力变化速度
WeatherState current_weather = WEATHER_LIGHT_RAIN;   // 当前天气状态
WeatherState target_weather = WEATHER_LIGHT_RAIN;    // 目标天气状态
Uint32 last_weather_change_time = 0;    // 上次天气变化时间
Uint32 weather_duration_min = 10000;    // 天气持续最短时间（毫秒）
Uint32 weather_duration_max = 30000;    // 天气持续最长时间（毫秒）
float rain_surface_ratio = 0.3f;        // 直接落在水面的雨点比例
int weather_intensity = 50;             // 天气剧烈程度 (0-100)
Uint32 last_thunder_time = 0;           // 上次雷声时间
bool thunder_active = false;            // 是否有雷声激活
Uint32 thunder_start_time = 0;          // 雷声开始时间
int thunder_duration = 0;               // 雷声持续时间

// 函数原型 function prototype
bool initialize();
void close();
void draw_crater(SDL_Surface* surface, int cx, int cy, int radius, Uint32 color);
void create_raindrop(bool on_surface);
void update_raindrops(Uint32 current_time, float delta_time);
void create_ripple(float x, float y, float z, SDL_Color color);
void update_ripples(Uint32 current_time);
void create_splash(float x, float y, float z, SDL_Color color);
void update_splashes(Uint32 current_time, float delta_time);
void create_lightning(int x, int y, int length, int width, int type);
void update_lightning(Uint32 current_time);
void initialize_moon();
void initialize_cloud();
void initialize_stars();
void initialize_mountains();
void initialize_reeds();
void generate_lotus_texture(LotusPad *pad);
void initialize_lotus_pads();
void render_lotus_texture(LotusPad *pad, int proj_x, float tilt);
void destroy_lotus_textures();
void initialize_lotus_flowers();
void update_stars(Uint32 current_time);
void update_lotus_pads(Uint32 current_time, float delta_time);
void update_lotus_flowers(Uint32 current_time, float delta_time);
void update_camera();
void update_weather_and_wind(Uint32 current_time);
void update_thunder(Uint32 current_time);
bool check_raindrop_lotus_collision(Raindrop* raindrop);
void render();
void render_weather_info();
SDL_Color get_random_color();
float get_z_scale(float z);    // 根据z坐标获取缩放比例
float project_x(float x, float z); // 根据z坐标投影x坐标
SDL_Color adjust_color_by_depth(SDL_Color color, float z); // 根据深度调整颜色
float get_rain_interval(WeatherState weather, int intensity); // 根据天气和强度获取雨滴间隔

int main(int argc, char* args[]) {
    /* allocate a terminal for this GUI program */
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    // 初始化随机数种子
    srand(time(NULL));
    
    // 初始化SDL和资源
    if (!initialize()) {
        printf("初始化失败!\n");
        printf("请确保已正确安装SDL2库并配置环境变量。\n");
        printf("如果您在Windows下使用VSCode，请参考之前的配置说明。\n");
        return -1;
    }
    
    // 显示使用说明
    printf("\nRainbow Rain in Nighty Pond\n");
    printf("============ Control Manual ============\n");
    printf("1: switch to light rain\n");
    printf("2: switch to moderate rain\n");
    printf("3: switch to heavy rain\n");
    printf("4: switch to thunder storm\n");
    printf("Up/Down: Inc/Dec weather intensity\n");
    printf("Left/Right: move camera Left/Right\n");
    printf("Home: reset camera\n");
    printf("Space: trigger thunder and lightning\n");
    printf("ESC: exit\n");
    printf("=================================\n\n");

    
    // 主循环标志
    bool quit = false;
    
    // 事件处理器
    SDL_Event e;
    
    // 设置上一次雨滴生成时间为当前时间
    last_raindrop_time = SDL_GetTicks();
    last_weather_change_time = SDL_GetTicks();
    last_lightning_time = SDL_GetTicks();
    last_thunder_time = SDL_GetTicks();
    
    // 初始化各种元素
    initialize_moon();
    initialize_cloud();
    initialize_stars();
    initialize_mountains();
    initialize_reeds();
    initialize_lotus_pads();
    initialize_lotus_flowers();
    
    // 时间跟踪
    Uint32 last_frame_time = SDL_GetTicks();
    
    // 主循环
    while (!quit) {
        /* record the time this frame starts */
        perf.frame_start = SDL_GetPerformanceCounter();

        // 当前时间和时间增量
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_frame_time) / 1000.0f;
        last_frame_time = current_time;
        
        /* ==== [1] tackle input events ====*/
        // 处理事件队列
        Uint64 input_start = SDL_GetPerformanceCounter();
        while (SDL_PollEvent(&e) != 0) {
            // 用户请求退出
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            // 用户按下按键
            else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                    case SDLK_1:
                        // 切换到和风细雨
                        target_weather = WEATHER_LIGHT_RAIN;
                        break;
                    case SDLK_2:
                        // 切换到中雨
                        target_weather = WEATHER_MEDIUM_RAIN;
                        break;
                    case SDLK_3:
                        // 切换到暴风骤雨
                        target_weather = WEATHER_HEAVY_RAIN;
                        break;
                    case SDLK_4:
                        // 切换到雷暴
                        target_weather = WEATHER_THUNDERSTORM;
                        break;
                    case SDLK_UP:
                        // 增加天气剧烈程度
                        weather_intensity += 10;
                        if (weather_intensity > 100) weather_intensity = 100;
                        break;
                    case SDLK_DOWN:
                        // 减少天气剧烈程度
                        weather_intensity -= 10;
                        if (weather_intensity < 0) weather_intensity = 0;
                        break;
                    case SDLK_SPACE:
                        // 手动触发闪电和雷声
                        if (current_weather >= WEATHER_HEAVY_RAIN) {
                            int x = WINDOW_WIDTH / 2 + (rand() % 300) - 150;
                            create_lightning(x, 0, 5 + rand() % 10, 2 + rand() % 3, 0);
                            thunder_active = true;
                            thunder_start_time = current_time;
                            thunder_duration = 1000 + rand() % 2000;
                        }
                        break;
                    case SDLK_LEFT:
                        // 向左移动视角
                        camera_target_x -= 100.0f;
                        camera_moving = true;
                        break;
                    case SDLK_RIGHT:
                        // 向右移动视角
                        camera_target_x += 100.0f;
                        camera_moving = true;
                        break;
                    case SDLK_HOME:
                        // 重置视角
                        camera_target_x = 0.0f;
                        camera_moving = true;
                        break;
                }
            }
        }
        perf.input_time = (SDL_GetPerformanceCounter() - input_start) * 1000.0 / perf.freq;
        
        /* ==== [2] unpdate physical system */
        Uint64 physics_start = SDL_GetPerformanceCounter();
        // 更新天气和风系统
        update_weather_and_wind(current_time);
        // 更新雷声
        update_thunder(current_time);
        // 如果达到生成间隔，创建新雨滴
        raindrop_interval = get_rain_interval(current_weather, weather_intensity);
        if (current_time - last_raindrop_time >= raindrop_interval) {
            // 根据rain_surface_ratio决定雨滴是直接落在水面还是从天空落下
            bool on_surface = ((float)rand() / RAND_MAX) < rain_surface_ratio;
            create_raindrop(on_surface);
            last_raindrop_time = current_time;
        } 
        // 在雷暴天气下，随机产生闪电
        if ((current_weather == WEATHER_THUNDERSTORM || 
             (current_weather == WEATHER_HEAVY_RAIN && weather_intensity > 70)) && 
            current_time - last_lightning_time > (10000 - weather_intensity * 80)) {            
            // 闪电出现概率随强度增加
            if (rand() % 100 < weather_intensity / 5) {
                int x = WINDOW_WIDTH / 2 + (rand() % 400) - 200;
                create_lightning(x, 0, 5 + rand() % 10, 2 + rand() % 3, 0);                
                // 随机产生雷声
                if (rand() % 100 < 50) {
                    thunder_active = true;
                    thunder_start_time = current_time + 500 + rand() % 1000; // 闪电后延迟出现雷声
                    thunder_duration = 1000 + rand() % 2000;
                }                
                last_lightning_time = current_time;
            }
        }
        // 更新所有元素
        update_raindrops(current_time, delta_time);
        update_ripples(current_time);
        update_splashes(current_time, delta_time);
        update_lightning(current_time);
        update_stars(current_time);
        update_lotus_pads(current_time, delta_time);
        update_lotus_flowers(current_time, delta_time);
        update_camera();
        perf.physics_time = (SDL_GetPerformanceCounter() - physics_start) * 1000.0 / perf.freq;

        /* ==== [3] rendering ==== */
        Uint64 rander_start = SDL_GetPerformanceCounter();
        // 清屏
        SDL_SetRenderDrawColor(renderer, 0, 0, 20, 255); // 深蓝色夜空
        SDL_RenderClear(renderer);        
        // 渲染所有元素
        render();        
        // 更新屏幕
        SDL_RenderPresent(renderer); 
        perf.render_time = (SDL_GetPerformanceCounter() - rander_start) * 1000.0 / perf.freq;

        /* ==== [4] compute performance data ==== */
        perf.frame_end = SDL_GetPerformanceCounter();
        perf.frame_time = (perf.frame_end - perf.frame_start) * 1000.0 / perf.freq; // 转换为毫秒
        perf.avg_frame_time = perf.avg_frame_time * 0.9 + perf.frame_time * 0.1; // 滑动平均
        perf.frame_count++;
        if (perf.frame_count % 60 == 0) { //output performance data every 60 frames
            printf("[Frame %d] Total: %.1fms (Phys:%.1fms Render:%.1fms Input:%.1fms) FPS: %.1f\n",
               perf.frame_count,
               perf.avg_frame_time,
               perf.physics_time,
               perf.render_time,
               perf.input_time,
               1000.0 / perf.avg_frame_time);
        }

        // 限制帧率为60 FPS
        SDL_Delay(1000 / 60);
    }
    
    // 释放资源并关闭SDL
    close();
    
    return 0;
}

bool initialize() {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL无法初始化! SDL错误: %s\n", SDL_GetError());
        return false;
    }
    
    // 创建窗口
    window = SDL_CreateWindow("彩色雨夜荷塘 (高级3D特效版)", 
                             SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                             WINDOW_WIDTH, WINDOW_HEIGHT, 
                             SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("无法创建窗口! SDL错误: %s\n", SDL_GetError());
        return false;
    }
    
    // 创建渲染器 - 尝试使用硬件加速
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("警告：无法创建硬件加速渲染器，尝试创建软件渲染器...\n");
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        
        if (renderer == NULL) {
            printf("无法创建任何渲染器! SDL错误: %s\n", SDL_GetError());
            return false;
        }
        printf("成功创建软件渲染器。\n");
    } else {
        printf("成功创建硬件加速渲染器。\n");
    }
    
    // 设置渲染器混合模式
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // 初始化各种元素数组
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        raindrops[i].active = false;
    }
    
    for (int i = 0; i < MAX_RIPPLES; i++) {
        ripples[i].active = false;
    }
    
    for (int i = 0; i < MAX_SPLASHES; i++) {
        splashes[i].active = false;
    }
    
    for (int i = 0; i < MAX_LIGHTNING; i++) {
        lightnings[i].active = false;
    }

    /* initialize performance monitor */
    perf.freq = SDL_GetPerformanceFrequency();
    perf.avg_frame_time = 0.0;
    perf.frame_count = 0;
    
    printf("初始化完成，开始渲染彩色雨夜荷塘...\n");
    return true;
}

void close() {
    /* destroy textures */
    if (moon_texture != NULL) {
        SDL_DestroyTexture(moon_texture);
        moon_texture = NULL;
    }
    for (int i = 0; i < MAX_CLOUD_LAYERS; i++) {
        if (cloud_textures[i]) {
            SDL_DestroyTexture(cloud_textures[i]);
            cloud_textures[i] = NULL;
        }
    }
    destroy_lotus_textures();
    
    // 销毁渲染器和窗口
    if (renderer != NULL) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    
    if (window != NULL) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    
    // 退出SDL子系统
    SDL_Quit();
}

void draw_crater(SDL_Surface* surface, int cx, int cy, int radius, Uint32 color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                int px = cx + x;
                int py = cy + y;
                if (px >= 0 && px < surface->w && py >= 0 && py < surface->h) {
                    ((Uint32*)surface->pixels)[py * surface->pitch/4 + px] = color;
                }
            }
        }
    }
}

void create_raindrop(bool on_surface) {
    // 查找一个未激活的雨滴槽位
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        if (!raindrops[i].active) {
            raindrops[i].active = true;
            raindrops[i].z = (float)rand() / RAND_MAX; // 随机深度 (0-1)
            
            // 根据深度，远处雨滴位置范围更大，模拟宽视场
            float z_width_scale = 1.0f + (1.0f - raindrops[i].z) * 2.0f;
            raindrops[i].x = (rand() % (int)(WINDOW_WIDTH * z_width_scale)) - 
                              ((z_width_scale - 1.0f) * WINDOW_WIDTH / 2);
            
            if (on_surface) {
                // 直接在水面随机位置生成雨滴
                raindrops[i].y = POND_HEIGHT + rand() % (WINDOW_HEIGHT - POND_HEIGHT);
                raindrops[i].in_water = true;
                raindrops[i].water_time = SDL_GetTicks();
                
                // 创建涟漪
                create_ripple(raindrops[i].x, raindrops[i].y, raindrops[i].z, raindrops[i].color);
            } else {
                // 在天空生成雨滴
                raindrops[i].in_water = false;
                raindrops[i].y = -10 - rand() % 50;  // 从窗口上方不同高度开始
            }
            
            // 远处的雨滴看起来应该下落得更慢
            float z_speed_scale = 0.2f + raindrops[i].z * 0.8f;
            
            // 根据天气强度调整下落速度
            float intensity_factor = 1.0f + (weather_intensity / 100.0f);
            
            raindrops[i].speed_y = (RAINDROP_FALL_SPEED_MIN + 
                                  (float)rand() / RAND_MAX * (RAINDROP_FALL_SPEED_MAX - RAINDROP_FALL_SPEED_MIN)) * 
                                  z_speed_scale * intensity_factor;
            
            // 初始水平速度受风影响
            raindrops[i].speed_x = wind_strength * 50.0f * z_speed_scale * intensity_factor;
            
            raindrops[i].color = get_random_color();
            raindrops[i].size = 2 + rand() % 5;  // 基础大小在2到6之间
            raindrops[i].creation_time = SDL_GetTicks();
            raindrop_count++;
            return;
        }
    }
}

void create_ripple(float x, float y, float z, SDL_Color color) {
    // 查找一个未激活的涟漪槽位
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) {
            ripples[i].active = true;
            ripples[i].x = x;
            ripples[i].y = y;
            ripples[i].z = z;
            ripples[i].radius = 0;
            // 远处的涟漪最大半径应该更小
            float z_radius_scale = get_z_scale(z);
            ripples[i].max_radius = (20 + rand() % 40) * z_radius_scale;
            ripples[i].color = color;
            ripples[i].creation_time = SDL_GetTicks();
            ripple_count++;
            return;
        }
    }
}

void create_splash(float x, float y, float z, SDL_Color color) {
    // 创建多个溅射水珠
    int splash_count = 5 + rand() % 8; // 5-12个水珠
    
    // 根据强度增加水珠数量
    splash_count = (int)(splash_count * (1.0f + weather_intensity / 100.0f));
    
    for (int i = 0; i < splash_count; i++) {
        // 查找未使用的溅射水珠槽位
        for (int j = 0; j < MAX_SPLASHES; j++) {
            if (!splashes[j].active) {
                splashes[j].active = true;
                splashes[j].x = x;
                splashes[j].y = y;
                splashes[j].z = z;
                
                // 随机速度方向，创造圆形溅射效果
                float angle = ((float)rand() / RAND_MAX) * 6.28f; // 0-2π
                
                // 根据天气强度调整速度
                float intensity_factor = 1.0f + (weather_intensity / 100.0f);
                float speed = (50.0f + ((float)rand() / RAND_MAX) * 150.0f) * intensity_factor; // 50-200，受强度影响
                
                // 风会影响水珠方向
                angle += wind_strength * 0.5f;
                
                splashes[j].speed_x = cosf(angle) * speed;
                splashes[j].speed_y = sinf(angle) * speed - 200.0f; // 初始向上的趋势
                
                splashes[j].size = 1.0f + ((float)rand() / RAND_MAX) * 2.0f; // 1-3
                splashes[j].color = color;
                splashes[j].creation_time = SDL_GetTicks();
                splash_count++;
                break;
            }
        }
    }
}

void create_lightning(int x, int y, int segments, int width, int type) {
    // 查找未使用的闪电槽位
    for (int i = 0; i < MAX_LIGHTNING; i++) {
        if (!lightnings[i].active) {
            lightnings[i].active = true;
            lightnings[i].segments = segments;
            lightnings[i].width = width;
            lightnings[i].type = type;
            
            // 亮度受天气强度影响
            lightnings[i].brightness = 180 + rand() % 75 + weather_intensity / 2; // 180-255 + 强度影响
            if (lightnings[i].brightness > 255) lightnings[i].brightness = 255;
            
            lightnings[i].creation_time = SDL_GetTicks();
            
            // 根据强度调整闪电持续时间
            float duration_factor = 1.0f + (weather_intensity / 100.0f);
            lightnings[i].duration = (int)((100 + rand() % 200) * duration_factor); // 100-300ms，受强度影响
            
            // 设置闪电路径
            int current_x = x;
            int current_y = y;
            lightnings[i].points[0][0] = current_x;
            lightnings[i].points[0][1] = current_y;
            
            // 根据强度调整闪电的锯齿度和分支概率
            float zigzag_factor = 40.0f * (1.0f + weather_intensity / 200.0f);
            float branch_prob = 30.0f * (1.0f + weather_intensity / 150.0f);
            
            for (int j = 1; j <= segments; j++) {
                // 闪电路径随机偏移 - 受强度影响
                current_x += (rand() % (int)zigzag_factor) - (int)(zigzag_factor / 2);
                current_y += WINDOW_HEIGHT / segments;
                
                if (current_y > POND_HEIGHT) current_y = POND_HEIGHT; // 不超过水面
                
                lightnings[i].points[j][0] = current_x;
                lightnings[i].points[j][1] = current_y;
                
                // 随机生成分支闪电 - 受强度影响
                if (type == 0 && segments > 3 && j > 1 && j < segments - 1 && rand() % 100 < branch_prob) {
                    int branch_segments = segments / 2;
                    if (weather_intensity > 70) branch_segments += 1; // 高强度时分支更长
                    create_lightning(current_x, current_y, branch_segments, width - 1, 1);
                }
            }
            
            lightning_count++;
            return;
        }
    }
}

void initialize_moon() {
    /* initialize moon texture */
    SDL_Surface* moon_surface = SDL_CreateRGBSurface(0, 80, 80, 32, 
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!moon_surface) {
        printf("无法创建月亮表面! SDL错误: %s\n", SDL_GetError());
        return;
    }
    // 绘制月亮到表面
    SDL_FillRect(moon_surface, NULL, SDL_MapRGBA(moon_surface->format, 0,0,0,0)); // 透明背景
    SDL_LockSurface(moon_surface);
    // 绘制月亮主体
    const int moon_radius = 40;
    const int center_x = 40;
    const int center_y = 40;
    Uint32 moon_color = SDL_MapRGBA(moon_surface->format, 230,230,230,255);
    for (int y = -moon_radius; y <= moon_radius; y++) {
        for (int x = -moon_radius; x <= moon_radius; x++) {
            if (x*x + y*y <= moon_radius*moon_radius) {
                int px = center_x + x;
                int py = center_y + y;
                if (px >= 0 && px < 80 && py >= 0 && py < 80) {
                    ((Uint32*)moon_surface->pixels)[py * moon_surface->pitch/4 + px] = moon_color;
                }
            }
        }
    }
    // 绘制陨石坑（预渲染到纹理）
    Uint32 crater_color = SDL_MapRGBA(moon_surface->format, 200,200,200,255);
    // 主陨石坑
    draw_crater(moon_surface, 25, 30, 10, crater_color);
    // 其他小陨石坑
    draw_crater(moon_surface, 50, 35, 5, crater_color);
    draw_crater(moon_surface, 35, 50, 7, crater_color);
    SDL_UnlockSurface(moon_surface);
    // 创建纹理
    moon_texture = SDL_CreateTextureFromSurface(renderer, moon_surface);
    SDL_FreeSurface(moon_surface);
    if (!moon_texture) {
        printf("无法创建月亮纹理! SDL错误: %s\n", SDL_GetError());
        return;
    }
}

void initialize_cloud() {
    for (int layer = 0; layer < MAX_CLOUD_LAYERS; layer++) {
        // 创建表面用于生成云纹理
        SDL_Surface* cloud_surface = SDL_CreateRGBSurface(0, WINDOW_WIDTH*2, 200, 32, 
            0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        SDL_FillRect(cloud_surface, NULL, SDL_MapRGBA(cloud_surface->format, 0,0,0,0));
        
        // 预生成云层数据（使用改进的噪声算法）
        SDL_LockSurface(cloud_surface);
        for (int x = 0; x < cloud_surface->w; x++) {
            // 使用分形噪声生成更自然的云图案
            float noise = 
                0.50 * sin(x * 0.01 + layer*2) +
                0.25 * cos(x * 0.03 + layer*5) +
                0.15 * sin(x * 0.07 + layer*3) +
                0.10 * cos(x * 0.13 + layer*7);
            
            int cloud_height = (int)(30 + noise * 40 + layer * 10);
            Uint8 alpha = (Uint8)(100 + layer * 15);
            
            for (int y = 0; y < cloud_surface->h; y++) {
                if(y < cloud_height) {
                    ((Uint32*)cloud_surface->pixels)[y * cloud_surface->pitch/4 + x] = 
                        SDL_MapRGBA(cloud_surface->format, 80-layer*10, 80-layer*10, 100-layer*10, 255);
                }
            }
        }
        SDL_UnlockSurface(cloud_surface);
        
        // 转换为纹理并保存
        cloud_textures[layer] = SDL_CreateTextureFromSurface(renderer, cloud_surface);
        SDL_FreeSurface(cloud_surface);
        cloud_offsets[layer] = 0;
    }    
}

void initialize_stars() {
    for (int i = 0; i < STARS_COUNT; i++) {
        stars[i].z = (float)rand() / RAND_MAX; // 随机深度 (0-1)
        
        // 根据深度，远处星星的分布范围更大
        float z_width_scale = 1.0f + (1.0f - stars[i].z) * 3.0f;
        stars[i].x = (rand() % (int)(WINDOW_WIDTH * z_width_scale)) - 
                     ((z_width_scale - 1.0f) * WINDOW_WIDTH / 2);
        stars[i].y = rand() % POND_HEIGHT;
        stars[i].brightness = 0.5f + ((float)rand() / RAND_MAX) * 0.5f;  // 亮度在0.5到1.0之间
        stars[i].twinkle_speed = 0.5f + ((float)rand() / RAND_MAX) * 2.0f;  // 闪烁速度在0.5到2.5之间
    }
}

void initialize_mountains() {
    for (int i = 0; i < MOUNTAIN_COUNT; i++) {
        mountains[i].z = 0.1f + (float)i / (MOUNTAIN_COUNT - 1) * 0.5f; // 深度从0.1到0.6按顺序递增
        mountains[i].x_offset = -WINDOW_WIDTH/2 + rand() % WINDOW_WIDTH; // 随机X偏移
        mountains[i].height = (int)(100 + (rand() % 100) * mountains[i].z); // 远处的山低，近处的山高
        mountains[i].width = (int)(200 + rand() % 300); // 随机宽度
        
        // 颜色从远到近由深变浅
        Uint8 color_value = (Uint8)(40 + mountains[i].z * 60);
        mountains[i].color.r = color_value - 20;
        mountains[i].color.g = color_value;
        mountains[i].color.b = color_value + 10;
        mountains[i].color.a = 255;
    }
}

void initialize_reeds() {
    for (int i = 0; i < REED_COUNT; i++) {
        reeds[i].z = 0.5f + ((float)rand() / RAND_MAX) * 0.5f; // 随机深度 (0.5-1.0)较近的位置
        
        // 分布在水域边缘
        float edge_variance = 50.0f; // 岸边区域大小
        reeds[i].x = rand() % WINDOW_WIDTH;
        reeds[i].y = POND_HEIGHT - 5 + (rand() % 10); // 岸边位置上下浮动
        
        // 大小和摇摆参数
        reeds[i].height = (int)(30 + rand() % 30 * reeds[i].z); // 高度随深度增加
        reeds[i].sway_offset = ((float)rand() / RAND_MAX) * 6.28f; // 随机相位 (0-2π)
        reeds[i].sway_speed = 0.5f + ((float)rand() / RAND_MAX) * 1.5f; // 随机摇摆速度
    }
}

void generate_lotus_texture(LotusPad* pad) {
    int radius = (int)pad->radius;
    int tex_size = radius * 2 + 2;
    
    // 创建表面
    SDL_Surface* surface = SDL_CreateRGBSurface(0, tex_size, tex_size, 32, 
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0,0,0,0));
    
    SDL_LockSurface(surface);
    
    int center_x = tex_size/2;
    int center_y = tex_size/2;
    
    // 绘制边缘
    SDL_Color edge_color = pad->color;
    edge_color.r = (Uint8)(edge_color.r * 0.7f);
    edge_color.g = (Uint8)(edge_color.g * 0.7f);
    edge_color.b = (Uint8)(edge_color.b * 0.7f);
    
    for (int angle = 0; angle < 360; angle += 2) {
        float rad = angle * 3.14f / 180.0f;
        float x = radius * cosf(rad) * (1.0f - 0.2f * sinf(rad));
        float y = radius * sinf(rad) * (1.0f + 0.1f * cosf(rad)) * 0.7;
        
        int px = center_x + (int)x;
        int py = center_y + (int)y;
        if (px >= 0 && px < tex_size && py >= 0 && py < tex_size) {
            ((Uint32*)surface->pixels)[py * surface->pitch/4 + px] = 
                SDL_MapRGBA(surface->format, edge_color.r, edge_color.g, edge_color.b, 255);
        }
    }
    
    // 填充内部
    SDL_Color fill_color = pad->color;
    for (float r = 0; r < radius * 0.95f; r += 0.5f) {
        for (int angle = 0; angle < 360; angle += 2) {
            float rad = angle * 3.14f / 180.0f;
            float x = r * cosf(rad) * (1.0f - 0.2f * sinf(rad));
            float y = r * sinf(rad) * (1.0f + 0.1f * cosf(rad)) * 0.7;
            
            int px = center_x + (int)x;
            int py = center_y + (int)y;
            if (px >= 0 && px < tex_size && py >= 0 && py < tex_size) {
                ((Uint32*)surface->pixels)[py * surface->pitch/4 + px] = 
                    SDL_MapRGBA(surface->format, fill_color.r, fill_color.g, fill_color.b, 255);
            }
        }
    }
    
    // 绘制叶脉
    SDL_Color vein_color = fill_color;
    vein_color.r = (Uint8)(vein_color.r * 0.8f);
    vein_color.g = (Uint8)(vein_color.g * 0.8f);
    vein_color.b = (Uint8)(vein_color.b * 0.8f);
    
    for (int j = 0; j < 8; j++) {
        float angle = j * 3.14f / 4.0f;
        for (float r = 0; r < radius * 0.9f; r += 0.5f) {
            int px = center_x + (int)(r * cosf(angle));
            int py = center_y + (int)(r * sinf(angle)) * 0.7;
            if (px >= 0 && px < tex_size && py >= 0 && py < tex_size) {
                ((Uint32*)surface->pixels)[py * surface->pitch/4 + px] = 
                    SDL_MapRGBA(surface->format, vein_color.r, vein_color.g, vein_color.b, 255);
            }
        }
    }
    
    SDL_UnlockSurface(surface);
    pad->texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
}

void initialize_lotus_pads() {
    for (int i = 0; i < LOTUS_PAD_COUNT; i++) {
        lotus_pads[i].z = 0.3f + ((float)rand() / RAND_MAX) * 0.7f; // 随机深度 (0.3-1.0)
        
        // 在水面随机分布
        float z_width_scale = 1.0f + (1.0f - lotus_pads[i].z) * 1.5f;
        lotus_pads[i].x = (rand() % (int)(WINDOW_WIDTH * z_width_scale)) - 
                         ((z_width_scale - 1.0f) * WINDOW_WIDTH / 2);
        lotus_pads[i].y = POND_HEIGHT + 10 + rand() % (WINDOW_HEIGHT - POND_HEIGHT - 20);
        
        // 大小和波动参数
        float z_scale = get_z_scale(lotus_pads[i].z);
        lotus_pads[i].radius = (15.0f + rand() % 20) * z_scale; // 荷叶半径随深度变化
        lotus_pads[i].wave_phase = ((float)rand() / RAND_MAX) * 6.28f; // 随机波动初相
        lotus_pads[i].wave_speed = 0.5f + ((float)rand() / RAND_MAX); // 随机波动速度
        lotus_pads[i].tilt_angle = ((float)rand() / RAND_MAX) * 0.3f; // 随机倾斜角度
        
        // 荷叶颜色 - 深绿色
        lotus_pads[i].color.r = 30 + rand() % 20;
        lotus_pads[i].color.g = 100 + rand() % 50;
        lotus_pads[i].color.b = 30 + rand() % 20;
        lotus_pads[i].color.a = 255;

        generate_lotus_texture(&lotus_pads[i]);
    }
}

void render_lotus_texture(LotusPad* pad, int proj_x, float tilt) {
    if (!pad->texture) return;
    
    int tex_w, tex_h;
    SDL_QueryTexture(pad->texture, NULL, NULL, &tex_w, &tex_h);
    
    float z_scale = get_z_scale(pad->z);
    SDL_Rect dest_rect = {
        proj_x - (int)(tex_w * z_scale / 2),
        (int)pad->y - (int)(tex_h * z_scale / 2),
        (int)(tex_w * z_scale),
        (int)(tex_h * z_scale)
    };
    
    SDL_Point center = {dest_rect.w/2, dest_rect.h/2};
    SDL_RenderCopyEx(renderer, pad->texture, NULL, &dest_rect, 
                   tilt * (180.0f / 3.14f), &center, SDL_FLIP_NONE);
}

void destroy_lotus_textures() {
    for (int i = 0; i < LOTUS_PAD_COUNT; i++) {
        if (lotus_pads[i].texture) {
            SDL_DestroyTexture(lotus_pads[i].texture);
            lotus_pads[i].texture = NULL;
        }
    }
}
    
void initialize_lotus_flowers() {
    for (int i = 0; i < LOTUS_FLOWER_COUNT; i++) {
        lotus_flowers[i].z = 0.4f + ((float)rand() / RAND_MAX) * 0.6f; // 随机深度 (0.4-1.0)
        
        // 在水面随机分布
        float z_width_scale = 1.0f + (1.0f - lotus_flowers[i].z) * 1.5f;
        lotus_flowers[i].x = (rand() % (int)(WINDOW_WIDTH * z_width_scale)) - 
                           ((z_width_scale - 1.0f) * WINDOW_WIDTH / 2);
        lotus_flowers[i].y = POND_HEIGHT + 10 + rand() % (WINDOW_HEIGHT - POND_HEIGHT - 20);
        
        // 大小和摇摆参数
        float z_scale = get_z_scale(lotus_flowers[i].z);
        lotus_flowers[i].size = (10.0f + rand() % 10) * z_scale; // 大小随深度变化
        lotus_flowers[i].sway_phase = ((float)rand() / RAND_MAX) * 6.28f; // 随机摇摆初相
        
        // 荷花颜色 - 粉白色
        lotus_flowers[i].color.r = 230 + rand() % 25;
        lotus_flowers[i].color.g = 200 + rand() % 25;
        lotus_flowers[i].color.b = 220 + rand() % 25;
        lotus_flowers[i].color.a = 255;
        
        // 花瓣数量
        lotus_flowers[i].petal_count = 5 + rand() % 4; // 5-8花瓣
    }
}

SDL_Color get_random_color() {
    SDL_Color color;
    // 生成适合雨滴的柔和颜色
    color.r = 150 + rand() % 105;  // 150-255

    color.g = 150 + rand() % 105;  // 150-255
    color.b = 150 + rand() % 105;  // 150-255
    color.a = 150 + rand() % 105;  // 150-255 (半透明)
    return color;
}

// 根据z坐标获取缩放比例
float get_z_scale(float z) {
    // z从0(远)到1(近)
    // 远处的物体应该更小
    return 0.2f + z * 0.8f; // 缩放从0.2到1.0
}

// 根据z坐标投影x坐标
float project_x(float x, float z) {
    // 应用摄像机偏移，并根据深度强度调整
    float perspective_strength = 0.3f; // 透视强度
    // 应用摄像机位置并根据深度进行透视校正
    return (x - camera_x * perspective_strength * z);
}

// 根据深度调整颜色
SDL_Color adjust_color_by_depth(SDL_Color color, float z) {
    // 远处的物体颜色偏蓝色并变暗，模拟大气透视
    SDL_Color adjusted;
    float fog_factor = 1.0f - z * 0.7f; // 远处的雾效果强度
    
    adjusted.r = (Uint8)(color.r * z + 0 * fog_factor);
    adjusted.g = (Uint8)(color.g * z + 5 * fog_factor);
    adjusted.b = (Uint8)(color.b * z + 30 * fog_factor);
    adjusted.a = color.a;
    
    return adjusted;
}

// 根据天气状态和强度获取雨滴生成间隔
float get_rain_interval(WeatherState weather, int intensity) {
    float base_interval;
    
    switch (weather) {
        case WEATHER_LIGHT_RAIN:
            base_interval = 150.0f; // 和风细雨 - 雨滴较少
            break;
        case WEATHER_MEDIUM_RAIN:
            base_interval = 80.0f;  // 中雨 - 雨滴适中
            break;
        case WEATHER_HEAVY_RAIN:
        case WEATHER_THUNDERSTORM:
            base_interval = 20.0f;  // 暴风骤雨/雷暴 - 雨滴密集
            break;
        default:
            base_interval = 100.0f;
    }
    
    // 根据强度调整间隔，强度越高间隔越短（雨越密集）
    return base_interval * (1.0f - intensity / 200.0f); // 强度影响降至50%
}

void update_stars(Uint32 current_time) {
    float time_seconds = current_time / 1000.0f;
    
    for (int i = 0; i < STARS_COUNT; i++) {
        // 使用正弦函数来创建闪烁效果
        float phase = time_seconds * stars[i].twinkle_speed;
        float sine_value = (sinf(phase) + 1.0f) / 2.0f;  // 将正弦值归一化到0-1范围
        stars[i].brightness = 0.5f + sine_value * 0.5f;  // 亮度在0.5到1.0之间变化
    }
}

void update_camera() {
    if (camera_moving) {
        // 平滑移动摄像机
        float move_speed = 0.1f;
        float diff = camera_target_x - camera_x;
        
        if (fabsf(diff) < 0.5f) {
            camera_x = camera_target_x;
            camera_moving = false;
        } else {
            camera_x += diff * move_speed;
        }
    }
}

void update_weather_and_wind(Uint32 current_time) {
    // 检查是否应该切换天气状态
    Uint32 weather_duration = weather_duration_min + 
                             rand() % (weather_duration_max - weather_duration_min);
    
    if (current_time - last_weather_change_time > weather_duration && 
        current_weather == target_weather) {
        // 随机选择新的目标天气，不同于当前天气
        do {
            target_weather = (WeatherState)(rand() % WEATHER_COUNT);
        } while (target_weather == current_weather);
        
        last_weather_change_time = current_time;
    }
    
    // 平滑过渡到目标天气
    if (current_weather != target_weather) {
        // 简化处理，直接设置为目标天气
        current_weather = target_weather;
        
        // 根据天气设置目标风强度
        switch (current_weather) {
            case WEATHER_LIGHT_RAIN:
                target_wind_strength = -0.2f + ((float)rand() / RAND_MAX) * 0.4f; // -0.2 到 0.2
                break;
            case WEATHER_MEDIUM_RAIN:
                target_wind_strength = -0.5f + ((float)rand() / RAND_MAX) * 1.0f; // -0.5 到 0.5
                break;
            case WEATHER_HEAVY_RAIN:
                target_wind_strength = -0.8f + ((float)rand() / RAND_MAX) * 1.6f; // -0.8 到 0.8
                break;
            case WEATHER_THUNDERSTORM:
                target_wind_strength = -1.0f + ((float)rand() / RAND_MAX) * 2.0f; // -1.0 到 1.0
                break;
        }
    }
    
    // 定期微调风强度，制造风力变化
    if (rand() % 100 == 0) {
        // 微调目标风力，但保持在当前天气的合理范围内
        float wind_variance = 0.0f;
        switch (current_weather) {
            case WEATHER_LIGHT_RAIN:
                wind_variance = 0.1f;
                break;
            case WEATHER_MEDIUM_RAIN:
                wind_variance = 0.2f;
                break;
            case WEATHER_HEAVY_RAIN:
                wind_variance = 0.4f;
                break;
            case WEATHER_THUNDERSTORM:
                wind_variance = 0.5f;
                break;
        }
        
        // 增加强度对风的影响
        wind_variance *= (0.5f + weather_intensity / 100.0f);
        
        target_wind_strength += (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * wind_variance;
        
        // 限制风强度范围
        if (target_wind_strength > 1.0f) target_wind_strength = 1.0f;
        if (target_wind_strength < -1.0f) target_wind_strength = -1.0f;
    }
    
    // 平滑过渡到目标风强度
    float wind_diff = target_wind_strength - wind_strength;
    if (fabsf(wind_diff) > 0.01f) {
        wind_strength += wind_diff * wind_change_speed;
    } else {
        wind_strength = target_wind_strength;
    }
}

void update_thunder(Uint32 current_time) {
    // 更新雷声状态
    if (thunder_active && current_time >= thunder_start_time) {
        if (current_time - thunder_start_time > thunder_duration) {
            thunder_active = false;
        }
    }
}

// 检查雨滴与荷叶的碰撞
bool check_raindrop_lotus_collision(Raindrop* raindrop) {
    // 检查雨滴与荷叶的碰撞
    float proj_x = project_x(raindrop->x, raindrop->z);
    
    for (int i = 0; i < LOTUS_PAD_COUNT; i++) {
        // 简单的圆形碰撞检测
        float pad_proj_x = project_x(lotus_pads[i].x, lotus_pads[i].z);
        
        // 雨滴深度应该接近荷叶深度才有效果
        if (fabsf(raindrop->z - lotus_pads[i].z) < 0.2f) {
            float dx = proj_x - pad_proj_x;
            float dy = raindrop->y - lotus_pads[i].y;
            float distance = sqrtf(dx*dx + dy*dy);
            
            // 考虑荷叶倾斜时的椭圆形状
            float tilt_factor = 1.0f + fabsf(lotus_pads[i].tilt_angle) * 0.5f;
            float scaled_radius = lotus_pads[i].radius / tilt_factor;
            
            if (distance < scaled_radius) {
                return true;
            }
        }
    }
    
    return false;
}

void update_raindrops(Uint32 current_time, float delta_time) {
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        if (raindrops[i].active) {
            if (!raindrops[i].in_water) {
                // 风力影响 - 只影响下落中的雨滴
                float wind_effect = wind_strength * 100.0f * delta_time;
                
                // 在暴风雨中，单个雨滴受到的风力有一定随机性，创造更动态的效果
                if (current_weather >= WEATHER_HEAVY_RAIN) {
                    wind_effect += (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * 20.0f * delta_time * 
                                  (0.5f + weather_intensity / 100.0f);
                }
                
                // 更新雨滴水平位置（受风影响）
                raindrops[i].x += raindrops[i].speed_x * delta_time + wind_effect;
                
                // 更新雨滴垂直位置
                raindrops[i].y += raindrops[i].speed_y * delta_time;
                
                // 检查雨滴是否击中荷叶
                if (raindrop_count % 5 == 0 && check_raindrop_lotus_collision(&raindrops[i])) {
                    raindrops[i].in_water = true;
                    raindrops[i].water_time = current_time;
                    
                    // 在荷叶上创建溅射效果
                    create_splash(raindrops[i].x, raindrops[i].y, raindrops[i].z, raindrops[i].color);
                }
                // 检查雨滴是否击中水面
                else if (raindrops[i].y >= POND_HEIGHT) {
                    raindrops[i].in_water = true;
                    raindrops[i].water_time = current_time;
                    
                    // 创建涟漪
                    create_ripple(raindrops[i].x, POND_HEIGHT, raindrops[i].z, raindrops[i].color);
                }
            } else {
                // 雨滴已入水
                // 如果入水超过500毫秒，停用它
                if (current_time - raindrops[i].water_time > 500) {
                    raindrops[i].active = false;
                    raindrop_count--;
                }
            }
        }
    }
}

void update_ripples(Uint32 current_time) {
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (ripples[i].active) {
            // 计算涟漪已经存在的时间
            Uint32 ripple_age = current_time - ripples[i].creation_time;
            
            // 根据年龄更新涟漪半径
            float progress = (float)ripple_age / RIPPLE_LIFETIME;
            if (progress > 1.0f) progress = 1.0f;
            
            ripples[i].radius = ripples[i].max_radius * progress;
            
            // 随时间淡出涟漪
            ripples[i].color.a = (Uint8)(255 * (1.0f - progress));
            
            // 如果涟漪达到其生命周期，停用它
            if (ripple_age >= RIPPLE_LIFETIME) {
                ripples[i].active = false;
                ripple_count--;
            }
        }
    }
}

void update_splashes(Uint32 current_time, float delta_time) {
    for (int i = 0; i < MAX_SPLASHES; i++) {
        if (splashes[i].active) {
            // 计算已存在时间
            Uint32 splash_age = current_time - splashes[i].creation_time;
            
            // 重力效果
            splashes[i].speed_y += 500.0f * delta_time; // 重力加速度
            
            // 更新位置
            splashes[i].x += splashes[i].speed_x * delta_time;
            splashes[i].y += splashes[i].speed_y * delta_time;
            
            // 随时间淡出
            float progress = (float)splash_age / SPLASH_LIFETIME;
            if (progress > 1.0f) {
                // 停用水珠
                splashes[i].active = false;
                splash_count--;
            }
            
            // 如果水珠落入水面，创建小涟漪并停用
            if (splashes[i].y >= POND_HEIGHT && splashes[i].speed_y > 0) {
                // 创建小涟漪
                SDL_Color ripple_color = splashes[i].color;
                ripple_color.a = (Uint8)(ripple_color.a * (1.0f - progress)); // 根据寿命调整透明度
                create_ripple(splashes[i].x, POND_HEIGHT, splashes[i].z, ripple_color);
                
                // 停用水珠
                splashes[i].active = false;
                splash_count--;
            }
        }
    }
}

void update_lightning(Uint32 current_time) {
    for (int i = 0; i < MAX_LIGHTNING; i++) {
        if (lightnings[i].active) {
            // 计算闪电已存在的时间
            Uint32 lightning_age = current_time - lightnings[i].creation_time;
            
            // 如果闪电达到其生命周期，停用它
            if (lightning_age >= lightnings[i].duration) {
                lightnings[i].active = false;
                lightning_count--;
            }
        }
    }
}

void update_lotus_pads(Uint32 current_time, float delta_time) {
    float time_seconds = current_time / 1000.0f;
    
    for (int i = 0; i < LOTUS_PAD_COUNT; i++) {
        // 荷叶随风轻微波动
        lotus_pads[i].wave_phase += delta_time * lotus_pads[i].wave_speed;
        
        // 风对荷叶的倾斜影响
        lotus_pads[i].tilt_angle = wind_strength * 0.2f + 
                                  sinf(time_seconds * lotus_pads[i].wave_speed + lotus_pads[i].wave_phase) * 0.1f;
    }
}

void update_lotus_flowers(Uint32 current_time, float delta_time) {
    float time_seconds = current_time / 1000.0f;
    
    for (int i = 0; i < LOTUS_FLOWER_COUNT; i++) {
        // 荷花随风轻微摇摆
        lotus_flowers[i].sway_phase += delta_time * 0.5f;
    }
}

void render() {
    // 绘制夜空背景（已在主循环中完成）
    
    // 是否有闪电照亮整个场景
    bool lightning_flash = false;
    Uint8 flash_brightness = 0;
    
    // 检查是否有活跃的闪电
    for (int i = 0; i < MAX_LIGHTNING; i++) {
        if (lightnings[i].active && lightnings[i].type == 0) {
            lightning_flash = true;
            flash_brightness = (Uint8)(lightnings[i].brightness * 0.4f);
            break;
        }
    }
    
    // 如果有闪电，覆盖整个屏幕的半透明白色矩形
    if (lightning_flash) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, flash_brightness, flash_brightness, flash_brightness, 100);
        SDL_Rect flash_rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
        SDL_RenderFillRect(renderer, &flash_rect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
    
    // 绘制星星
    for (int i = 0; i < STARS_COUNT; i++) {
        // 计算投影位置
        int proj_x = (int)project_x(stars[i].x, stars[i].z);
        
        // 只绘制在屏幕内的星星
        if (proj_x >= 0 && proj_x < WINDOW_WIDTH) {
            // 暴风雨时星星会被雨云遮挡，变暗
            float weather_visibility = 1.0f;
            if (current_weather == WEATHER_THUNDERSTORM) {
                weather_visibility = 0.2f; // 雷暴时星星只有20%亮度
            } else if (current_weather == WEATHER_HEAVY_RAIN) {
                weather_visibility = 0.4f; // 暴风雨时星星只有40%亮度
            } else if (current_weather == WEATHER_MEDIUM_RAIN) {
                weather_visibility = 0.7f; // 中雨时星星只有70%亮度
            }
            
            // 天气强度进一步影响可见度
            weather_visibility *= (1.0f - weather_intensity / 200.0f);
            
            // 计算星星的实际亮度（0-255）
            float z_brightness_scale = get_z_scale(stars[i].z);
            Uint8 brightness = (Uint8)(stars[i].brightness * 255 * z_brightness_scale * weather_visibility);
            
            // 闪电会增加星星亮度
            if (lightning_flash) {
                brightness = (Uint8)fminf(255, brightness + flash_brightness);
            }
            
            SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, 255);
            
            // 绘制星星（小点）
            SDL_RenderDrawPoint(renderer, proj_x, stars[i].y);
            
            // 对于特别亮的且较近的星星，绘制更大的点
            if (stars[i].brightness > 0.8f && stars[i].z > 0.7f) {
                SDL_RenderDrawPoint(renderer, proj_x + 1, stars[i].y);
                SDL_RenderDrawPoint(renderer, proj_x - 1, stars[i].y);
                SDL_RenderDrawPoint(renderer, proj_x, stars[i].y + 1);
                SDL_RenderDrawPoint(renderer, proj_x, stars[i].y - 1);
            }
        }
    }
    
    // 绘制月亮 - 根据天气状态调整可见度
    float moon_visibility = 1.0f;
    switch (current_weather) {
        case WEATHER_LIGHT_RAIN:
            moon_visibility = 0.9f;
            break;
        case WEATHER_MEDIUM_RAIN:
            moon_visibility = 0.7f;
            break;
        case WEATHER_HEAVY_RAIN:
            moon_visibility = 0.4f;
            break;
        case WEATHER_THUNDERSTORM:
            moon_visibility = 0.2f;
            break;
    }
    
    // 天气强度进一步影响可见度
    moon_visibility *= (1.0f - weather_intensity / 200.0f);
    
    int moon_x = WINDOW_WIDTH * 3 / 4;
    int moon_y = POND_HEIGHT / 4;
    int moon_radius = 40;
    
    // 应用摄像机偏移到月亮位置，但效果较小以模拟远距离
    int projected_moon_x = (int)project_x(moon_x, 0.1f);
    
    // 月亮主体
    Uint8 moon_brightness = (Uint8)(230 * moon_visibility);

    // 闪电会增加月亮亮度
    if (lightning_flash) {
        moon_brightness = (Uint8)fminf(255, moon_brightness + flash_brightness);
    }
    
    SDL_Rect moon_rect = {
        projected_moon_x - 40,
        moon_y -40,
        80, 80
    };

    /* draw the texture */
    SDL_SetTextureColorMod(
        moon_texture,
        moon_brightness,
        moon_brightness,
        (Uint8)(moon_brightness * 0.9f)
    );
    SDL_RenderCopy(renderer, moon_texture, NULL, &moon_rect);
    
    // 在暴风雨或中雨时绘制云层
    if (current_weather >= WEATHER_MEDIUM_RAIN) {
        int cloud_layers = 3;
        if (current_weather == WEATHER_HEAVY_RAIN) cloud_layers = 5;
        if (current_weather == WEATHER_THUNDERSTORM) cloud_layers = 7;
        cloud_layers = (int)(cloud_layers * (0.7f + weather_intensity / 100.0f * 0.6f));
        Uint32 current_time = SDL_GetTicks();
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        for (int layer = cloud_layers-1; layer >= 0; layer--) {
            // 计算云层位移（不同层以不同速度移动）
            cloud_offsets[layer] = (cloud_offsets[layer] + cloud_layers) % (WINDOW_WIDTH*2);
            
            // 设置纹理透明度
            SDL_SetTextureAlphaMod(cloud_textures[layer], 255);
            
            // 绘制双倍宽度纹理实现无缝滚动
            SDL_Rect src_rect = { cloud_offsets[layer], 0, WINDOW_WIDTH, 200 };
            SDL_Rect dest_rect = { 0, 0, WINDOW_WIDTH, 200 };
            SDL_RenderCopy(renderer, cloud_textures[layer], &src_rect, &dest_rect);
            
            // 绘制剩余部分实现循环
            if (cloud_offsets[layer] > WINDOW_WIDTH) {
                SDL_Rect src_remain = { 0, 0, WINDOW_WIDTH*2 - cloud_offsets[layer], 200 };
                SDL_Rect dest_remain = { cloud_offsets[layer] - WINDOW_WIDTH, 0, 
                                        WINDOW_WIDTH*2 - cloud_offsets[layer], 200 };
                SDL_RenderCopy(renderer, cloud_textures[layer], &src_remain, &dest_remain);
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }
     
    // 绘制闪电
    for (int i = 0; i < MAX_LIGHTNING; i++) {
        if (lightnings[i].active) {
            SDL_SetRenderDrawColor(renderer, lightnings[i].brightness, lightnings[i].brightness, lightnings[i].brightness, 255);
            
            // 绘制主闪电路径
            for (int j = 0; j < lightnings[i].segments; j++) {
                // 绘制粗线条
                for (int w = -lightnings[i].width; w <= lightnings[i].width; w++) {
                    SDL_RenderDrawLine(renderer, 
                                      lightnings[i].points[j][0] + w, 
                                      lightnings[i].points[j][1],
                                      lightnings[i].points[j+1][0] + w, 
                                      lightnings[i].points[j+1][1]);
                }
            }
            
            // 绘制闪电光晕
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, lightnings[i].brightness, lightnings[i].brightness, lightnings[i].brightness, 50);
            
            for (int j = 0; j < lightnings[i].segments; j++) {
                // 绘制更宽的半透明线条
                for (int w = -lightnings[i].width*3; w <= lightnings[i].width*3; w++) {
                    SDL_RenderDrawLine(renderer, 
                                      lightnings[i].points[j][0] + w, 
                                      lightnings[i].points[j][1],
                                      lightnings[i].points[j+1][0] + w, 
                                      lightnings[i].points[j+1][1]);
                }
            }
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
    }
    
    // 绘制远山（3D背景）
    for (int i = 0; i < MOUNTAIN_COUNT; i++) {
        // 计算投影后的山位置
        int mountain_proj_x = (int)project_x(mountains[i].x_offset, mountains[i].z);
        
        // 绘制山脉轮廓
        SDL_Color base_color = mountains[i].color;
        int peak_x = mountain_proj_x + WINDOW_WIDTH / 2;
        int base_y = POND_HEIGHT;
        int peak_y = base_y - mountains[i].height;
        
        // 预计算颜色变化范围
        int color_variation = 5; // 减少颜色变化幅度
        
        // 填充山体（优化为按行绘制）
        for (int y = peak_y; y <= base_y; y++) {
            float height_ratio = (float)(y - peak_y) / (base_y - peak_y);
            int current_width = (int)(mountains[i].width * height_ratio);
            int start_x = peak_x - current_width / 2;
            int end_x = peak_x + current_width / 2;
            
            if (current_width <= 0) continue;
            
            // 计算本行颜色
            SDL_Color line_color = base_color;
            line_color.r = (Uint8)fmin(255, fmax(0, line_color.r));
            line_color.g = (Uint8)fmin(255, fmax(0, line_color.g));
            line_color.b = (Uint8)fmin(255, fmax(0, line_color.b));
            
            // 闪电效果
            if (lightning_flash) {
                line_color.r = (Uint8)fmin(255, line_color.r + flash_brightness);
                line_color.g = (Uint8)fmin(255, line_color.g + flash_brightness);
                line_color.b = (Uint8)fmin(255, line_color.b + flash_brightness);
            }
            
            SDL_SetRenderDrawColor(renderer, line_color.r, line_color.g, line_color.b, line_color.a);
            
            // 绘制整行（使用填充矩形代替逐点绘制）
            SDL_Rect line_rect = {
                start_x < 0 ? 0 : start_x,  // 限制在窗口范围内
                y,
                end_x - start_x + 1,
                1
            };
            if (line_rect.x + line_rect.w > WINDOW_WIDTH) {
                line_rect.w = WINDOW_WIDTH - line_rect.x;
            }
            if (line_rect.w > 0) {
                SDL_RenderFillRect(renderer, &line_rect);
            }
        }
    }
    
    // 绘制荷塘背景
    SDL_SetRenderDrawColor(renderer, 0, 30, 60, 255);  // 深蓝色荷塘
    SDL_Rect pond_rect = {0, POND_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT - POND_HEIGHT};
    SDL_RenderFillRect(renderer, &pond_rect);
    
    // 绘制芦苇（受风影响摇摆）
    float time_seconds = SDL_GetTicks() / 1000.0f;
    
    for (int i = 0; i < REED_COUNT; i++) {
        // 计算投影位置
        int proj_x = (int)project_x(reeds[i].x, reeds[i].z);
        
        // 只绘制在屏幕内的芦苇
        if (proj_x >= -10 && proj_x < WINDOW_WIDTH + 10) {
            // 风力影响芦苇摇摆幅度
            float wind_factor = wind_strength * 30.0f;
            
            // 摇摆角度计算 - 使用正弦函数
            float sway_angle = sinf(time_seconds * reeds[i].sway_speed + reeds[i].sway_offset) * 
                               (0.1f + fabsf(wind_strength) * 0.5f); // 风越大摇摆越厉害
            
            // 芦苇颜色 - 随深度调整
            Uint8 green_value = (Uint8)(100 + reeds[i].z * 50);
            
            // 闪电会照亮芦苇
            if (lightning_flash) {
                SDL_SetRenderDrawColor(renderer, 
                                      (Uint8)fminf(255, 30 + flash_brightness), 
                                      (Uint8)fminf(255, green_value + flash_brightness), 
                                      (Uint8)fminf(255, 10 + flash_brightness), 
                                      255);
            } else {
                SDL_SetRenderDrawColor(renderer, 30, green_value, 10, 255);
            }
            
            // 绘制芦苇茎
            int stem_height = (int)(reeds[i].height * 0.7f);
            int stem_end_x = proj_x + (int)(stem_height * sinf(sway_angle));
            int stem_end_y = (int)reeds[i].y - stem_height;
            
            SDL_RenderDrawLine(renderer, proj_x, (int)reeds[i].y, stem_end_x, stem_end_y);
            
            // 绘制芦苇叶
            int leaf_length = (int)(reeds[i].height * 0.5f);
            
            // 左叶
            int leaf1_end_x = stem_end_x + (int)(leaf_length * sinf(sway_angle - 0.3f));
            int leaf1_end_y = stem_end_y - (int)(leaf_length * cosf(sway_angle - 0.3f));
            SDL_RenderDrawLine(renderer, stem_end_x, stem_end_y, leaf1_end_x, leaf1_end_y);
            
            // 右叶
            int leaf2_end_x = stem_end_x + (int)(leaf_length * sinf(sway_angle + 0.3f));
            int leaf2_end_y = stem_end_y - (int)(leaf_length * cosf(sway_angle + 0.3f));
            SDL_RenderDrawLine(renderer, stem_end_x, stem_end_y, leaf2_end_x, leaf2_end_y);
        }
    }
    
    // 绘制荷叶
    for (int i = 0; i < LOTUS_PAD_COUNT; i++) {
        // 计算投影坐标
        int proj_x = (int)project_x(lotus_pads[i].x, lotus_pads[i].z);
        
        // 只绘制在屏幕内的荷叶
        if (proj_x + (int)lotus_pads[i].radius >= 0 && 
            proj_x - (int)lotus_pads[i].radius < WINDOW_WIDTH) {
            
            // 应用倾斜效果 - 创建椭圆
            float tilt = lotus_pads[i].tilt_angle + wind_strength * 0.2f;
            float h_radius = lotus_pads[i].radius;
            float v_radius = lotus_pads[i].radius * (0.5f - tilt * 0.2f);
            
            render_lotus_texture(&lotus_pads[i], proj_x, tilt);
        }
    }
    
    // 绘制荷花
    for (int i = 0; i < LOTUS_FLOWER_COUNT; i++) {
        // 计算投影坐标
        int proj_x = (int)project_x(lotus_flowers[i].x, lotus_flowers[i].z);
        
        // 只绘制在屏幕内的荷花
        if (proj_x + (int)lotus_flowers[i].size >= 0 && 
            proj_x - (int)lotus_flowers[i].size < WINDOW_WIDTH) {
            
            // 风的影响
            float wind_sway = sinf(time_seconds + lotus_flowers[i].sway_phase) * wind_strength * 5.0f;
            
            // 荷花茎
            SDL_SetRenderDrawColor(renderer, 0, 100, 50, 255);
            SDL_RenderDrawLine(renderer,
                             proj_x + (int)wind_sway, lotus_flowers[i].y + (int)lotus_flowers[i].size,
                             proj_x, POND_HEIGHT);
            
            // 荷花颜色可能被闪电影响
            SDL_Color flower_color = lotus_flowers[i].color;
            if (lightning_flash) {
                flower_color.r = (Uint8)fminf(255, flower_color.r + flash_brightness);
                flower_color.g = (Uint8)fminf(255, flower_color.g + flash_brightness);
                flower_color.b = (Uint8)fminf(255, flower_color.b + flash_brightness);
            }
            
            // 绘制荷花花瓣
            for (int p = 0; p < lotus_flowers[i].petal_count; p++) {
                float angle = p * 6.28f / lotus_flowers[i].petal_count + time_seconds * 0.1f;
                
                // 内层花瓣
                SDL_SetRenderDrawColor(renderer, flower_color.r, flower_color.g, flower_color.b, 255);
                for (int r = 0; r < lotus_flowers[i].size; r++) {
                    float petal_width = sinf(r / lotus_flowers[i].size * 3.14f) * lotus_flowers[i].size * 0.5f;
                    for (int w = -(int)petal_width; w <= (int)petal_width; w++) {
                        int petal_x = proj_x + (int)(r * cosf(angle)) + w + (int)wind_sway;
                        int petal_y = lotus_flowers[i].y + (int)(r * sinf(angle));
                        
                        if (petal_x >= 0 && petal_x < WINDOW_WIDTH && 
                            petal_y >= 0 && petal_y < WINDOW_HEIGHT) {
                            SDL_RenderDrawPoint(renderer, petal_x, petal_y);
                        }
                    }
                }
            }
            
            // 荷花中心
            SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255);
            int center_size = (int)(lotus_flowers[i].size * 0.3f);
            for (int y = -center_size; y <= center_size; y++) {
                for (int x = -center_size; x <= center_size; x++) {
                    if (x*x + y*y <= center_size*center_size) {
                        int cx = proj_x + x + (int)wind_sway;
                        int cy = lotus_flowers[i].y + y;
                        
                        if (cx >= 0 && cx < WINDOW_WIDTH && cy >= 0 && cy < WINDOW_HEIGHT) {
                            SDL_RenderDrawPoint(renderer, cx, cy);
                        }
                    }
                }
            }
        }
    }
    
    // 绘制涟漪
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (ripples[i].active) {
            // 计算投影坐标
            int proj_x = (int)project_x(ripples[i].x, ripples[i].z);
            
            // 如果涟漪在屏幕上
            if (proj_x + (int)ripples[i].radius >= 0 && 
                proj_x - (int)ripples[i].radius < WINDOW_WIDTH) {
                
                // 根据深度调整颜色
                SDL_Color adjusted_color = adjust_color_by_depth(ripples[i].color, ripples[i].z);
                
                // 闪电可能会影响涟漪颜色
                if (lightning_flash) {
                    adjusted_color.r = (Uint8)fminf(255, adjusted_color.r + flash_brightness / 2);
                    adjusted_color.g = (Uint8)fminf(255, adjusted_color.g + flash_brightness / 2);
                    adjusted_color.b = (Uint8)fminf(255, adjusted_color.b + flash_brightness / 2);
                }
                
                // 设置颜色并考虑透明度
                SDL_SetRenderDrawColor(renderer, 
                                      adjusted_color.r,
                                      adjusted_color.g,
                                      adjusted_color.b,
                                      adjusted_color.a);
                
                // 根据深度计算实际半径
                float z_scale = get_z_scale(ripples[i].z);
                int radius = (int)(ripples[i].radius * z_scale);
                
                // 椭圆压缩系数 - 根据y位置不同而变化，实现透视效果
                float y_perspective = (ripples[i].y - POND_HEIGHT) / (WINDOW_HEIGHT - POND_HEIGHT);
                float ellipse_factor = 0.3f + y_perspective * 0.2f;
                
                // 绘制多个细线条的椭圆
                for (int r = radius - 2; r <= radius; r++) {
                    // 计算圆周上的点并绘制
                    for (int angle = 0; angle < 360; angle += 5) {
                        float rad = angle * 3.14159f / 180.0f;
                        int x = (int)(proj_x + r * cosf(rad));
                        int y = (int)(ripples[i].y + r * ellipse_factor * sinf(rad));
                        
                        if (x >= 0 && x < WINDOW_WIDTH && y >= POND_HEIGHT && y < WINDOW_HEIGHT) {
                            SDL_RenderDrawPoint(renderer, x, y);
                        }
                    }
                }
            }
        }
    }
    
    // 绘制溅射水珠
    for (int i = 0; i < MAX_SPLASHES; i++) {
        if (splashes[i].active) {
            // 计算投影坐标
            int proj_x = (int)project_x(splashes[i].x, splashes[i].z);
            
            // 根据深度调整大小
            float z_scale = get_z_scale(splashes[i].z);
            int size = (int)(splashes[i].size * z_scale);
            
            // 只绘制在屏幕内的水珠
            if (proj_x >= 0 && proj_x < WINDOW_WIDTH && 
                splashes[i].y >= 0 && splashes[i].y < WINDOW_HEIGHT) {
                
                // 根据深度调整颜色
                SDL_Color adjusted_color = adjust_color_by_depth(splashes[i].color, splashes[i].z);
                
                // 闪电会影响水珠颜色
                if (lightning_flash) {
                    adjusted_color.r = (Uint8)fminf(255, adjusted_color.r + flash_brightness / 2);
                    adjusted_color.g = (Uint8)fminf(255, adjusted_color.g + flash_brightness / 2);
                    adjusted_color.b = (Uint8)fminf(255, adjusted_color.b + flash_brightness / 2);
                }
                
                // 设置颜色
                SDL_SetRenderDrawColor(renderer, 
                                      adjusted_color.r,
                                      adjusted_color.g,
                                      adjusted_color.b,
                                      adjusted_color.a);
                
                // 绘制水珠 - 小圆点
                for (int y = -size; y <= size; y++) {
                    for (int x = -size; x <= size; x++) {
                        if (x*x + y*y <= size*size) {
                            int px = proj_x + x;
                            int py = (int)splashes[i].y + y;
                            
                            if (px >= 0 && px < WINDOW_WIDTH && py >= 0 && py < WINDOW_HEIGHT) {
                                SDL_RenderDrawPoint(renderer, px, py);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 绘制雨滴
    for (int i = 0; i < MAX_RAINDROPS; i++) {
        if (raindrops[i].active && !raindrops[i].in_water) {
            // 计算投影坐标
            int proj_x = (int)project_x(raindrops[i].x, raindrops[i].z);
            
            // 根据深度调整大小
            float z_scale = get_z_scale(raindrops[i].z);
            int actual_size = (int)(raindrops[i].size * z_scale);
            
            // 只绘制在屏幕内的雨滴
            if (proj_x >= 0 && proj_x < WINDOW_WIDTH && 
                raindrops[i].y >= 0 && raindrops[i].y < WINDOW_HEIGHT) {
                
                // 根据深度调整颜色
                SDL_Color adjusted_color = adjust_color_by_depth(raindrops[i].color, raindrops[i].z);
                
                // 闪电会增亮雨滴
                if (lightning_flash) {
                    adjusted_color.r = (Uint8)fminf(255, adjusted_color.r + flash_brightness / 2);
                    adjusted_color.g = (Uint8)fminf(255, adjusted_color.g + flash_brightness / 2);
                    adjusted_color.b = (Uint8)fminf(255, adjusted_color.b + flash_brightness / 2);
                }
                
                // 设置颜色
                SDL_SetRenderDrawColor(renderer, 
                                      adjusted_color.r,
                                      adjusted_color.g,
                                      adjusted_color.b,
                                      adjusted_color.a);
                
                // 计算雨滴的倾斜角度 - 受风影响
                float rain_angle = wind_strength * 0.7f; // -0.7 到 0.7 弧度
                
                // 雨滴长度受强度影响
                int drop_length = actual_size * (1 + weather_intensity / 100);
                
                // 计算雨滴起点和终点 - 考虑风力倾斜
                int end_x = proj_x;
                int end_y = (int)raindrops[i].y;
                int start_x = end_x - (int)(drop_length * sinf(rain_angle));
                int start_y = end_y - (int)(drop_length * cosf(rain_angle));
                
                // 雨滴间歇性出现，以获得更真实的效果
                Uint32 current_time = SDL_GetTicks();
                int visibility = (current_time / 50) % 5;  // 0-4循环
                
                if (visibility < 3) {  // 在5个时间单位中可见3个
                    // 绘制雨滴（短线）- 考虑风力倾斜
                    SDL_RenderDrawLine(renderer, start_x, start_y, end_x, end_y);
                }
            }
        }
    }
    
    // 模拟雷声视觉效果 - 屏幕部分闪烁
    if (thunder_active) {
        Uint32 current_time = SDL_GetTicks();
        Uint32 thunder_age = current_time - thunder_start_time;
        
        if (thunder_age < thunder_duration) {
            // 余弦波模拟雷声强度变化
            float thunder_intensity = cosf(thunder_age * 3.14159f * 5 / thunder_duration) * 0.5f + 0.5f;
            thunder_intensity *= (1.0f - (float)thunder_age / thunder_duration); // 随时间衰减
            
            if (thunder_intensity > 0.05f) {
                // 在屏幕底部显示震动效果
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, (Uint8)(50 * thunder_intensity));
                
                // 在底部随机绘制一些线条模拟震动
                int lines = (int)(20 * thunder_intensity);
                for (int j = 0; j < lines; j++) {
                    int y = WINDOW_HEIGHT - rand() % 100;
                    int length = 20 + rand() % 100;
                    int x = rand() % (WINDOW_WIDTH - length);
                    
                    SDL_RenderDrawLine(renderer, x, y, x + length, y);
                }
                
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
        }
    }
    
    // 绘制天气状态信息
    render_weather_info();
}

// 绘制天气信息
void render_weather_info() {
    // 在屏幕左上角显示当前天气信息和控制提示
    char* weather_name;
    SDL_Color weather_color;
    
    // 根据当前天气设置名称和颜色
    switch (current_weather) {
        case WEATHER_LIGHT_RAIN:
            weather_name = "和风细雨";
            weather_color.r = 100;
            weather_color.g = 200;
            weather_color.b = 255;
            break;
        case WEATHER_MEDIUM_RAIN:
            weather_name = "中雨";
            weather_color.r = 80;
            weather_color.g = 150;
            weather_color.b = 200;
            break;
        case WEATHER_HEAVY_RAIN:
            weather_name = "暴风骤雨";
            weather_color.r = 50;
            weather_color.g = 100;
            weather_color.b = 150;
            break;
        case WEATHER_THUNDERSTORM:
            weather_name = "电闪雷鸣";
            weather_color.r = 30;
            weather_color.g = 70;
            weather_color.b = 120;
            break;
        default:
            weather_name = "未知天气";
            weather_color.r = 255;
            weather_color.g = 255;
            weather_color.b = 255;
    }
    
    // 绘制半透明背景
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Rect info_bg = {10, 10, 250, 80};
    SDL_RenderFillRect(renderer, &info_bg);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    
    // 绘制当前天气状态指示点
    SDL_SetRenderDrawColor(renderer, weather_color.r, weather_color.g, weather_color.b, 255);
    
    SDL_Rect weather_indicator = {20, 20, 15, 15};
    SDL_RenderFillRect(renderer, &weather_indicator);
    
    // 绘制天气强度条
    int intensity_bar_width = 200;
    int intensity_bar_x = 20;
    int intensity_bar_y = 45;
    
    // 强度条背景
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect intensity_bar_bg = {intensity_bar_x, intensity_bar_y, intensity_bar_width, 10};
    SDL_RenderFillRect(renderer, &intensity_bar_bg);
    
    // 强度条填充
    SDL_SetRenderDrawColor(renderer, weather_color.r, weather_color.g, weather_color.b, 255);
    SDL_Rect intensity_indicator = {intensity_bar_x, intensity_bar_y, intensity_bar_width * weather_intensity / 100, 10};
    SDL_RenderFillRect(renderer, &intensity_indicator);
    
    // 绘制风力指示条
    int wind_bar_width = 200;
    int wind_bar_x = 20;
    int wind_bar_y = 65;
    
    // 风力条背景
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect wind_bar_bg = {wind_bar_x, wind_bar_y, wind_bar_width, 10};
    SDL_RenderFillRect(renderer, &wind_bar_bg);
    
    // 风力条填充
    int wind_pos = wind_bar_width / 2 + (int)(wind_strength * wind_bar_width / 2);
    SDL_SetRenderDrawColor(renderer, 150, 150, 255, 255);
    SDL_Rect wind_indicator = {wind_bar_x + wind_pos - 3, wind_bar_y, 6, 10};
    SDL_RenderFillRect(renderer, &wind_indicator);
    
    // 中心线
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderDrawLine(renderer, wind_bar_x + wind_bar_width/2, wind_bar_y, 
                      wind_bar_x + wind_bar_width/2, wind_bar_y + 10);
    
    // 检查是否有雷声，如果有则显示闪烁的"雷声"指示器
    if (thunder_active) {
        Uint32 current_time = SDL_GetTicks();
        if ((current_time / 100) % 2 == 0) { // 闪烁效果
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_Rect thunder_indicator = {170, 20, 15, 15};
            SDL_RenderFillRect(renderer, &thunder_indicator);
        }
    }
}