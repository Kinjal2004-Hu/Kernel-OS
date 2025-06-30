/* =========================================================
 *  kernel.c  – MyOS 1.2   (Multiboot + simple FAT-12 shell)
 *  Updated: case-insensitive commands & FAT12 lookups
 * ========================================================= */
#include <stdint.h>
#include <string.h>  // Include string.h for strcmpi

#define VIDEO        0xB8000
#define SW           80
#define SH           25
#define CLR_TXT      0x0F
#define CLR_OK       0x0A
#define CLR_ERR      0x0C

static int cx = 0, cy = 0;
static char inbuf[256];
static int  inlen = 0;

static char *fat = 0;

static inline unsigned char inb(unsigned short p) {
    unsigned char v;
    __asm__ __volatile__("inb %1,%0":"=a"(v):"dN"(p));
    return v;
}

/* ──────── string helpers ──────── */
static int strcmpi(const char*a, const char*b) {
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return (unsigned char)ca - (unsigned char)cb;
        ++a; ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

static int strncmpi(const char*a, const char*b, int n) {
    for (int i = 0; i < n && a[i] && b[i]; ++i) {
        char ca = a[i], cb = b[i];
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return ca - cb;
    }
    return 0;
}

/* ──────── VGA text output ──────── */
static void scroll(void) {
    if (cy < SH) return;
    volatile char *v = (char*)VIDEO;
    for (int i = 0; i < (SH-1)*SW; ++i) {
        v[i*2] = v[(i+SW)*2];
        v[i*2+1] = v[(i+SW)*2+1];
    }
    for (int i = (SH-1)*SW; i < SH*SW; ++i) {
        v[i*2] = ' ';
        v[i*2+1] = CLR_TXT;
    }
    cy = SH-1;
}

static void putc(char c, unsigned char col) {
    volatile char *v = (char*)VIDEO;
    if (c == '\n') { cx = 0; ++cy; scroll(); return; }
    if (c == '\b') { if (cx) { --cx; int p = cy*SW+cx; v[p*2] = ' '; } return; }
    int p = cy*SW+cx;
    v[p*2] = c;
    v[p*2+1] = col;
    if (++cx >= SW) { cx = 0; ++cy; scroll(); }
}

static void puts(const char *s, unsigned char col) {
    while (*s) putc(*s++, col);
}

static void cls(void) {
    volatile char *v = (char*)VIDEO;
    for (int i = 0; i < SW*SH; ++i) {
        v[i*2] = ' ';
        v[i*2+1] = CLR_TXT;
    }
    cx = cy = 0;
}

/* ──────── Keyboard map ──────── */
static char sc2a(unsigned char s) {
    static char t[] = {0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
    return (s < sizeof(t)) ? t[s] : 0;
}

/* ──────── Multiboot structures ──────── */
typedef struct { uint32_t s,e,str,res; } __attribute__((packed)) mb_mod_t;
typedef struct { uint32_t f,ml,mh,bd,cl,mc,ma; } __attribute__((packed)) mbi_t;

/* ──────── FAT-12 BPB cache ──────── */
typedef struct {
    uint16_t bps;
    uint8_t  spc;
    uint16_t res;
    uint8_t  nf;
    uint16_t nr;
    uint16_t spf;
    uint32_t root_off, data_off;
} fat12_t;

static fat12_t fi;

static void fat_init(char *img) {
    fi.bps = *(uint16_t*)(img+11);
    fi.spc = *(uint8_t *)(img+13);
    fi.res = *(uint16_t*)(img+14);
    fi.nf  = *(uint8_t *)(img+16);
    fi.nr  = *(uint16_t*)(img+17);
    fi.spf = *(uint16_t*)(img+22);
    fi.root_off = (fi.res + fi.nf * fi.spf) * fi.bps;
    fi.data_off = fi.root_off + fi.nr * 32;
}

static uint16_t fat_next(char *img, uint16_t c) {
    uint32_t o = fi.res*fi.bps + c + (c>>1);
    uint16_t v = *(uint16_t*)(img + o);
    return (c & 1) ? (v >> 4) : (v & 0x0FFF);
}

static char* fat_find(char *img, const char *name) {
    char *root = img + fi.root_off;
    for (int i = 0; i < fi.nr; ++i) {
        char *e = root + i*32;
        if (e[0] == 0x00) break;
        if ((uint8_t)e[0] == 0xE5) continue;
        char n[13] = {0};
        int p = 0;
        for (int j = 0; j < 8 && e[j] != ' '; ++j) n[p++] = e[j];
        if (e[8] != ' ') {
            n[p++] = '.';
            for (int j = 0; j < 3 && e[8+j] != ' '; ++j) n[p++] = e[8+j];
        }
        if (strcmpi(n, name) == 0) return e;
    }
    return 0;
}

static void fat_ls(void) {
    char *root = fat + fi.root_off;
    puts("Root directory:\n", CLR_OK);
    for (int i = 0; i < fi.nr; ++i) {
        char *e = root + i*32;
        if (e[0] == 0x00) break;
        if ((uint8_t)e[0] == 0xE5) continue;
        char n[13] = {0};
        int p = 0;
        for (int j = 0; j < 8 && e[j] != ' '; ++j) n[p++] = e[j];
        if (e[8] != ' ') {
            n[p++] = '.';
            for (int j = 0; j < 3 && e[8+j] != ' '; ++j) n[p++] = e[8+j];
        }
        puts("  ", CLR_TXT);
        puts(n, CLR_TXT);
        putc('\n', CLR_TXT);
    }
}

static void fat_cat(const char *name) {
    char *e = fat_find(fat, name);
    if (!e) { puts("File not found.\n", CLR_ERR); return; }
    uint16_t c = *(uint16_t*)(e+26);
    uint32_t sz = *(uint32_t*)(e+28);
    while (c < 0xFF8) {
        uint32_t off = fi.data_off + (c-2)*fi.spc*fi.bps;
        char *data = fat + off;
        for (uint32_t i = 0; i < fi.spc*fi.bps && sz; i++, --sz) {
            if (data[i] == '\r') continue;
            putc(data[i], CLR_TXT);
        }
        c = fat_next(fat, c);
    }
    putc('\n', CLR_TXT);
}

static void app_calc(void) {
    char buf[32];
    int len = 0;
    int a, b;
    char op;

    puts("Calculator App\n", CLR_OK);

    // Get first number
    puts("Enter first number: ", CLR_TXT);
    len = 0;
    while (1) {
        if (inb(0x64) & 1) {
            unsigned char sc = inb(0x60);
            if (!(sc & 0x80)) {
                char ch = sc2a(sc);
                if (ch == '\n') { putc('\n', CLR_TXT); break; }
                if (ch == '\b') {
                    if (len) { --len; putc('\b', CLR_TXT); }
                } else if (ch >= '0' && ch <= '9' && len < 31) {
                    buf[len++] = ch;
                    putc(ch, CLR_TXT);
                }
            }
        }
    }
    buf[len] = 0;
    a = 0;
    for (int i = 0; i < len; i++) a = a * 10 + (buf[i] - '0');

    // Get operator
    puts("Enter operator (+ - * /): ", CLR_TXT);
    while (1) {
        if (inb(0x64) & 1) {
            unsigned char sc = inb(0x60);
            if (!(sc & 0x80)) {
                char ch = sc2a(sc);
                if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
                    op = ch;
                    putc(ch, CLR_TXT);
                    putc('\n', CLR_TXT);
                    break;
                }
            }
        }
    }

    // Get second number
    puts("Enter second number: ", CLR_TXT);
    len = 0;
    while (1) {
        if (inb(0x64) & 1) {
            unsigned char sc = inb(0x60);
            if (!(sc & 0x80)) {
                char ch = sc2a(sc);
                if (ch == '\n') { putc('\n', CLR_TXT); break; }
                if (ch == '\b') {
                    if (len) { --len; putc('\b', CLR_TXT); }
                } else if (ch >= '0' && ch <= '9' && len < 31) {
                    buf[len++] = ch;
                    putc(ch, CLR_TXT);
                }
            }
        }
    }
    buf[len] = 0;
    b = 0;
    for (int i = 0; i < len; i++) b = b * 10 + (buf[i] - '0');

    // Calculate result
    int res = 0;
    int divzero = 0;

    if (op == '+') res = a + b;
    else if (op == '-') res = a - b;
    else if (op == '*') res = a * b;
    else if (op == '/') {
        if (b == 0) divzero = 1;
        else res = a / b;
    }

    // Display result
    if (divzero) {
        puts("Error: Division by zero\n", CLR_ERR);
    } else {
        puts("Result: ", CLR_OK);
        int tmp = res;
        char rbuf[32];
        int rlen = 0;
        if (tmp == 0) rbuf[rlen++] = '0';
        else {
            if (tmp < 0) { putc('-', CLR_TXT); tmp = -tmp; }
            while (tmp > 0) {
                rbuf[rlen++] = '0' + (tmp % 10);
                tmp /= 10;
            }
        }
        for (int i = rlen - 1; i >= 0; i--) putc(rbuf[i], CLR_TXT);
        putc('\n', CLR_TXT);
    }
}

/* ──────── shell command dispatcher ──────── */
static void cmd(const char *s) {
    if (strcmpi(s, "help") == 0) {
        puts("help clear about ls cat <N> run calc reboot\n", CLR_OK);
    } else if (strcmpi(s, "clear") == 0) {
        cls(); puts("MyOS> ", CLR_OK);
    } else if (strcmpi(s, "about") == 0) {
        puts("MyOS 1.2 – FAT-12, case-insensitive shell\n", CLR_OK);
    } else if (strcmpi(s, "ls") == 0) {
        fat ? fat_ls() : puts("No disk.\n", CLR_ERR);
    } else if (strncmpi(s, "cat ", 4) == 0) {
        fat ? fat_cat(s+4) : puts("No disk.\n", CLR_ERR);
    } else if (strcmpi(s, "run calc") == 0) {
        app_calc();
    } else if (strcmpi(s, "reboot") == 0) {
        __asm__ __volatile__("cli; hlt");
    } else if (*s) {
        puts("Unknown command. Type help.\n", CLR_ERR);
    }
}

/* ──────── KERNEL ENTRY ──────── */
void kmain(mbi_t *mb) {
    cls(); puts("Welcome to MyOS!\n", CLR_OK);
    if (mb && (mb->f & (1<<3)) && mb->mc) {
        mb_mod_t *m = (mb_mod_t*)mb->ma;
        fat = (char*)m->s;
        fat_init(fat);
        puts("FAT-12 module loaded.\n", CLR_TXT);
    } else puts("No disk image.\n", CLR_ERR);

    puts("\nMyOS> ", CLR_OK);
    while (1) {
        if (inb(0x64) & 1) {
            unsigned char sc = inb(0x60);
            if (!(sc & 0x80)) {
                char ch = sc2a(sc);
                if (ch == '\n') {
                    putc('\n', CLR_TXT);
                    inbuf[inlen] = 0;
                    cmd(inbuf);
                    inlen = 0;
                    puts("MyOS> ", CLR_OK);
                } else if (ch == '\b') {
                    if (inlen) { --inlen; putc('\b', CLR_TXT); }
                } else if (ch && inlen < 255) {
                    inbuf[inlen++] = ch;
                    putc(ch, CLR_TXT);
                }
            }
        }
    }
}
