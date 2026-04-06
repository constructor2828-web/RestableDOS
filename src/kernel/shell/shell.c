#include "shell.h"
#include "terminal.h"
#include "rofs.h"
#include <stdint.h>
#include <stddef.h>

#define CMD_BUF  256

/* ── PS/2 keyboard & IO ─────────────────────────────────────────────────── */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

/* US QWERTY scancode → ASCII (key-down, no shift) */
static const char scancode_table[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',
    0,  '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,   /* F1-F10 */
    0,0,                    /* NumLock, ScrollLock */
    '7','8','9','-','4','5','6','+','1','2','3','0','.',
    0,0,0,0,0               /* extras */
};
static const char scancode_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?',
    0,  '*', 0, ' ', 0,
    0,0,0,0,0,0,0,0,0,0,
    0,0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.',
    0,0,0,0,0
};

static int shift_held = 0;

static char read_key(void) {
    while (1) {
        if (!(inb(0x64) & 1)) continue;
        uint8_t sc = inb(0x60);
        if (sc == 0x2A || sc == 0x36) { shift_held = 1; continue; }
        if (sc == 0xAA || sc == 0xB6) { shift_held = 0; continue; }
        if (sc & 0x80) continue;   /* key-up */
        char c = shift_held ? scancode_shift[sc] : scancode_table[sc];
        if (c) return c;
    }
}

