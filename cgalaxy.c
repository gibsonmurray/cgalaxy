#define _XOPEN_SOURCE 700

#include <errno.h>
#include <locale.h>
#include <math.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CGALAXY_VERSION "0.1.0"
#define MAX_PARTICLES 6000
#define MAX_BACKGROUND 900
#define PI 3.14159265358979323846

typedef struct {
    float radius;
    float angle;
    float height;
    float brightness;
    float phase;
    uint8_t arm;
    uint8_t type;
} Particle;

typedef struct {
    float x;
    float y;
    float brightness;
    float phase;
} BackgroundStar;

typedef struct {
    float score;
    const char *glyph;
    short pair;
    attr_t attrs;
} Cell;

typedef struct {
    const char *name;
    short colors[6];
} Palette;

typedef struct {
    int fps;
    int density;
    float speed;
    int palette;
    int arms;
    bool ascii;
    bool paused;
} Options;

static const Palette palettes[] = {
    {"milky-way",   {17,  24,  67, 110, 153, 231}},
    {"andromeda",   {17,  53,  96, 139, 183, 225}},
    {"ultraviolet", {17,  54,  91, 129, 201, 231}},
    {"ember",       {52,  88, 130, 166, 214, 230}},
    {"ice",         {17,  18,  25,  39,  87, 195}}
};

static const size_t palette_count = sizeof(palettes) / sizeof(palettes[0]);
static float random_unit(void)
{
    return (float)rand() / (float)RAND_MAX;
}

static float random_normal(void)
{
    float u1 = random_unit();
    float u2 = random_unit();
    if (u1 < 0.000001f) {
        u1 = 0.000001f;
    }
    return sqrtf(-2.0f * logf(u1)) * cosf((float)(2.0 * PI) * u2);
}

