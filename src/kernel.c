#include "../include/keyboard.h"
#include "../include/ports.h"
#include "../include/terminal.h"
#include "../include/fs.h"

// VGA
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000
#define SCROLL_BACK 100

unsigned short *vga = (unsigned short*) VGA_MEMORY;
int cursor_x = 0;
int cursor_y = 0;
unsigned char current_color = 0x0F;

unsigned short scroll_buf[(VGA_HEIGHT + SCROLL_BACK) * VGA_WIDTH];
int total_lines = 0;
int scroll_offset = 0;

// ─── LOGIN STATE ─────────────────────────────────────────────────────────────
#define MAX_USERS 4
typedef struct { char username[32]; char password[32]; } User;
static User users[MAX_USERS] = {
    { "sakshyam", "kryptonix" },
    { "root",     "toor"      },
    { "guest",    "guest"     },
    { "",         ""          },
};
static int  login_state    = 0;
static char login_username[32];
static char login_password[32];
static int  login_attempts = 0;
static int  logged_in_user = -1;

// ─── DESKTOP STATE ───────────────────────────────────────────────────────────
#define MODE_DESKTOP  0
#define MODE_TERMINAL 1
#define MODE_FILES    2
#define MODE_ABOUT    3

static int current_mode = MODE_DESKTOP;
static int desktop_sel  = 0;  // selected icon 0-7

// ─── VGA CORE ────────────────────────────────────────────────────────────────

unsigned short make_char(char c, unsigned char color) {
    return (unsigned short)c | ((unsigned short)color << 8);
}

void vga_put(int row, int col, char c, unsigned char color) {
    if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH)
        vga[row * VGA_WIDTH + col] = make_char(c, color);
}

void vga_puts(int row, int col, const char *s, unsigned char color) {
    int i = 0;
    while (s[i] && col + i < VGA_WIDTH) {
        vga_put(row, col + i, s[i], color);
        i++;
    }
}

void vga_fill(int row, int col, int w, int h, char c, unsigned char color) {
    int r, ci;
    for (r = row; r < row + h && r < VGA_HEIGHT; r++)
        for (ci = col; ci < col + w && ci < VGA_WIDTH; ci++)
            vga_put(r, ci, c, color);
}

void render_screen() {
    int start_line = total_lines - VGA_HEIGHT - scroll_offset;
    if (start_line < 0) start_line = 0;
    int i, row;
    for (row = 0; row < VGA_HEIGHT; row++) {
        int buf_row = start_line + row;
        for (i = 0; i < VGA_WIDTH; i++) {
            if (buf_row < (VGA_HEIGHT + SCROLL_BACK))
                vga[row * VGA_WIDTH + i] = scroll_buf[buf_row * VGA_WIDTH + i];
            else
                vga[row * VGA_WIDTH + i] = make_char(' ', 0x07);
        }
    }
}

void scroll_up() {
    if (scroll_offset < SCROLL_BACK) {
        scroll_offset += 3;
        if (scroll_offset > total_lines - VGA_HEIGHT)
            scroll_offset = total_lines - VGA_HEIGHT;
        render_screen();
    }
}

void scroll_down() {
    if (scroll_offset > 0) {
        scroll_offset -= 3;
        if (scroll_offset < 0) scroll_offset = 0;
        render_screen();
    }
}

void terminal_scroll() {
    int i;
    int max_lines = VGA_HEIGHT + SCROLL_BACK;
    for (i = 0; i < (max_lines - 1) * VGA_WIDTH; i++)
        scroll_buf[i] = scroll_buf[i + VGA_WIDTH];
    for (i = (max_lines - 1) * VGA_WIDTH; i < max_lines * VGA_WIDTH; i++)
        scroll_buf[i] = make_char(' ', 0x07);
    cursor_y = VGA_HEIGHT - 1;
}

void terminal_putchar(char c) {
    if (c == '\n') { cursor_x = 0; cursor_y++; }
    else if (c == '\b') {
        if (cursor_x > 0) cursor_x--;
        scroll_buf[cursor_y * VGA_WIDTH + cursor_x] = make_char(' ', 0x07);
    } else {
        scroll_buf[cursor_y * VGA_WIDTH + cursor_x] = make_char(c, current_color);
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) { cursor_x = 0; cursor_y++; }
    }
    if (cursor_y >= VGA_HEIGHT) { terminal_scroll(); total_lines++; }
    if (scroll_offset == 0) render_screen();
}

void terminal_backspace() { terminal_putchar('\b'); render_screen(); }

void print(const char *str) {
    int i = 0;
    while (str[i]) { terminal_putchar(str[i]); i++; }
}

void clear_screen() {
    int i;
    for (i = 0; i < (VGA_HEIGHT + SCROLL_BACK) * VGA_WIDTH; i++)
        scroll_buf[i] = make_char(' ', 0x07);
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = make_char(' ', 0x07);
    cursor_x = 0; cursor_y = 0;
    total_lines = VGA_HEIGHT; scroll_offset = 0;
}