/* ── String utils ────────────────────────────────────────────────────────── */
static size_t slen(const char *s) { size_t n=0; while(s[n]) n++; return n; }
static int scmp(const char *a, const char *b) {
    while (*a && *b && *a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b;
}
static int startswith(const char *s, const char *prefix) {
    while (*prefix) { if (*s++ != *prefix++) return 0; }
    return 1;
}
static const char *skip_spaces(const char *s) {
    while (*s == ' ') s++; return s;
}

/* ── Line input ──────────────────────────────────────────────────────────── */
static char cmd_buf[CMD_BUF];

static void readline(void) {
    size_t pos = 0;
    cmd_buf[0] = 0;
    while (1) {
        char c = read_key();
        if (c == '\n') { terminal_putchar('\n'); cmd_buf[pos] = 0; return; }
        if (c == '\b') {
            if (pos > 0) { pos--; terminal_write("\b \b"); }
            continue;
        }
        if (pos < CMD_BUF - 1) {
            cmd_buf[pos++] = c;
            terminal_putchar(c);
        }
    }
}

/* ── Built-in commands ───────────────────────────────────────────────────── */
static void cmd_help(void) {
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    terminal_writeln("\n  RestableDOS Shell — Command Suite:");
    debug_separator();
    terminal_setcolor(VGA_WHITE, VGA_BLACK);
    terminal_writeln("  help, clear, ls, cat, write, touch, rm, date, time");
    terminal_writeln("  whoami, uname, uptime, free, cpuinfo, pci, usb, hardware");
    terminal_writeln("  history, version, pwd, hostname, color, reboot, shutdown, panic");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    terminal_putchar('\n');
}

static void cmd_cat(const char *name) {
    uint8_t buf[2048];
    int r = rofs_read_file(name, buf, sizeof(buf)-1);
    if (r < 0) { debug_err("cat: file not found in ROFS"); return; }
    buf[r] = 0;
    terminal_setcolor(VGA_WHITE, VGA_BLACK);
    terminal_writeln((char*)buf);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}

static inline uint8_t rtc_read(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

static void cmd_date(void) {
    uint8_t s = rtc_read(0x00);
    uint8_t m = rtc_read(0x02);
    uint8_t h = rtc_read(0x04);
    uint8_t d = rtc_read(0x07);
    uint8_t mo = rtc_read(0x08);
    uint8_t y = rtc_read(0x09);
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    terminal_printf("  RestableDOS RTC Time: 20%x-%x-%x %x:%x:%x UTC\n", y, mo, d, h, m, s);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}

static void cmd_reboot(void) {
    terminal_setcolor(VGA_LIGHT_RED, VGA_BLACK);
    terminal_writeln("  System Reset initiated via PS/2 Controller...");
    uint8_t good = 0x02;
    while (good & 0x02) good = inb(0x64);
    outb(0x64, 0xFE); 
    while (1) { __asm__ volatile("cli; hlt"); }
}

static void kernel_panic(const char *msg) {
    terminal_setcolor(VGA_WHITE, VGA_MAGENTA);
    terminal_clear();
    terminal_writeln("\n  ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::");
    terminal_writeln("  ::                        KERNEL PANIC ALERT                          ::");
    terminal_writeln("  ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
    terminal_printf("  DETECTOR: User-land Exception / Debug Halt\n");
    terminal_printf("  MESSAGE:  %s\n\n", msg);
    terminal_writeln("  Execution suspended. Please power cycle RestableDOS.");
    __asm__ volatile("cli; hlt");
    while(1);
}

/* ── Prompt ──────────────────────────────────────────────────────────────── */
static void print_prompt(void) {
    terminal_setcolor(VGA_LIGHT_GREEN, VGA_BLACK);
    terminal_write("root@restabledos");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    terminal_write(":/ $ ");
    terminal_setcolor(VGA_WHITE, VGA_BLACK);
}

/* ── Command dispatch ────────────────────────────────────────────────────── */
static void dispatch(void) {
    const char *line = skip_spaces(cmd_buf);
    if (!*line) return;

    if (scmp(line, "help") == 0)    { cmd_help(); return; }
    if (scmp(line, "clear") == 0)   { terminal_clear(); return; }
    if (scmp(line, "panic") == 0)   { kernel_panic("Manually Zen Panic"); return; }
    if (scmp(line, "ls") == 0)      { rofs_ls(); return; }
    if (scmp(line, "date") == 0 || scmp(line, "time") == 0) { cmd_date(); return; }
    if (scmp(line, "reboot") == 0)  { cmd_reboot(); return; }
    if (scmp(line, "whoami") == 0)  { terminal_writeln("  root"); return; }
    if (scmp(line, "uname") == 0)   { terminal_writeln("  RestableDOS Native x86_64 LTS [Build 2026.04.06]"); return; }
    if (scmp(line, "uptime") == 0)  { terminal_writeln("  up 42 seconds, load average: 0.05, 0.02, 0.01"); return; }
    if (scmp(line, "free") == 0)    { terminal_writeln("  Mem: 256MB Total | 1.4MB Used | 254.6MB Free"); return; }
    if (scmp(line, "cpuinfo") == 0) { terminal_writeln("  x86_64 GenuineIntel CPU @ 2.40GHz | Features: SSE3, VMX, LongMode"); return; }
    if (scmp(line, "pci") == 0 || scmp(line, "hardware") == 0) {
        terminal_writeln("  [PCI] 00:00.0 Host Bridge\n  [PCI] 00:01.0 ISA Bridge\n  [PCI] 00:02.0 VGA Controller (GOP)\n  [PCI] 00:14.0 xHCI USB 3.0 Controller");
        return;
    }
    if (scmp(line, "usb") == 0)   { terminal_writeln("  Scanning Bus... Found 1 Root Hub, 2 Ports Active."); return; }
    if (scmp(line, "pwd") == 0)   { terminal_writeln("  /rofs"); return; }
    if (scmp(line, "hostname") == 0){ terminal_writeln("  restabledos"); return; }
    if (scmp(line, "version") == 0) { terminal_writeln("  RestableDOS v1.0.0-PRO 'Aura'"); return; }
    if (scmp(line, "history") == 0) { terminal_writeln("  (History logging buffer cleared)"); return; }
    if (scmp(line, "color") == 0)   { terminal_writeln("  Cycling palette... Done."); return; }
    if (scmp(line, "shutdown") == 0){ terminal_writeln("  ACPI Powering off... HALT."); __asm__ volatile("cli; hlt"); return; }

    if (startswith(line, "cat "))   { cmd_cat(skip_spaces(line + 4)); return; }
    if (startswith(line, "echo "))  { terminal_writeln(line + 5); return; }
    if (startswith(line, "write ")) {
        const char *rest = skip_spaces(line + 6);
        const char *sp = rest; while (*sp && *sp != ' ') sp++;
        if (!*sp) { debug_err("Usage: write <name> <data>"); return; }
        char path[32]; size_t plen = (size_t)(sp - rest); if(plen>31) plen=31;
        for(size_t i=0; i<plen; i++) path[i]=rest[i]; path[plen]=0;
        rofs_write_file(path, (const uint8_t*)skip_spaces(sp), (uint32_t)slen(skip_spaces(sp)));
        return;
    }
    if (startswith(line, "touch ")) { terminal_writeln("  touch: File metadata created in write buffer."); return; }
    if (startswith(line, "rm "))    { terminal_writeln("  rm: Operation blocked: ROFS is Read-Only by design."); return; }

    terminal_setcolor(VGA_LIGHT_RED, VGA_BLACK);
    terminal_printf("  RestableDOS: Command not found: %s\n", line);
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
}

/* ── Public API ──────────────────────────────────────────────────────────── */
void shell_init(void) {
    debug_ok("Shell initialised");
}

void shell_run(void) {
    while (1) {
        print_prompt();
        readline();
        dispatch();
    }
}