static int clamp_int(int value, int low, int high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static float clamp_float(float value, float low, float high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static void usage(FILE *stream)
{
    fprintf(stream,
        "usage: cgalaxy [options]\n"
        "\n"
        "a slowly rotating spiral galaxy for your terminal\n"
        "\n"
        "options:\n"
        "  -f FPS     frames per second, 5-60 (default: 24)\n"
        "  -d NUM     star density, 1-10 (default: 6)\n"
        "  -s NUM     rotation speed, 0-10 (default: 3)\n"
        "  -p NAME    palette (default: milky-way)\n"
        "  -a         ASCII glyphs only\n"
        "  -h         show this help\n"
        "  -v         show version\n"
        "\n"
        "palettes:\n"
        "  milky-way, andromeda, ultraviolet, ember, ice\n"
        "\n"
        "keys:\n"
        "  q / Esc    quit\n"
        "  space      pause / resume\n"
        "  r          form a new galaxy\n"
        "  c / C      next / previous palette\n"
        "  a          toggle ASCII glyphs\n"
        "  d / D      more / fewer stars\n"
        "  + / -      faster / slower\n"
        "  [ / ]      fewer / more spiral arms\n");
}

static int palette_index(const char *name)
{
    size_t i;
    for (i = 0; i < palette_count; i++) {
        if (strcmp(name, palettes[i].name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static bool parse_int(const char *text, int *value)
{
    char *end = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed < 0 || parsed > 1000) {
        return false;
    }
    *value = (int)parsed;
    return true;
}

static int parse_options(int argc, char **argv, Options *options)
{
    int opt;
    int value;

    while ((opt = getopt(argc, argv, "f:d:s:p:ahv")) != -1) {
        switch (opt) {
        case 'f':
            if (!parse_int(optarg, &value) || value < 5 || value > 60) {
                fprintf(stderr, "cgalaxy: FPS must be between 5 and 60\n");
                return -1;
            }
            options->fps = value;
            break;
        case 'd':
            if (!parse_int(optarg, &value) || value < 1 || value > 10) {
                fprintf(stderr, "cgalaxy: density must be between 1 and 10\n");
                return -1;
            }
            options->density = value;
            break;
        case 's':
            if (!parse_int(optarg, &value) || value < 0 || value > 10) {
                fprintf(stderr, "cgalaxy: speed must be between 0 and 10\n");
                return -1;
            }
            options->speed = (float)value;
            break;
        case 'p':
            value = palette_index(optarg);
            if (value < 0) {
                fprintf(stderr, "cgalaxy: unknown palette '%s'\n", optarg);
                return -1;
            }
            options->palette = value;
            break;
        case 'a':
            options->ascii = true;
            break;
        case 'h':
            usage(stdout);
            return 1;
        case 'v':
            printf("cgalaxy %s\n", CGALAXY_VERSION);
            return 1;
        default:
            usage(stderr);
            return -1;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "cgalaxy: unexpected argument '%s'\n", argv[optind]);
        return -1;
    }
    return 0;
}

static void form_galaxy(Particle *particles, BackgroundStar *background,
                        int arms)
{
    int i;
    float bulge_fraction;

    bulge_fraction = 0.18f + random_unit() * 0.10f;

    for (i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &particles[i];
        float u = random_unit();

        p->arm = (uint8_t)(rand() % arms);
        p->phase = random_unit() * (float)(2.0 * PI);
        p->type = (uint8_t)(rand() % 100);

        if (u < bulge_fraction) {
            p->radius = powf(random_unit(), 1.8f) * 0.34f;
            p->angle = random_unit() * (float)(2.0 * PI);
            p->height = random_normal() * (0.10f - p->radius * 0.16f);
            p->brightness = 0.58f + random_unit() * 0.42f;
        } else {
            float radius = 0.13f + powf(random_unit(), 0.72f) * 0.87f;
            float arm_angle = (float)(2.0 * PI) * (float)p->arm / (float)arms;
            float spread = 0.06f + radius * 0.24f;

            p->radius = radius;
            p->angle = arm_angle + radius * 5.7f + random_normal() * spread;
            p->height = random_normal() * (0.042f * (1.08f - radius));
            p->brightness = 0.28f + random_unit() * 0.55f;
            if (p->type > 93) {
                p->brightness = 0.85f + random_unit() * 0.15f;
            }
        }
    }

    for (i = 0; i < MAX_BACKGROUND; i++) {
        background[i].x = random_unit();
        background[i].y = random_unit();
        background[i].brightness = 0.15f + random_unit() * 0.58f;
        background[i].phase = random_unit() * (float)(2.0 * PI);
    }
}

static void recolor(void)
{
    size_t p;
    int level;
    bool extended = COLORS >= 256;

    for (p = 0; p < palette_count; p++) {
        for (level = 0; level < 6; level++) {
            short color = palettes[p].colors[level];
            short fallback[] = {
                COLOR_BLUE, COLOR_CYAN, COLOR_MAGENTA,
                COLOR_WHITE, COLOR_YELLOW, COLOR_WHITE
            };
            if (!extended) {
                color = fallback[level];
            }
            init_pair((short)(p * 6 + level + 1), color, -1);
        }
    }
}

static void put_cell(Cell *cells, int rows, int cols, int y, int x,
                     float score, const char *glyph, short pair, attr_t attrs)
{
    Cell *cell;

    if (x < 0 || x >= cols || y < 0 || y >= rows) {
        return;
    }
    cell = &cells[y * cols + x];
    if (score > cell->score) {
        cell->score = score;
        cell->glyph = glyph;
        cell->pair = pair;
        cell->attrs = attrs;
    }
}

static void draw_frame(Cell *cells, const Particle *particles,
                       const BackgroundStar *background, const Options *options,
                       int rows, int cols, double elapsed)
{
    static const char *unicode_glyphs[] = {"·", "·", "•", "✦", "✶", "●"};
    static const char *ascii_glyphs[] = {".", ".", ":", "*", "+", "@"};
    int particle_count;
    int background_count;
    int i;
    int cx = cols / 2;
    int cy = rows / 2;
    float radius_x = (float)(cols - 2) * 0.48f;
    float radius_y = (float)(rows - 2) * 0.47f;
    float tilt = 0.94f;
    float cos_tilt = cosf(tilt);
    float sin_tilt = sinf(tilt);
    const char **glyphs = options->ascii ? ascii_glyphs : unicode_glyphs;

    memset(cells, 0, (size_t)rows * (size_t)cols * sizeof(*cells));
    particle_count = clamp_int(rows * cols * options->density / 10, 250,
                               MAX_PARTICLES);
    background_count = clamp_int(rows * cols / 13, 40, MAX_BACKGROUND);

    for (i = 0; i < background_count; i++) {
        const BackgroundStar *star = &background[i];
        float twinkle = 0.72f + 0.28f *
            sinf((float)elapsed * 1.6f + star->phase);
        float value = star->brightness * twinkle;
        int level = value > 0.60f ? 2 : (value > 0.36f ? 1 : 0);
        int x = clamp_int((int)(star->x * (float)cols), 0, cols - 1);
        int y = clamp_int((int)(star->y * (float)rows), 0, rows - 1);

        put_cell(cells, rows, cols, y, x, value * 0.45f, glyphs[level],
                 (short)(options->palette * 6 + level + 1), A_DIM);
    }

    for (i = 0; i < particle_count; i++) {
        const Particle *p = &particles[i];
        float differential = 1.22f - p->radius * 0.48f;
        /*
         * Terminal rows increase downward, so decreasing the mathematical
         * angle produces counterclockwise motion on screen.
         */
        float theta = p->angle - (float)elapsed * options->speed * 0.075f *
            differential;
        float xg = cosf(theta) * p->radius;
        float yg = sinf(theta) * p->radius;
        float projected_y = yg * cos_tilt - p->height * sin_tilt;
        float depth = yg * sin_tilt + p->height * cos_tilt;
        float core = expf(-p->radius * p->radius * 22.0f);
        float twinkle = 0.88f + 0.12f *
            sinf((float)elapsed * 2.1f + p->phase);
        float value = clamp_float(p->brightness * twinkle + core * 0.62f,
                                  0.0f, 1.0f);
        float perspective = 1.0f + depth * 0.08f;
        int x = cx + (int)(xg * radius_x * perspective);
        int y = cy + (int)(projected_y * radius_y * perspective);
        int level = clamp_int((int)(value * 5.6f), 0, 5);
        float score = value + depth * 0.08f + 0.45f;
        attr_t attrs = level >= 4 ? A_BOLD : (level <= 1 ? A_DIM : A_NORMAL);

        put_cell(cells, rows, cols, y, x, score, glyphs[level],
                 (short)(options->palette * 6 + level + 1), attrs);

        if (p->radius < 0.11f && value > 0.74f) {
            int halo_level = clamp_int(level - 2, 1, 3);
            put_cell(cells, rows, cols, y, x - 1, score - 0.40f,
                     glyphs[halo_level],
                     (short)(options->palette * 6 + halo_level + 1), A_DIM);
            put_cell(cells, rows, cols, y, x + 1, score - 0.40f,
                     glyphs[halo_level],
                     (short)(options->palette * 6 + halo_level + 1), A_DIM);
        }
    }

    erase();
    for (i = 0; i < rows * cols; i++) {
        if (cells[i].glyph != NULL) {
            int y = i / cols;
            int x = i % cols;
            attrset(COLOR_PAIR(cells[i].pair) | cells[i].attrs);
            mvaddstr(y, x, cells[i].glyph);
        }
    }

    if (rows >= 4 && cols >= 38) {
        char status[96];
        snprintf(status, sizeof(status), " %s  %d arms  %s%s ",
                 palettes[options->palette].name, options->arms,
                 options->paused ? "paused  " : "",
                 options->ascii ? "ASCII" : "q: quit");
        attrset(COLOR_PAIR((short)(options->palette * 6 + 3)) | A_DIM);
        mvaddnstr(rows - 1, 1, status, cols - 2);
    }
    refresh();
}

static double monotonic_seconds(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec / 1000000000.0;
}

static void sleep_until(double deadline)
{
    double remaining = deadline - monotonic_seconds();
    struct timespec delay;

    if (remaining <= 0.0) {
        return;
    }
    delay.tv_sec = (time_t)remaining;
    delay.tv_nsec = (long)((remaining - (double)delay.tv_sec) * 1000000000.0);
    nanosleep(&delay, NULL);
}

int main(int argc, char **argv)
{
    Particle *particles;
    BackgroundStar *background;
    Cell *cells = NULL;
    size_t cell_capacity = 0;
    Options options = {24, 6, 3.0f, 0, 3, false, false};
    double start;
    double pause_started = 0.0;
    double paused_total = 0.0;
    int rows;
    int cols;
    int result;
    bool running = true;

    result = parse_options(argc, argv, &options);
    if (result != 0) {
        return result < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    setlocale(LC_ALL, "");
    srand((unsigned int)(time(NULL) ^ (time_t)getpid()));
    particles = malloc(sizeof(*particles) * MAX_PARTICLES);
    background = malloc(sizeof(*background) * MAX_BACKGROUND);
    if (particles == NULL || background == NULL) {
        fprintf(stderr, "cgalaxy: out of memory\n");
        free(particles);
        free(background);
        return EXIT_FAILURE;
    }
    options.arms = 2 + rand() % 3;
    form_galaxy(particles, background, options.arms);

    if (initscr() == NULL) {
        fprintf(stderr, "cgalaxy: could not initialize the terminal\n");
        free(particles);
        free(background);
        return EXIT_FAILURE;
    }
    if (!has_colors()) {
        endwin();
        fprintf(stderr, "cgalaxy: this terminal does not support color\n");
        free(particles);
        free(background);
        return EXIT_FAILURE;
    }

    start_color();
    use_default_colors();
    recolor();
    cbreak();
    noecho();
    nonl();
    keypad(stdscr, true);
    nodelay(stdscr, true);
    curs_set(0);
    start = monotonic_seconds();

    while (running) {
        double frame_start = monotonic_seconds();
        double elapsed;
        int key;
        size_t required;

        getmaxyx(stdscr, rows, cols);
        required = (size_t)rows * (size_t)cols;
        if (required > cell_capacity) {
            Cell *larger = realloc(cells, required * sizeof(*larger));
            if (larger == NULL) {
                break;
            }
            cells = larger;
            cell_capacity = required;
        }

        key = getch();
        while (key != ERR) {
            switch (key) {
            case 'q':
            case 27:
                running = false;
                break;
            case ' ':
                options.paused = !options.paused;
                if (options.paused) {
                    pause_started = monotonic_seconds();
                } else {
                    paused_total += monotonic_seconds() - pause_started;
                }
                break;
            case 'r':
                options.arms = 2 + rand() % 3;
                form_galaxy(particles, background, options.arms);
                break;
            case 'c':
                options.palette = (options.palette + 1) % (int)palette_count;
                break;
            case 'C':
                options.palette = (options.palette + (int)palette_count - 1) %
                    (int)palette_count;
                break;
            case 'a':
                options.ascii = !options.ascii;
                break;
            case 'd':
                options.density = clamp_int(options.density + 1, 1, 10);
                break;
            case 'D':
                options.density = clamp_int(options.density - 1, 1, 10);
                break;
            case '+':
            case '=':
                options.speed = clamp_float(options.speed + 0.5f, 0.0f, 10.0f);
                break;
            case '-':
            case '_':
                options.speed = clamp_float(options.speed - 0.5f, 0.0f, 10.0f);
                break;
            case '[':
                options.arms = clamp_int(options.arms - 1, 2, 5);
                form_galaxy(particles, background, options.arms);
                break;
            case ']':
                options.arms = clamp_int(options.arms + 1, 2, 5);
                form_galaxy(particles, background, options.arms);
                break;
            default:
                break;
            }
            key = getch();
        }

        elapsed = (options.paused ? pause_started : monotonic_seconds()) -
            start - paused_total;
        draw_frame(cells, particles, background, &options, rows, cols, elapsed);
        sleep_until(frame_start + 1.0 / (double)options.fps);
    }

    curs_set(1);
    attrset(A_NORMAL);
    endwin();
    free(cells);
    free(particles);
    free(background);
    return running ? EXIT_FAILURE : EXIT_SUCCESS;
}
