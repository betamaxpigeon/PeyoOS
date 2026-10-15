typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned char uint8_t;

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
};

uint8_t* framebuffer = nullptr;
uint32_t framebuffer_pitch = 0;
uint32_t width = 0;
uint32_t height = 0;
uint32_t cursor_x = 24;
uint32_t cursor_y = 48;

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

static uint32_t color(uint8_t red, uint8_t green, uint8_t blue) {
  return ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
}

void put_pixel(int x, int y, uint32_t color) {
  if (x >= 0 && x < (int)width && y >= 0 && y < (int)height) {
    uint32_t* pixel = (uint32_t*)(framebuffer + y * framebuffer_pitch + x * 4);
    *pixel = color;
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
  if (character >= 'a' && character <= 'z') character -= 'a' - 'A';

  uint32_t glyph_index;
  if (character >= 'A' && character <= 'Z') {
    glyph_index = character - 'A';
  } else if (character >= '0' && character <= '9') {
    glyph_index = 26 + character - '0';
  } else {
    switch (character) {
      case ':': glyph_index = 36; break;
      case ',': glyph_index = 37; break;
      case '!': glyph_index = 38; break;
      case '?': glyph_index = 39; break;
      case '-': glyph_index = 40; break;
      case '>': glyph_index = 41; break;
      case '.': glyph_index = 43; break;
      default: return;
    }
  }

  for (uint32_t row = 0; row < 7; row++) {
    for (uint32_t column = 0; column < 5; column++) {
      if (font[glyph_index][row] & (1 << (4 - column))) {
        put_pixel(x + column * 2, y + row * 2, foreground);
        put_pixel(x + column * 2 + 1, y + row * 2, foreground);
        put_pixel(x + column * 2, y + row * 2 + 1, foreground);
        put_pixel(x + column * 2 + 1, y + row * 2 + 1, foreground);
      }
    }
  }
}

void scroll_terminal() {
  const uint32_t scroll_pixels = 18;
  for (uint32_t row = 48; row + scroll_pixels < height; row++) {
    for (uint32_t column = 0; column < framebuffer_pitch; column++) {
      framebuffer[row * framebuffer_pitch + column] =
        framebuffer[(row + scroll_pixels) * framebuffer_pitch + column];
    }
  }
  fill_rect(0, height - scroll_pixels, width, scroll_pixels, color(18, 25, 35));
  cursor_y -= scroll_pixels;
}

void put_char(char character) {
  if (character == '\n') {
    cursor_x = 24;
    cursor_y += 18;
  } else {
    draw_char(character, cursor_x, cursor_y, color(221, 229, 238));
    cursor_x += 12;
    if (cursor_x + 12 >= width) {
      cursor_x = 24;
      cursor_y += 18;
    }
  }

  if (cursor_y + 16 >= height) scroll_terminal();
}

void write(const char* text) {
  while (*text) put_char(*text++);
}

void clear_terminal() {
  fill_rect(0, 0, width, height, color(18, 25, 35));
  fill_rect(0, 0, width, 30, color(30, 43, 57));
  fill_rect(0, 30, width, 2, color(58, 184, 157));
  cursor_x = 24;
  cursor_y = 48;
}

uint8_t read_scancode() {
  uint8_t scan_code;
  do {
    asm volatile("inb $0x64, %0" : "=a"(scan_code));
  } while ((scan_code & 1) == 0);
  asm volatile("inb $0x60, %0" : "=a"(scan_code));
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
    case 0x0c: key = '-'; break;
    case 0x10: key = 'q'; break; case 0x11: key = 'w'; break;
    case 0x12: key = 'e'; break; case 0x13: key = 'r'; break;
    case 0x14: key = 't'; break; case 0x15: key = 'y'; break;
    case 0x16: key = 'u'; break; case 0x17: key = 'i'; break;
    case 0x18: key = 'o'; break; case 0x19: key = 'p'; break;
    case 0x1e: key = 'a'; break; case 0x1f: key = 's'; break;
    case 0x20: key = 'd'; break; case 0x21: key = 'f'; break;
    case 0x22: key = 'g'; break; case 0x23: key = 'h'; break;
    case 0x24: key = 'j'; break; case 0x25: key = 'k'; break;
    case 0x26: key = 'l'; break; case 0x2c: key = 'z'; break;
    case 0x2d: key = 'x'; break; case 0x2e: key = 'c'; break;
    case 0x2f: key = 'v'; break; case 0x30: key = 'b'; break;
    case 0x31: key = 'n'; break; case 0x32: key = 'm'; break;
    case 0x33: key = ','; break; case 0x34: key = '.'; break;
    case 0x35: key = '/'; break;
    case 0x39: key = ' '; break;
    default: break;
  }
  if (shifted && key >= 'a' && key <= 'z') key -= 'a' - 'A';
  if (shifted && key == '.') key = '>';
  if (shifted && key == '/') key = '?';
  if (shifted && key == '1') key = '!';
  return key;
}

bool equals(const char* left, const char* right) {
  while (*left && *right && *left == *right) {
    left++;
    right++;
  }
  return *left == *right;
}

void execute_command(const char* command) {
  if (equals(command, "help")) {
    write("Commands: help, clear, about, echo TEXT");
    put_char('\n');
  } else if (equals(command, "clear")) {
    clear_terminal();
  } else if (equals(command, "about")) {
    write("PeyoOS is a small, freestanding operating system.");
    put_char('\n');
    write("Built from scratch with no external runtime.");
    put_char('\n');
  } else if (command[0] == 'e' && command[1] == 'c' &&
             command[2] == 'h' && command[3] == 'o' && command[4] == ' ') {
    write(command + 5);
    put_char('\n');
  } else if (command[0] != 0) {
    write("Unknown command. Type help.");
    put_char('\n');
  }
}

extern "C" void kernel_main(uint32_t magic, uint64_t addr) {
  if (magic != 0x36d76289) return;

  uint32_t* current_tag = (uint32_t*)(addr + 8);
  while (*current_tag != 0) {
    struct multiboot_tag* tag = (struct multiboot_tag*)current_tag;

    if (tag->type == 8) {
      struct multiboot_tag_framebuffer* fb_tag = (struct multiboot_tag_framebuffer*)tag;
      framebuffer = (uint8_t*)fb_tag->framebuffer_addr;
      framebuffer_pitch = fb_tag->framebuffer_pitch;
      width = fb_tag->framebuffer_width;
      height = fb_tag->framebuffer_height;
      break;
    }
    current_tag += ((tag->size + 7) & ~7) / 4;
  }

  if (framebuffer == nullptr || framebuffer_pitch < width * 4 || height < 80) return;

  clear_terminal();
  write("PEYOOS  |  FREESTANDING SYSTEM CONSOLE");
  put_char('\n');
  write("Type help to see available commands.");
  put_char('\n');

  char command[128];
  uint32_t command_length = 0;
  bool shifted = false;
  write("peyoos> ");

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
      write("peyoos> ");
    } else if (scan_code == 0x0e && command_length > 0) {
      command[--command_length] = 0;
      cursor_x -= 12;
      fill_rect(cursor_x, cursor_y, 12, 18, color(18, 25, 35));
    } else if (command_length < sizeof(command) - 1 && cursor_x + 24 < width) {
      char key = key_from_scancode(scan_code, shifted);
      if (key >= 'A' && key <= 'Z') key += 'a' - 'A';
      if (key != 0) {
        command[command_length++] = key;
        put_char(key);
      }
    }
  }
}