// ─── UTILS ───────────────────────────────────────────────────────────────────

int streq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}

int startswith(const char *str, const char *prefix) {
    while (*prefix) { if (*str != *prefix) return 0; str++; prefix++; }
    return 1;
}

void str_copy(char *dst, const char *src, int max) {
    int i;
    for (i = 0; i < max - 1 && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
}

int str_len(const char *s) { int i = 0; while (s[i]) i++; return i; }

void delay_ms(int ms) { volatile int i; for (i = 0; i < ms * 1500; i++) {} }

// ─── CALC ────────────────────────────────────────────────────────────────────

int str_to_int(const char *s) {
    int result = 0, neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') { result = result * 10 + (*s - '0'); s++; }
    return neg ? -result : result;
}

void int_to_str(int n, char *buf) {
    if (n == 0) { buf[0] = '0'; buf[1] = 0; return; }
    int i = 0, neg = 0;
    if (n < 0) { neg = 1; n = -n; }
    char tmp[12];
    while (n > 0) { tmp[i++] = '0' + (n % 10); n /= 10; }
    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = 0;
}

void cmd_calc(const char *expr) {
    expr += 5;
    int a = str_to_int(expr);
    while (*expr && *expr != '+' && *expr != '-' && *expr != '*' && *expr != '/') expr++;
    char op = *expr; expr++;
    int b = str_to_int(expr);
    int result = 0, err = 0;
    if      (op == '+') result = a + b;
    else if (op == '-') result = a - b;
    else if (op == '*') result = a * b;
    else if (op == '/') {
        if (b == 0) { print("Error: divide by zero!"); err = 1; }
        else result = a / b;
    } else { print("Error: use +, -, *, /"); err = 1; }
    if (!err) { char buf[16]; int_to_str(result, buf); print("= "); print(buf); }
}

// ─── SNAKE ───────────────────────────────────────────────────────────────────

#define SNAKE_W  40
#define SNAKE_H  18
#define MAX_LEN  200

int snake_x[MAX_LEN], snake_y[MAX_LEN];
int snake_len, food_x, food_y, dir_x, dir_y, score, game_over;

static unsigned int rand_seed = 12345;
int my_rand() { rand_seed = rand_seed * 1103515245 + 12345; return (rand_seed >> 16) & 0x7FFF; }

void snake_draw_cell(int x, int y, char c, unsigned char color) {
    vga_put(y + 2, x + 1, c, color);
}
void snake_clear_cell(int x, int y) { snake_draw_cell(x, y, ' ', 0x07); }
void snake_place_food() { food_x = my_rand() % SNAKE_W; food_y = my_rand() % SNAKE_H; }

void run_snake() {
    int i;
    // draw snake window
    vga_fill(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', 0x07);
    // border
    for (i = 0; i < SNAKE_W + 2; i++) {
        vga_put(1, i, '-', 0x0A);
        vga_put(SNAKE_H + 2, i, '-', 0x0A);
    }
    for (i = 1; i <= SNAKE_H + 1; i++) {
        vga_put(i, 0, '|', 0x0A);
        vga_put(i, SNAKE_W + 1, '|', 0x0A);
    }
    vga_puts(0, 0, "[ SNAKE - WASD=move  Q=quit ]", 0x0E);
    vga_puts(1, 44, "SCORE:", 0x0E);

    snake_len=3; snake_x[0]=SNAKE_W/2; snake_y[0]=SNAKE_H/2;
    snake_x[1]=snake_x[0]-1; snake_y[1]=snake_y[0];
    snake_x[2]=snake_x[0]-2; snake_y[2]=snake_y[0];
    dir_x=1; dir_y=0; score=0; game_over=0;
    snake_place_food();

    while (!game_over) {
        unsigned char status = port_byte_in(0x64);
        if (status & 0x01) {
            unsigned char sc = port_byte_in(0x60);
            if (!(sc & 0x80)) {
                if (sc==17&&dir_y==0){dir_x=0;dir_y=-1;}
                if (sc==31&&dir_y==0){dir_x=0;dir_y=1;}
                if (sc==30&&dir_x==0){dir_x=-1;dir_y=0;}
                if (sc==32&&dir_x==0){dir_x=1;dir_y=0;}
                if (sc==16) { game_over=1; break; } // Q to quit
            }
        }
        delay_ms(20);
        snake_clear_cell(snake_x[snake_len-1], snake_y[snake_len-1]);
        for(i=snake_len-1;i>0;i--){snake_x[i]=snake_x[i-1];snake_y[i]=snake_y[i-1];}
        snake_x[0]+=dir_x; snake_y[0]+=dir_y;
        if(snake_x[0]<0||snake_x[0]>=SNAKE_W||snake_y[0]<0||snake_y[0]>=SNAKE_H){game_over=1;break;}
        for(i=1;i<snake_len;i++){if(snake_x[0]==snake_x[i]&&snake_y[0]==snake_y[i]){game_over=1;break;}}
        if(game_over)break;
        if(snake_x[0]==food_x&&snake_y[0]==food_y){score++;if(snake_len<MAX_LEN)snake_len++;snake_place_food();}
        snake_draw_cell(food_x,food_y,'*',0x0C);
        snake_draw_cell(snake_x[0],snake_y[0],'O',0x0B);
        for(i=1;i<snake_len;i++) snake_draw_cell(snake_x[i],snake_y[i],'o',0x09);
        // score
        char buf[16]; int_to_str(score,buf);
        vga_puts(1, 51, "     ", 0x0E);
        vga_puts(1, 51, buf, 0x0E);
    }
}

// ─── MATRIX ──────────────────────────────────────────────────────────────────

void run_matrix() {
    int i, j;
    unsigned char cols[VGA_WIDTH];
    unsigned char active[VGA_WIDTH];
    for (i = 0; i < VGA_WIDTH; i++) {
        cols[i] = my_rand() % VGA_HEIGHT;
        active[i] = my_rand() % 2;
    }
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) vga[i] = make_char(' ', 0x02);
    char *msg = "[ Press any key to exit ]";
    int msg_col = (VGA_WIDTH - 25) / 2;
    for (i = 0; msg[i]; i++) vga[(VGA_HEIGHT-1)*VGA_WIDTH+msg_col+i] = make_char(msg[i], 0x0A);

    while (1) {
        if (port_byte_in(0x64) & 0x01) {
            unsigned char sc = port_byte_in(0x60);
            if (!(sc & 0x80)) break;
        }
        delay_ms(50);
        for (j = 0; j < VGA_WIDTH; j++) {
            if (!active[j]) { if ((my_rand()%20)==0) active[j]=1; continue; }
            int head_row = cols[j];
            if (head_row < VGA_HEIGHT - 1) {
                char c = '!' + (my_rand() % 93);
                vga[head_row*VGA_WIDTH+j] = make_char(c, 0x0F);
                if (head_row > 0) vga[(head_row-1)*VGA_WIDTH+j] = make_char('!'+my_rand()%93, 0x0A);
                if (head_row > 3) vga[(head_row-3)*VGA_WIDTH+j] = make_char('!'+my_rand()%93, 0x02);
                if (head_row > 6) vga[(head_row-6)*VGA_WIDTH+j] = make_char(' ', 0x00);
                cols[j]++;
            } else {
                for (i = 0; i < VGA_HEIGHT-1; i++) vga[i*VGA_WIDTH+j] = make_char(' ', 0x00);
                cols[j]=0; active[j]=(my_rand()%3)!=0;
            }
        }
        for (i = 0; msg[i]; i++) vga[(VGA_HEIGHT-1)*VGA_WIDTH+msg_col+i] = make_char(msg[i], 0x0A);
    }
}

// ─── CLOCK ───────────────────────────────────────────────────────────────────

unsigned char read_rtc(unsigned char reg) { port_byte_out(0x70, reg); return port_byte_in(0x71); }
unsigned char bcd_to_bin(unsigned char bcd) { return (bcd & 0x0F) + ((bcd >> 4) * 10); }

void run_clock() {
    vga_fill(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', 0x01);
    vga_puts(0, 0, "[ CLOCK - Press any key to exit ]", 0x0B);
    unsigned char last_sec = 255;
    while (1) {
        if (port_byte_in(0x64) & 0x01) {
            unsigned char sc = port_byte_in(0x60);
            if (!(sc & 0x80)) break;
        }
        unsigned char sec  = bcd_to_bin(read_rtc(0x00));
        unsigned char min  = bcd_to_bin(read_rtc(0x02));
        unsigned char hour = bcd_to_bin(read_rtc(0x04));
        unsigned char day  = bcd_to_bin(read_rtc(0x07));
        unsigned char mon  = bcd_to_bin(read_rtc(0x08));
        unsigned char year = bcd_to_bin(read_rtc(0x09));
        if (sec == last_sec) { delay_ms(100); continue; }
        last_sec = sec;

        // big time display
        char tbuf[9];
        tbuf[0]='0'+(hour/10); tbuf[1]='0'+(hour%10); tbuf[2]=':';
        tbuf[3]='0'+(min/10);  tbuf[4]='0'+(min%10);  tbuf[5]=':';
        tbuf[6]='0'+(sec/10);  tbuf[7]='0'+(sec%10);  tbuf[8]=0;
        vga_puts(10, 36, tbuf, 0x0E);

        char dbuf[11];
        dbuf[0]='0'+(day/10); dbuf[1]='0'+(day%10); dbuf[2]='/';
        dbuf[3]='0'+(mon/10); dbuf[4]='0'+(mon%10); dbuf[5]='/';
        dbuf[6]='2'; dbuf[7]='0';
        dbuf[8]='0'+(year/10); dbuf[9]='0'+(year%10); dbuf[10]=0;
        vga_puts(12, 36, dbuf, 0x0B);

        vga_puts(9,  30, "  Time: ", 0x07);
        vga_puts(11, 30, "  Date: ", 0x07);
    }
}

// ─── SYSINFO ─────────────────────────────────────────────────────────────────

void show_sysinfo() {
    vga_fill(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', 0x01);
    vga_puts(0, 0, "[ SYSINFO - Press any key to exit ]", 0x0B);

    // ASCII logo
    vga_puts(3,  2, " ##   ##", 0x0A);
    vga_puts(4,  2, " ## # ##", 0x0A);
    vga_puts(5,  2, " #######", 0x0A);
    vga_puts(6,  2, " ## # ##", 0x0A);
    vga_puts(7,  2, " ##   ##", 0x0A);
    vga_puts(8,  2, "Kryptonix", 0x0B);

    char *labels[] = {"OS","Kernel","Shell","CPU","Memory","Display","Arch","Author"};
    char *values[] = {
        "Kryptonix OS v0.1",
        "Custom x86 32-bit",
        "Kryptonix Shell v1.0",
        "Intel Core i5 x86",
        "7.66 GiB Total",
        "VGA Text 80x25",
        "x86 32-bit",
        "Sakshyam Kharel"
    };

    int i;
    for (i = 0; i < 8; i++) {
        vga_puts(3 + i, 20, labels[i], 0x0B);
        vga_put(3 + i, 20 + str_len(labels[i]), ':', 0x0F);
        vga_puts(3 + i, 28, values[i], 0x0F);
    }

    // color palette
    for (i = 0; i < 16; i++) {
        vga_put(13, 20 + i*2,   ' ', (i << 4));
        vga_put(13, 20 + i*2+1, ' ', (i << 4));
    }

    while (1) {
        if (port_byte_in(0x64) & 0x01) {
            unsigned char sc = port_byte_in(0x60);
            if (!(sc & 0x80)) break;
        }
        delay_ms(100);
    }
}

// ─── FILE MANAGER ────────────────────────────────────────────────────────────

void show_files() {
    vga_fill(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', 0x17);
    vga_puts(0, 0,  "[ FILE MANAGER - Q=quit ]", 0x1E);
    vga_fill(1, 0, VGA_WIDTH, 1, '-', 0x17);
    vga_puts(2, 2, "Name                    Size", 0x1F);
    vga_fill(3, 0, VGA_WIDTH, 1, '-', 0x17);

    // list files using print into VGA directly
    // reset terminal to this area
    cursor_y = 4; cursor_x = 2;
    current_color = 0x1F;

    // we call fs_list which uses print()
    // but we need it to write to our blue area
    // so temporarily redirect
    fs_list();

    vga_puts(VGA_HEIGHT-1, 0, "  Arrow keys to navigate | Q to return to desktop", 0x17);

    while (1) {
        if (port_byte_in(0x64) & 0x01) {
            unsigned char sc = port_byte_in(0x60);
            if (!(sc & 0x80)) {
                if (sc == 16) break; // Q
            }
        }
        delay_ms(50);
    }
}

// ─── ABOUT WINDOW ────────────────────────────────────────────────────────────

void show_about() {
    vga_fill(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', 0x01);
    // draw window box
    int i;
    for (i = 1; i < VGA_WIDTH-1; i++) {
        vga_put(1, i, '=', 0x0B);
        vga_put(VGA_HEIGHT-2, i, '=', 0x0B);
    }
    for (i = 2; i < VGA_HEIGHT-2; i++) {
        vga_put(i, 1, '|', 0x0B);
        vga_put(i, VGA_WIDTH-2, '|', 0x0B);
    }

    vga_puts(0,  0,  "[ ABOUT KRYPTONIX - Any key to exit ]", 0x0B);
    vga_puts(3,  5,  "Kryptonix OS v0.1", 0x0E);
    vga_puts(4,  5,  "Built from scratch using C + x86 Assembly", 0x0F);
    vga_puts(6,  5,  "Developer  : Sakshyam Kharel", 0x0A);
    vga_puts(7,  5,  "Time taken : ~3 months", 0x0A);
    vga_puts(8,  5,  "Language   : C + x86 Assembly", 0x0A);
    vga_puts(9,  5,  "Bootloader : GRUB Multiboot", 0x0A);
    vga_puts(10, 5,  "Platform   : x86 32-bit bare metal", 0x0A);
    vga_puts(12, 5,  "Features:", 0x0E);
    vga_puts(13, 5,  " * Custom VGA + Keyboard drivers", 0x07);
    vga_puts(14, 5,  " * In-memory file system", 0x07);
    vga_puts(15, 5,  " * Multi-user login with passwords", 0x07);
    vga_puts(16, 5,  " * Snake, Matrix, Clock, Calculator", 0x07);
    vga_puts(17, 5,  " * Fake GUI Desktop with icons", 0x07);
    vga_puts(18, 5,  " * Boots on real hardware!", 0x07);
    vga_puts(20, 5,  "No Linux. No Windows. Pure code.", 0x0D);

    while (1) {
        if (port_byte_in(0x64) & 0x01) {
            unsigned char sc = port_byte_in(0x60);
            if (!(sc & 0x80)) break;
        }
        delay_ms(100);
    }
}

// ─── TERMINAL APP ────────────────────────────────────────────────────────────

// command history
#define HISTORY_SIZE 16
static char history[HISTORY_SIZE][256];
static int  history_count = 0;

void cmd_color(const char *arg) {
    arg += 6;
    if      (streq(arg, "red"))    { current_color = 0x0C; print("Color: red"); }
    else if (streq(arg, "green"))  { current_color = 0x0A; print("Color: green"); }
    else if (streq(arg, "blue"))   { current_color = 0x09; print("Color: blue"); }
    else if (streq(arg, "yellow")) { current_color = 0x0E; print("Color: yellow"); }
    else if (streq(arg, "cyan"))   { current_color = 0x0B; print("Color: cyan"); }
    else if (streq(arg, "white"))  { current_color = 0x0F; print("Color: white"); }
    else if (streq(arg, "pink"))   { current_color = 0x0D; print("Color: pink"); }
    else print("Colors: red green blue yellow cyan white pink");
}

void print_prompt() {
    current_color = 0x0A;
    print("\n[Kryptonix@");
    print(users[logged_in_user].username);
    print("] > ");
    current_color = 0x0F;
}

void shell(char *input);  // forward declaration

void run_terminal() {
    clear_screen();
    current_color = 0x0B;
    print("Kryptonix Terminal\n");
    current_color = 0x07;
    print("Type 'help' for commands. Type 'desktop' to return.\n");
    current_color = 0x0F;
    keyboard_init(shell);
    print_prompt();
    current_mode = MODE_TERMINAL;
}

void shell(char *input) {
    if (input[0] != 0 && history_count < HISTORY_SIZE)
        str_copy(history[history_count++], input, 256);

    if (streq(input, "desktop")) {
        current_mode = MODE_DESKTOP;
        return;
    } else if (streq(input, "help")) {
        current_color=0x0E; print("Commands:\n"); current_color=0x0F;
        print("  desktop        - return to desktop\n");
        print("  clear          - clear screen\n");
        print("  color <name>   - change color\n");
        print("  calc <expr>    - calculator\n");
        print("  ls             - list files\n");
        print("  create <n>     - create file\n");
        print("  write <f> <t>  - write to file\n");
        print("  append <f> <t> - append to file\n");
        print("  read <n>       - read file\n");
        print("  delete <n>     - delete file\n");
        print("  snake          - snake game\n");
        print("  matrix         - matrix rain\n");
        print("  clock          - live clock\n");
        print("  sysinfo        - system info\n");
        print("  whoami         - current user\n");
        print("  logout         - logout\n");
        print("  shutdown       - power off\n");
    } else if (streq(input, "clear")) {
        clear_screen();
    } else if (startswith(input, "color ")) {
        cmd_color(input);
    } else if (startswith(input, "calc ")) {
        cmd_calc(input);
    } else if (streq(input, "snake")) {
        run_snake();
        clear_screen();
        current_color=0x0B; print("Back in terminal. Type 'desktop' to go back.\n");
        current_color=0x0F;
    } else if (streq(input, "matrix")) {
        run_matrix();
        clear_screen();
    } else if (streq(input, "clock")) {
        run_clock();
        clear_screen();
    } else if (streq(input, "sysinfo")) {
        show_sysinfo();
        clear_screen();
    } else if (streq(input, "whoami")) {
        current_color=0x0B; print("User: ");
        current_color=0x0A; print(users[logged_in_user].username); print("\n");
        current_color=0x0F;
    } else if (streq(input, "ls")) {
        current_color=0x0E; print("Files:\n");
        current_color=0x0A; fs_list(); current_color=0x0F;
    } else if (startswith(input, "create ")) {
        int r=fs_create(input+7);
        if(r>=0){current_color=0x0A;print("Created!");current_color=0x0F;}
    } else if (startswith(input, "write ")) {
        const char *p=input+6; int i=0;
        while(p[i]&&p[i]!=' ')i++;
        if(p[i]==' '){
            char fname[32]; int j;
            for(j=0;j<i&&j<31;j++)fname[j]=p[j];fname[j]=0;
            fs_write(fname,p+i+1);
            current_color=0x0A;print("Written!");current_color=0x0F;
        } else print("Usage: write <file> <text>");
    } else if (startswith(input, "append ")) {
        const char *p=input+7; int i=0;
        while(p[i]&&p[i]!=' ')i++;
        if(p[i]==' '){
            char fname[32]; int j;
            for(j=0;j<i&&j<31;j++)fname[j]=p[j];fname[j]=0;
            fs_append(fname,p+i+1);
            current_color=0x0A;print("Appended!");current_color=0x0F;
        } else print("Usage: append <file> <text>");
    } else if (startswith(input, "read ")) {
        char *content=fs_read(input+5);
        if(content){current_color=0x0B;print(content);current_color=0x0F;}
        else{current_color=0x0C;print("File not found!");current_color=0x0F;}
    } else if (startswith(input, "delete ")) {
        fs_delete(input+7);
        current_color=0x0C;print("Deleted!");current_color=0x0F;
    } else if (streq(input, "logout")) {
        login_state=0; logged_in_user=-1; login_attempts=0;
        login_username[0]=0; login_password[0]=0;
        current_mode=MODE_DESKTOP;
        return;
    } else if (streq(input, "shutdown")) {
        clear_screen();
        current_color=0x0C; print("Shutting down...\n");
        port_byte_out(0x604, 0x00); port_byte_out(0x600, 0x34);
        asm volatile("cli; hlt");
    } else if (input[0]==0) {
        // empty
    } else {
        current_color=0x0C; print("Unknown: '"); print(input);
        print("' - type 'help'"); current_color=0x0F;
    }

    print_prompt();
}

// ─── DESKTOP ─────────────────────────────────────────────────────────────────

typedef struct {
    char *name;
    char *icon[3];
    unsigned char color;
} AppIcon;

static AppIcon icons[] = {
    { "Terminal", {" ___ "," |> |"," --- "}, 0x0A },
    { "Files   ", {"[===]","| F |","[===]"}, 0x0E },
    { "Snake   ", {" ~~~ ","(o)> ","     "}, 0x0C },
    { "Matrix  ", {"#|#|#","|#|#|","#|#|#"}, 0x02 },
    { "Clock   ", {" ___ ","( @ )","  -  "}, 0x0B },
    { "Sysinfo ", {"[CPU]","[MEM]","[SYS]"}, 0x0D },
    { "About   ", {" ??? "," (i) ","     "}, 0x09 },
    { "Shutdown", {" !!! ","[OFF]"," !!! "}, 0x0C },
};

#define NUM_ICONS 8

void draw_desktop() {
    int i, j;

    // clear with dark blue background
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = make_char(' ', 0x17);

    // taskbar at top
    for (i = 0; i < VGA_WIDTH; i++)
        vga[i] = make_char(' ', 0x70);

    // title in taskbar
    vga_puts(0, 1,  "Kryptonix OS v0.1", 0x70);
    vga_puts(0, 55, "By Sakshyam Kharel", 0x70);

    // clock in taskbar
    unsigned char sec  = bcd_to_bin(read_rtc(0x00));
    unsigned char min  = bcd_to_bin(read_rtc(0x02));
    unsigned char hour = bcd_to_bin(read_rtc(0x04));
    char tbuf[9];
    tbuf[0]='0'+(hour/10); tbuf[1]='0'+(hour%10); tbuf[2]=':';
    tbuf[3]='0'+(min/10);  tbuf[4]='0'+(min%10);  tbuf[5]=':';
    tbuf[6]='0'+(sec/10);  tbuf[7]='0'+(sec%10);  tbuf[8]=0;
    vga_puts(0, 36, tbuf, 0x74); // red on white

    // bottom bar
    for (i = 0; i < VGA_WIDTH; i++)
        vga[(VGA_HEIGHT-1)*VGA_WIDTH+i] = make_char(' ', 0x70);
    vga_puts(VGA_HEIGHT-1, 1, "Arrows=navigate  Enter=open  Kryptonix Desktop", 0x70);

    // draw icons in 2 rows of 4
    int positions[8][2] = {
        {3, 3},  {3, 22}, {3, 41}, {3, 60},
        {13, 3}, {13, 22},{13, 41},{13, 60}
    };

    for (i = 0; i < NUM_ICONS; i++) {
        int row = positions[i][0];
        int col = positions[i][1];
        unsigned char bg = (i == desktop_sel) ? 0x70 : 0x17;
        unsigned char fg = (i == desktop_sel) ? (0x70 | icons[i].color & 0x0F) : (0x10 | icons[i].color);

        // draw icon box
        for (j = row; j < row + 7; j++)
            vga_fill(j, col, 16, 1, ' ', bg);

        // icon art (3 lines)
        for (j = 0; j < 3; j++) {
            int art_col = col + (16 - 5) / 2;
            vga_puts(row + 1 + j, art_col, icons[i].icon[j], fg);
        }

        // icon name
        int name_col = col + (16 - 8) / 2;
        vga_puts(row + 5, name_col, icons[i].name, (i == desktop_sel) ? 0x74 : 0x1F);
    }
}

void desktop_launch(int sel) {
    switch (sel) {
        case 0: run_terminal(); break;
        case 1: show_files(); draw_desktop(); break;
        case 2: run_snake(); draw_desktop(); break;
        case 3: run_matrix(); draw_desktop(); break;
        case 4: run_clock(); draw_desktop(); break;
        case 5: show_sysinfo(); draw_desktop(); break;
        case 6: show_about(); draw_desktop(); break;
        case 7:
            // shutdown confirm
            vga_fill(10, 20, 40, 5, ' ', 0x4F);
            vga_puts(11, 25, "Shutdown Kryptonix?", 0x4F);
            vga_puts(12, 25, "Enter=Yes  Esc=Cancel", 0x4F);
            while (1) {
                if (port_byte_in(0x64) & 0x01) {
                    unsigned char sc = port_byte_in(0x60);
                    if (!(sc & 0x80)) {
                        if (sc == 0x1C) { // Enter
                            vga_fill(0, 0, VGA_WIDTH, VGA_HEIGHT, ' ', 0x00);
                            vga_puts(12, 30, "Shutting down...", 0x0C);
                            delay_ms(1000);
                            port_byte_out(0x604, 0x00);
                            port_byte_out(0x600, 0x34);
                            asm volatile("cli; hlt");
                        } else if (sc == 0x01) { // Esc
                            draw_desktop();
                            break;
                        }
                    }
                }
            }
            break;
    }
}

void desktop_poll() {
    if (!(port_byte_in(0x64) & 0x01)) return;
    unsigned char sc = port_byte_in(0x60);
    if (sc & 0x80) return;

    // arrow keys are extended (0xE0 prefix) — handle simple scancodes
    // left=0x4B right=0x4D up=0x48 down=0x50 enter=0x1C
    if (sc == 0x4B) { // left
        if (desktop_sel % 4 > 0) desktop_sel--;
        draw_desktop();
    } else if (sc == 0x4D) { // right
        if (desktop_sel % 4 < 3 && desktop_sel < NUM_ICONS-1) desktop_sel++;
        draw_desktop();
    } else if (sc == 0x48) { // up
        if (desktop_sel >= 4) desktop_sel -= 4;
        draw_desktop();
    } else if (sc == 0x50) { // down
        if (desktop_sel + 4 < NUM_ICONS) desktop_sel += 4;
        draw_desktop();
    } else if (sc == 0x1C) { // enter
        desktop_launch(desktop_sel);
    }
}

// ─── ANIMATED BOOT ───────────────────────────────────────────────────────────

void animated_boot() {
    int i, j;
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) vga[i] = make_char(' ', 0x00);
    delay_ms(300);

    char *title = "K R Y P T O N I X";
    int tlen = str_len(title);
    int tcol = (VGA_WIDTH - tlen) / 2;
    int trow = VGA_HEIGHT / 2 - 3;

    for (i = 0; title[i]; i++) {
        vga[trow * VGA_WIDTH + tcol + i] = make_char(title[i], 0x0A);
        delay_ms(60);
    }
    delay_ms(200);

    char *sub = "Operating System v0.1";
    int slen = str_len(sub);
    int scol = (VGA_WIDTH - slen) / 2;
    for (i = 0; sub[i]; i++) {
        vga[(trow+2)*VGA_WIDTH+scol+i] = make_char(sub[i], 0x0B);
        delay_ms(30);
    }
    delay_ms(100);

    char *auth = "By Sakshyam Kharel";
    int alen = str_len(auth);
    int acol = (VGA_WIDTH - alen) / 2;
    for (i = 0; auth[i]; i++) {
        vga[(trow+3)*VGA_WIDTH+acol+i] = make_char(auth[i], 0x07);
        delay_ms(30);
    }
    delay_ms(300);

    // loading bar
    int lrow = trow + 5;
    int lcol = (VGA_WIDTH - 20) / 2;
    char *loading = "Loading ";
    for (i = 0; loading[i]; i++) vga[lrow*VGA_WIDTH+lcol+i] = make_char(loading[i], 0x0F);
    vga[lrow*VGA_WIDTH+lcol+8]  = make_char('[', 0x07);
    vga[lrow*VGA_WIDTH+lcol+19] = make_char(']', 0x07);
    for (i = 0; i < 10; i++) {
        vga[lrow*VGA_WIDTH+lcol+9+i] = make_char('#', 0x0A);
        delay_ms(100);
    }
    delay_ms(200);

    // flash
    for (j = 0; j < 2; j++) {
        for (i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++) vga[i] ^= 0x7700;
        delay_ms(60);
        for (i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++) vga[i] ^= 0x7700;
        delay_ms(60);
    }

    for (i = 0; i < VGA_WIDTH*VGA_HEIGHT; i++) vga[i] = make_char(' ', 0x00);
    delay_ms(150);
}

// ─── LOGIN ───────────────────────────────────────────────────────────────────

void show_login_screen() {
    // draw a centered login box
    int i;
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) vga[i] = make_char(' ', 0x17);

    // box
    int brow = 7, bcol = 20, bw = 40, bh = 11;
    vga_fill(brow, bcol, bw, bh, ' ', 0x70);
    for (i = bcol; i < bcol+bw; i++) {
        vga_put(brow,    i, '=', 0x7B);
        vga_put(brow+bh-1, i, '=', 0x7B);
    }

    vga_puts(brow+1, bcol+10, "KRYPTONIX OS v0.1", 0x74);
    vga_puts(brow+2, bcol+8,  "By Sakshyam Kharel", 0x70);
    vga_puts(brow+3, bcol+1,  "========================================", 0x7B);
    vga_puts(brow+5, bcol+4,  "Users: sakshyam / root / guest", 0x78);
    vga_puts(brow+7, bcol+4,  "Username:", 0x70);
    vga_puts(brow+9, bcol+4,  "Password:", 0x70);

    // reset terminal cursor for input
    cursor_y = brow + 7;
    cursor_x = bcol + 14;
    // manually set scroll_buf state
    total_lines = VGA_HEIGHT;
    scroll_offset = 0;

    // draw the current screen into scroll_buf so terminal_putchar works
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        scroll_buf[i] = vga[i];

    // move cursor to username field
    cursor_y = brow + 7;
    cursor_x = bcol + 14;
    current_color = 0x70;

    login_state = 0;
}

void login_handler(char *input) {
    if (login_state == 0) {
        str_copy(login_username, input, 32);
        login_state = 1;
        // move to password field
        cursor_y = 18;
        cursor_x = 34;
        current_color = 0x70;
    } else if (login_state == 1) {
        str_copy(login_password, input, 32);
        int i, found = -1;
        for (i = 0; i < MAX_USERS; i++) {
            if (users[i].username[0] == 0) continue;
            if (streq(login_username, users[i].username) &&
                streq(login_password, users[i].password)) { found = i; break; }
        }
        if (found >= 0) {
            logged_in_user = found;
            login_state = 2;
            login_attempts = 0;
            current_mode = MODE_DESKTOP;
            desktop_sel = 0;
            draw_desktop();
        } else {
            login_attempts++;
            login_username[0] = 0; login_password[0] = 0;
            if (login_attempts >= 3) {
                for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) vga[i] = make_char(' ', 0x40);
                vga_puts(12, 25, "Too many attempts! Locked.", 0x4F);
                asm volatile("cli; hlt");
            } else {
                // flash red
                for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) vga[i] ^= 0x4000;
                delay_ms(200);
                show_login_screen();
                // show error
                vga_puts(20, 20, "Wrong username or password!", 0x1C);
            }
        }
    }
}

static char login_buf[256];
static int  login_buf_pos = 0;
static char keys_login[58] = {
    0,  0,  '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};

void login_poll() {
    unsigned char status = port_byte_in(0x64);
    if (!(status & 0x01)) return;
    unsigned char scancode = port_byte_in(0x60);
    if (scancode & 0x80) return;
    if (scancode >= 58) return;
    char c = keys_login[scancode];
    if (c == 0) return;

    if (c == '\b') {
        if (login_buf_pos > 0) {
            login_buf_pos--;
            login_buf[login_buf_pos] = 0;
            // erase from screen
            if (cursor_x > 0) cursor_x--;
            vga_put(cursor_y, cursor_x, ' ', 0x70);
        }
    } else if (c == '\n') {
        login_buf[login_buf_pos] = 0;
        login_handler(login_buf);
        login_buf_pos = 0; login_buf[0] = 0;
    } else {
        if (login_buf_pos < 255) {
            login_buf[login_buf_pos++] = c;
            if (login_state == 1) {
                vga_put(cursor_y, cursor_x, '*', 0x70);
            } else {
                vga_put(cursor_y, cursor_x, c, 0x70);
            }
            cursor_x++;
        }
    }
}

// ─── MAIN ────────────────────────────────────────────────────────────────────

void kernel_main() {
    fs_init();
    animated_boot();
    show_login_screen();

    while (1) {
        if (login_state < 2) {
            login_poll();
        } else if (current_mode == MODE_DESKTOP) {
            // update clock in taskbar every loop
            static unsigned char last_sec = 255;
            unsigned char sec = bcd_to_bin(read_rtc(0x00));
            if (sec != last_sec) {
                last_sec = sec;
                unsigned char min  = bcd_to_bin(read_rtc(0x02));
                unsigned char hour = bcd_to_bin(read_rtc(0x04));
                char tbuf[9];
                tbuf[0]='0'+(hour/10); tbuf[1]='0'+(hour%10); tbuf[2]=':';
                tbuf[3]='0'+(min/10);  tbuf[4]='0'+(min%10);  tbuf[5]=':';
                tbuf[6]='0'+(sec/10);  tbuf[7]='0'+(sec%10);  tbuf[8]=0;
                vga_puts(0, 36, tbuf, 0x74);
            }
            desktop_poll();
        } else if (current_mode == MODE_TERMINAL) {
            keyboard_poll();
            // check if returned to desktop
            if (current_mode == MODE_DESKTOP) {
                draw_desktop();
            }
        }
    }
}