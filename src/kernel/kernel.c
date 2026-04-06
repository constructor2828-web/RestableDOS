#include "boot_info.h"
#include "terminal.h"
#include "rofs.h"
#include "shell.h"
#include "usb.h"
#include "multiboot2.h"
#include "gui/gui.h"

static void print_banner(void);

/* ── Kernel entry ─────────────────────────────────────────────────────────── */
void kernel_main(uint64_t multiboot_addr) {
    boot_info_t binfo = {0};
    struct multiboot_tag *tag;

    // First tag starts at multiboot_addr + 8 (skipping size/reserved)
    for (tag = (struct multiboot_tag *)(multiboot_addr + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7))) 
    {
        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            struct multiboot_tag_framebuffer *fb = (struct multiboot_tag_framebuffer *)tag;
            binfo.framebuffer_base = fb->addr;
            binfo.framebuffer_size = fb->pitch * fb->height;
            binfo.screen_width = fb->width;
            binfo.screen_height = fb->height;
            binfo.pixels_per_scanline = fb->pitch / 4;
        } else if (tag->type == MULTIBOOT_TAG_TYPE_MODULE) {
            struct multiboot_tag_module *mod = (struct multiboot_tag_module *)tag;
            // The first module is assumed to be rofs.bin
            if (binfo.rofs_base == 0) {
                binfo.rofs_base = mod->mod_start;
                binfo.rofs_size = mod->mod_end - mod->mod_start;
            }
        }
    }

    terminal_init(&binfo);
    print_banner();

    debug_separator();
    debug_info("Kernel starting via GRUB/Multiboot2...");
    debug_hex("Multiboot Info Address", (uint32_t)multiboot_addr);
    debug_hex("Framebuffer Address", (uint32_t)binfo.framebuffer_base);
    debug_hex("ROFS Base Address", (uint32_t)binfo.rofs_base);
    debug_separator();

    /* Init subsystems */
    terminal_writeln("");
    rofs_init(binfo.rofs_base);
    usb_init();
    shell_init();

    /* Print MOTD */
    terminal_writeln("");
    debug_separator();
    {
        uint8_t motd[256];
        int r = rofs_read_file("motd", motd, sizeof(motd)-1);
        if (r > 0) { motd[r] = 0; terminal_writeln((char*)motd); }
    }
    debug_separator();

    debug_info("Switching to Graphical Mode in 5 seconds...");
    for(volatile int i = 0; i < 500000000; i++); // Very crude delay

    /* Hand off to GUI */
    gui_init(&binfo);
    gui_run();

    /* Should never reach here */
    __asm__ volatile("cli; hlt");
    while(1);
}

/* ── Banner ──────────────────────────────────────────────────────────────── */
static void print_banner(void) {
    terminal_setcolor(VGA_LIGHT_CYAN, VGA_BLACK);
    terminal_writeln("");
    terminal_writeln("  ██╗  ██╗███████╗██████╗ ███╗  ██╗███████╗██╗     ██╗███████╗████████╗ ██████╗ ███████╗");
    terminal_writeln("  ██║ ██╔╝██╔════╝██╔══██╗████╗ ██║██╔════╝██║     ██║██╔════╝╚══██╔══╝██╔═══██╗██╔════╝");
    terminal_writeln("  █████╔╝ █████╗  ██████╔╝██╔██╗██║█████╗  ██║     ██║███████╗   ██║   ██║   ██║███████╗");
    terminal_writeln("  ██╔═██╗ ██╔══╝  ██╔══██╗██║╚████║██╔══╝  ██║     ██║╚════██║   ██║   ██║   ██║╚════██║");
    terminal_writeln("  ██║  ██╗███████╗██║  ██║██║ ╚███║███████╗███████╗██║███████║   ██║   ╚██████╔╝███████║");
    terminal_writeln("  ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚══╝╚══════╝╚══════╝╚═╝╚══════╝   ╚═╝    ╚═════╝ ╚══════╝");
    terminal_setcolor(VGA_DARK_GREY, VGA_BLACK);
    terminal_writeln("                             RestableDOS v1.0 — Premium Edition");
    terminal_setcolor(VGA_LIGHT_GREY, VGA_BLACK);
    terminal_writeln("");
}