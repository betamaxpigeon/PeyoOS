typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

#ifdef PEYO_CUSTOM_FONT
#include "generated_font.h"
#endif

bool equals(const char* left, const char* right);

struct multiboot_tag {
  uint32_t type;
  uint32_t size;
};

struct multiboot_tag_framebuffer {
  uint32_t type;
  uint32_t size;
  uint64_t framebuffer_addr;
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t  framebuffer_bpp;
  uint8_t  framebuffer_type;
  uint16_t reserved;
  uint8_t  red_position;
  uint8_t  red_mask_size;
  uint8_t  green_position;
  uint8_t  green_mask_size;
  uint8_t  blue_position;
  uint8_t  blue_mask_size;
};

uint8_t* framebuffer = nullptr;
uint64_t framebuffer_address = 0;
uint32_t framebuffer_pitch = 0;
uint32_t width = 0;
uint32_t height = 0;
uint8_t framebuffer_bpp = 0;
uint8_t framebuffer_type = 0;
uint8_t red_position = 0;
uint8_t red_mask_size = 0;
uint8_t green_position = 0;
uint8_t green_mask_size = 0;
uint8_t blue_position = 0;
uint8_t blue_mask_size = 0;
uint32_t cursor_x = 24;
uint32_t cursor_y = 48;

static inline uint32_t terminal_margin_x() {
  return (width > 96) ? 24u : 12u;
}

static inline uint32_t terminal_margin_y() {
  return (height > 160) ? 48u : 20u;
}

static inline uint32_t terminal_char_width() {
#ifdef PEYO_CUSTOM_FONT
  return PEYO_FONT_WIDTH * 2u + 2u;
#else
  return 12u;
#endif
}

static inline uint32_t terminal_char_height() {
#ifdef PEYO_CUSTOM_FONT
  return PEYO_FONT_HEIGHT * 2u + 2u;
#else
  return 18u;
#endif
}

static inline uint32_t terminal_columns() {
  if (width <= terminal_margin_x() * 2) return 1u;
  return (width - terminal_margin_x() * 2) / terminal_char_width();
}

void debug_write(const char* text) {
  while (*text) {
    asm volatile("outb %0, %1" : : "a"((unsigned char)*text), "Nd"(0xe9));
    text++;
  }
}

void debug_write_number(uint32_t value) {
  char digits[10];
  uint32_t digit_count = 0;
  do {
    digits[digit_count++] = '0' + value % 10;
    value /= 10;
  } while (value != 0);
  while (digit_count > 0) {
    char digit = digits[--digit_count];
    asm volatile("outb %0, %1" : : "a"((unsigned char)digit), "Nd"(0xe9));
  }
}

static const uint8_t font[44][7] = {
  {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
  {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
  {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
  {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
  {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
  {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
  {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
  {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
  {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
  {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
  {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
  {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8}, {14,17,17,14,17,17,14},
  {14,17,17,15,1,1,14}, {0,0,4,0,0,4,0}, {0,0,4,0,0,4,8},
  {4,4,4,4,4,0,4}, {14,17,1,2,4,0,4}, {0,0,31,0,0,0,0},
  {8,4,2,1,2,4,8}, {0,0,0,0,0,0,0}, {0,0,0,0,0,4,4}
};

static const uint8_t lowercase_font[26][7] = {
  {0,0,14,17,31,17,17}, {0,0,30,17,30,17,30}, {0,0,14,17,16,17,14},
  {0,0,15,17,15,1,30}, {0,0,31,16,30,16,31}, {0,0,31,16,30,16,16},
  {0,0,15,17,23,17,15}, {0,0,17,17,31,17,17}, {0,0,14,4,4,4,14},
  {0,0,30,1,1,17,14}, {0,0,17,18,20,18,17}, {0,0,16,16,16,16,31},
  {0,0,17,27,21,17,17}, {0,0,17,25,21,19,17}, {0,0,14,17,17,17,14},
  {0,0,30,17,17,30,16}, {0,0,14,17,17,21,18}, {0,0,30,17,30,20,17},
  {0,0,15,16,14,1,30}, {0,0,31,4,4,4,4}, {0,0,17,17,17,17,14},
  {0,0,17,17,17,10,4}, {0,0,17,17,21,21,10}, {0,0,17,10,4,10,17},
  {0,0,17,17,10,4,4}, {0,0,31,2,4,8,31}
};

static uint32_t color(uint8_t red, uint8_t green, uint8_t blue) {
  return ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
}

void put_pixel(int x, int y, uint32_t color) {
  if (x >= 0 && x < (int)width && y >= 0 && y < (int)height) {
    uint32_t bytes_per_pixel = (framebuffer_bpp + 7) / 8;
    uint8_t* pixel = framebuffer + (uint64_t)y * framebuffer_pitch + (uint64_t)x * bytes_per_pixel;
    uint32_t red = (color >> 16) & 0xff;
    uint32_t green = (color >> 8) & 0xff;
    uint32_t blue = color & 0xff;
    uint32_t packed =
      ((red >> (8 - red_mask_size)) << red_position) |
      ((green >> (8 - green_mask_size)) << green_position) |
      ((blue >> (8 - blue_mask_size)) << blue_position);

    if (framebuffer_bpp == 32) {
      *(volatile uint32_t*)pixel = packed;
    } else if (framebuffer_bpp == 24) {
      pixel[0] = packed & 0xff;
      pixel[1] = (packed >> 8) & 0xff;
      pixel[2] = (packed >> 16) & 0xff;
    } else if (framebuffer_bpp == 16 || framebuffer_bpp == 15) {
      *(volatile uint16_t*)pixel = (uint16_t)packed;
    }
  }
}

void fill_rect(int x, int y, int rect_width, int rect_height, uint32_t fill) {
  for (int row = 0; row < rect_height; row++) {
    for (int column = 0; column < rect_width; column++) {
      put_pixel(x + column, y + row, fill);
    }
  }
}

void draw_char(char character, uint32_t x, uint32_t y, uint32_t foreground) {
#ifdef PEYO_CUSTOM_FONT
  if (character < 32 || character > 126) return;
  const uint16_t* glyph = peyo_font_bitmap[character - 32];

  for (uint32_t row = 0; row < PEYO_FONT_HEIGHT; row++) {
    for (uint32_t column = 0; column < PEYO_FONT_WIDTH; column++) {
      if (glyph[row] & (1u << (PEYO_FONT_WIDTH - 1u - column))) {
        put_pixel(x + column * 2, y + row * 2, foreground);
        put_pixel(x + column * 2 + 1, y + row * 2, foreground);
        put_pixel(x + column * 2, y + row * 2 + 1, foreground);
        put_pixel(x + column * 2 + 1, y + row * 2 + 1, foreground);
      }
    }
  }
#else
  const uint8_t* glyph = nullptr;

  if (character >= 'A' && character <= 'Z') {
    glyph = font[character - 'A'];
  } else if (character >= 'a' && character <= 'z') {
    glyph = lowercase_font[character - 'a'];
  } else if (character >= '0' && character <= '9') {
    glyph = font[26 + character - '0'];
  } else {
    switch (character) {
      case ':': glyph = font[36]; break;
      case ';': glyph = font[36]; break;
      case ',': glyph = font[37]; break;
      case '!': glyph = font[38]; break;
      case '?': glyph = font[39]; break;
      case '-': glyph = font[40]; break;
      case '>': glyph = font[41]; break;
      case '.': glyph = font[43]; break;
      case '/': glyph = font[42]; break;
      case '_': glyph = font[43]; break;
      case '+': glyph = font[41]; break;
      case '=': glyph = font[43]; break;
      case '(': glyph = font[41]; break;
      case ')': glyph = font[42]; break;
      case '[': glyph = font[41]; break;
      case ']': glyph = font[42]; break;
      case '\'': glyph = font[37]; break;
      case '"': glyph = font[39]; break;
      default: return;
    }
  }

  if (glyph == nullptr) return;

  for (uint32_t row = 0; row < 7; row++) {
    for (uint32_t column = 0; column < 5; column++) {
      if (glyph[row] & (1 << (4 - column))) {
        put_pixel(x + column * 2, y + row * 2, foreground);
        put_pixel(x + column * 2 + 1, y + row * 2, foreground);
        put_pixel(x + column * 2, y + row * 2 + 1, foreground);
        put_pixel(x + column * 2 + 1, y + row * 2 + 1, foreground);
      }
    }
  }
#endif
}

void scroll_terminal() {
  const uint32_t scroll_pixels = terminal_char_height();
  uint32_t start_row = terminal_margin_y();
  for (uint32_t row = start_row; row + scroll_pixels < height; row++) {
    for (uint32_t column = 0; column < framebuffer_pitch; column++) {
      framebuffer[row * framebuffer_pitch + column] =
        framebuffer[(row + scroll_pixels) * framebuffer_pitch + column];
    }
  }
  fill_rect(0, height - scroll_pixels, width, scroll_pixels, color(18, 25, 35));
  cursor_y -= scroll_pixels;
}

void put_char(char character) {
  uint32_t left_margin = terminal_margin_x();
  if (character == '\n') {
    cursor_x = left_margin;
    cursor_y += terminal_char_height();
  } else {
    draw_char(character, cursor_x, cursor_y, color(221, 229, 238));
    cursor_x += terminal_char_width();
    if (cursor_x + terminal_char_width() >= width || cursor_x >= width - left_margin) {
      cursor_x = left_margin;
      cursor_y += terminal_char_height();
    }
  }

  if (cursor_y + terminal_char_height() >= height) scroll_terminal();
}

void write(const char* text) {
  while (*text) put_char(*text++);
}

void clear_terminal() {
  fill_rect(0, 0, width, height, color(18, 25, 35));
  fill_rect(0, 0, width, 30, color(30, 43, 57));
  fill_rect(0, 30, width, 2, color(58, 184, 157));
  cursor_x = terminal_margin_x();
  cursor_y = terminal_margin_y();
}

uint8_t read_scancode() {
  uint8_t scan_code;
  do {
    asm volatile("inb %1, %0" : "=a"(scan_code) : "Nd"(0x64));
  } while ((scan_code & 1) == 0);
  asm volatile("inb %1, %0" : "=a"(scan_code) : "Nd"(0x60));
  return scan_code;
}

char key_from_scancode(uint8_t scan_code, bool shifted) {
  char key = 0;
  switch (scan_code) {
    case 0x02: key = '1'; break; case 0x03: key = '2'; break;
    case 0x04: key = '3'; break; case 0x05: key = '4'; break;
    case 0x06: key = '5'; break; case 0x07: key = '6'; break;
    case 0x08: key = '7'; break; case 0x09: key = '8'; break;
    case 0x0a: key = '9'; break; case 0x0b: key = '0'; break;
    case 0x0c: key = '-'; break; case 0x0d: key = '='; break;
    case 0x10: key = 'q'; break; case 0x11: key = 'w'; break;
    case 0x12: key = 'e'; break; case 0x13: key = 'r'; break;
    case 0x14: key = 't'; break; case 0x15: key = 'y'; break;
    case 0x16: key = 'u'; break; case 0x17: key = 'i'; break;
    case 0x18: key = 'o'; break; case 0x19: key = 'p'; break;
    case 0x1a: key = '['; break; case 0x1b: key = ']'; break;
    case 0x1e: key = 'a'; break; case 0x1f: key = 's'; break;
    case 0x20: key = 'd'; break; case 0x21: key = 'f'; break;
    case 0x22: key = 'g'; break; case 0x23: key = 'h'; break;
    case 0x24: key = 'j'; break; case 0x25: key = 'k'; break;
    case 0x26: key = 'l'; break; case 0x27: key = ';'; break;
    case 0x28: key = '\''; break; case 0x29: key = '`'; break;
    case 0x2b: key = '\\'; break; case 0x2c: key = 'z'; break;
    case 0x2d: key = 'x'; break; case 0x2e: key = 'c'; break;
    case 0x2f: key = 'v'; break; case 0x30: key = 'b'; break;
    case 0x31: key = 'n'; break; case 0x32: key = 'm'; break;
    case 0x33: key = ','; break; case 0x34: key = '.'; break;
    case 0x35: key = '/'; break; case 0x39: key = ' '; break;
    default: break;
  }
  if (shifted && key >= 'a' && key <= 'z') key -= 'a' - 'A';
  if (shifted && key == '1') key = '!';
  if (shifted && key == '2') key = '@';
  if (shifted && key == '3') key = '#';
  if (shifted && key == '4') key = '$';
  if (shifted && key == '5') key = '%';
  if (shifted && key == '6') key = '^';
  if (shifted && key == '7') key = '&';
  if (shifted && key == '8') key = '*';
  if (shifted && key == '9') key = '(';
  if (shifted && key == '0') key = ')';
  if (shifted && key == '-') key = '_';
  if (shifted && key == '=') key = '+';
  if (shifted && key == ';') key = ':';
  if (shifted && key == '\'') key = '"';
  if (shifted && key == ',') key = '<';
  if (shifted && key == '.') key = '>';
  if (shifted && key == '/') key = '?';
  if (shifted && key == '[') key = '{';
  if (shifted && key == ']') key = '}';
  if (shifted && key == '\\') key = '|';
  return key;
}

struct InMemoryFile {
  char name[24];
  char content[256];
  uint32_t length;
  bool is_directory;
};

static InMemoryFile g_files[32];
static uint32_t g_file_count = 0;

static void fs_copy_string(char* dst, const char* src, uint32_t dst_size) {
  uint32_t index = 0;
  while (src[index] != 0 && index + 1 < dst_size) {
    dst[index] = src[index];
    index++;
  }
  dst[index] = 0;
}

static bool fs_add_file(const char* name, const char* content, bool is_directory) {
  if (g_file_count >= 32) return false;

  for (uint32_t i = 0; i < g_file_count; i++) {
    if (equals(g_files[i].name, name)) {
      return false;
    }
  }

  InMemoryFile& file = g_files[g_file_count++];
  fs_copy_string(file.name, name, sizeof(file.name));
  file.length = 0;
  file.is_directory = is_directory;
  if (content != nullptr) {
    while (content[file.length] != 0 && file.length + 1 < sizeof(file.content)) {
      file.content[file.length] = content[file.length];
      file.length++;
    }
    file.content[file.length] = 0;
  } else {
    file.content[0] = 0;
  }
  return true;
}

static InMemoryFile* fs_find_file(const char* name) {
  for (uint32_t i = 0; i < g_file_count; i++) {
    if (equals(g_files[i].name, name)) return &g_files[i];
  }
  return nullptr;
}

static void fs_init() {
  g_file_count = 0;
  fs_add_file("/", "", true);
  fs_add_file("bin", "", true);
  fs_add_file("etc", "", true);
  fs_add_file("README", "Welcome to PeyoOS.\nType help for commands.\n", false);
  fs_add_file("hello.txt", "Hello from Peyo Shell!\n", false);
  fs_add_file("motd", "PeyoOS is real!!\n", false);
}

static void fs_list_files() {
  for (uint32_t i = 0; i < g_file_count; i++) {
    if (g_files[i].name[0] == 0) continue;
    write(g_files[i].name);
    if (g_files[i].is_directory) {
      write("/");
    }
    put_char('\n');
  }
}

static void fs_print_file(const char* name) {
  InMemoryFile* file = fs_find_file(name);
  if (file == nullptr) {
    write("File not found.");
    put_char('\n');
    return;
  }
  if (file->is_directory) {
    write("This is a directory.");
    put_char('\n');
    return;
  }
  write(file->content);
}

static void fs_touch_file(const char* name) {
  if (name == nullptr || name[0] == 0) {
    write("Usage: touch <file>");
    put_char('\n');
    return;
  }
  if (fs_find_file(name) == nullptr) {
    fs_add_file(name, "", false);
  }
}

static void fs_mkdir(const char* name) {
  if (name == nullptr || name[0] == 0) {
    write("Usage: mkdir <directory>");
    put_char('\n');
    return;
  }
  if (fs_find_file(name) == nullptr) {
    fs_add_file(name, "", true);
  }
}

static const char* skip_spaces(const char* text) {
  while (*text == ' ' || *text == '\t') {
    text++;
  }
  return text;
}

static void read_arg(const char* text, char* buffer, uint32_t buffer_size) {
  const char* start = skip_spaces(text);
  uint32_t index = 0;
  while (start[index] != 0 && start[index] != ' ' && start[index] != '\t' && index + 1 < buffer_size) {
    buffer[index] = start[index];
    index++;
  }
  buffer[index] = 0;
}

bool equals(const char* left, const char* right) {
  while (*left && *right && *left == *right) {
    left++;
    right++;
  }
  return *left == *right;
}

void execute_command(const char* command) {
  const char* args = skip_spaces(command);

  if (equals(args, "help")) {
    write("Commands: help, clear, about, echo, ls, cat, mkdir, touch, pwd, sh");
    put_char('\n');
  } else if (equals(args, "clear")) {
    clear_terminal();
  } else if (equals(args, "about")) {
    put_char('\n');
    write("PeyoOS is a basic operating system built from scratch based off of Ubuntu, made by VHSCrow.");
    put_char('\n');
  } else if (equals(args, "pwd")) {
    write("/");
    put_char('\n');
  } else if (equals(args, "ls")) {
    fs_list_files();
  } else if (equals(args, "sh")) {
    write("PeyoSH v0.1 ready");
    put_char('\n');
  } else if (command[0] == 'e' && command[1] == 'c' && command[2] == 'h' && command[3] == 'o' && command[4] == ' ') {
    write(command + 5);
    put_char('\n');
  } else if (command[0] == 'c' && command[1] == 'a' && command[2] == 't' && command[3] == ' ') {
    char filename[64];
    read_arg(command + 4, filename, sizeof(filename));
    fs_print_file(filename);
    put_char('\n');
  } else if (command[0] == 'm' && command[1] == 'k' && command[2] == 'd' && command[3] == 'i' && command[4] == 'r' && command[5] == ' ') {
    char dirname[64];
    read_arg(command + 6, dirname, sizeof(dirname));
    fs_mkdir(dirname);
  } else if (command[0] == 't' && command[1] == 'o' && command[2] == 'u' && command[3] == 'c' && command[4] == 'h' && command[5] == ' ') {
    char filename[64];
    read_arg(command + 6, filename, sizeof(filename));
    fs_touch_file(filename);
  } else if (command[0] != 0) {
    write("Unknown command. Type help.");
    put_char('\n');
  }
}

extern "C" void kernel_main(uint32_t magic, uint64_t addr) {
  debug_write("kernel_main\n");
  if (magic != 0x36d76289) {
    debug_write("invalid multiboot magic\n");
    return;
  }

  uint32_t* current_tag = (uint32_t*)(addr + 8);
  while (*current_tag != 0) {
    struct multiboot_tag* tag = (struct multiboot_tag*)current_tag;

    if (tag->type == 8) {
      struct multiboot_tag_framebuffer* fb_tag = (struct multiboot_tag_framebuffer*)tag;
      framebuffer_address = fb_tag->framebuffer_addr;
      framebuffer = (uint8_t*)framebuffer_address;
      framebuffer_pitch = fb_tag->framebuffer_pitch;
      width = fb_tag->framebuffer_width;
      height = fb_tag->framebuffer_height;
      framebuffer_bpp = fb_tag->framebuffer_bpp;
      framebuffer_type = fb_tag->framebuffer_type;
      red_position = fb_tag->red_position;
      red_mask_size = fb_tag->red_mask_size;
      green_position = fb_tag->green_position;
      green_mask_size = fb_tag->green_mask_size;
      blue_position = fb_tag->blue_position;
      blue_mask_size = fb_tag->blue_mask_size;
      break;
    }
    current_tag += ((tag->size + 7) & ~7) / 4;
  }

  if (framebuffer == nullptr) {
    debug_write("no framebuffer tag\n");
    return;
  }
  debug_write("framebuffer mode: ");
  debug_write_number(width);
  debug_write("x");
  debug_write_number(height);
  debug_write("x");
  debug_write_number(framebuffer_bpp);
  debug_write(" type ");
  debug_write_number(framebuffer_type);
  debug_write(" pitch ");
  debug_write_number(framebuffer_pitch);
  debug_write(" address-high ");
  debug_write_number((uint32_t)(framebuffer_address >> 32));
  debug_write(" address-low ");
  debug_write_number((uint32_t)framebuffer_address);
  debug_write("\n");

  uint32_t bytes_per_pixel = (framebuffer_bpp + 7) / 8;
  if (framebuffer_type != 1 ||
      (framebuffer_bpp != 16 && framebuffer_bpp != 24 && framebuffer_bpp != 32) ||
      framebuffer_pitch < width * bytes_per_pixel || height < 80 ||
      red_mask_size == 0 || green_mask_size == 0 || blue_mask_size == 0) {
    debug_write("unsupported framebuffer mode\n");
    return;
  }

  debug_write("framebuffer ready\n");

  fs_init();

  debug_write("clear start\n");
  clear_terminal();
  debug_write("clear done\n");
  write("PEYO OS | SYS CONSOLE");
  put_char('\n');
  write("Type help to see available commands.");
  put_char('\n');
  debug_write("text drawn\n");

  char command[128];
  uint32_t command_length = 0;
  bool shifted = false;
  write("peyosh> ");

  while (true) {
    uint8_t scan_code = read_scancode();
    if (scan_code == 0x2a || scan_code == 0x36) {
      shifted = true;
      continue;
    }
    if (scan_code == 0xaa || scan_code == 0xb6) {
      shifted = false;
      continue;
    }
    if (scan_code & 0x80) continue;

    if (scan_code == 0x1c) {
      put_char('\n');
      command[command_length] = 0;
      execute_command(command);
      command_length = 0;
      write("psh> ");
    } else if (scan_code == 0x0e && command_length > 0) {
      command[--command_length] = 0;
      cursor_x -= terminal_char_width();
      fill_rect(cursor_x, cursor_y, terminal_char_width(), terminal_char_height(), color(18, 25, 35));
    } else if (command_length < sizeof(command) - 1) {
      char key = key_from_scancode(scan_code, shifted);
      if (key != 0) {
        command[command_length++] = key;
        put_char(key);
      }
    }
  }
}
