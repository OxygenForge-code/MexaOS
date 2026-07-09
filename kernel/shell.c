/* ============================================================
 *  MexaOS Shell (MexaShell)
 *  Intent-aware command line interface
 * ============================================================ */

#include "../include/mexaos.h"
#include "include/kernlib.h"
#include "include/vga.h"
#include "include/memory.h"
#include "include/keyboard.h"
#include "include/timer.h"
#include "include/process.h"
#include "include/mexfs.h"
#include "include/ai_engine.h"
#include "include/shell.h"

/* ─── Forward Declarations ─── */
static void shell_read_line(void);
static int shell_tokenize(char *line, char **argv, int max_argv);
static void shell_welcome(void);
static void shell_print_error(const char *msg);
static void shell_print_success(const char *msg);

/* ─── Shell State ─── */
static struct shell_state shell;
static struct shell_command commands[];

/* ─── History Ring Buffer ─── */
static char history[SHELL_HISTORY_SIZE][SHELL_INPUT_SIZE];
static int history_count = 0;
static int history_pos = -1;

/* ─── Built-in Command Table ─── */
static struct shell_command commands[] = {
    {"help",      "Show available commands",           "help [command]",          cmd_help,      1},
    {"clear",     "Clear the screen",                  "clear",                   cmd_clear,     0},
    {"ls",        "List directory contents",           "ls [path]",               cmd_ls,        1},
    {"cd",        "Change directory",                  "cd <path>",               cmd_cd,        1},
    {"pwd",       "Print working directory",           "pwd",                     cmd_pwd,       0},
    {"cat",       "Display file contents",             "cat <file>",              cmd_cat,       1},
    {"touch",     "Create an empty file",              "touch <file>",            cmd_touch,     1},
    {"rm",        "Remove a file or directory",        "rm <path>",               cmd_rm,        1},
    {"mkdir",     "Create a directory",                "mkdir <path>",            cmd_mkdir,     1},
    {"ps",        "List running processes",            "ps",                      cmd_ps,        0},
    {"kill",      "Terminate a process",               "kill <pid>",              cmd_kill,      0},
    {"mem",       "Show memory usage",                 "mem",                     cmd_mem,       0},
    {"df",        "Show disk usage",                   "df",                      cmd_df,        0},
    {"uptime",    "Show system uptime",                "uptime",                  cmd_uptime,    0},
    {"echo",      "Print a message",                   "echo <message>",          cmd_echo,      0},
    {"intent",    "Execute a natural language intent", "intent <description>",    cmd_intent,    1},
    {"ai",        "Toggle AI mode",                    "ai [on|off]",             cmd_ai,        1},
    {"workspace", "Manage AI workspaces",              "workspace <name>",        cmd_workspace, 1},
    {"motd",      "Show message of the day",           "motd",                    cmd_motd,      0},
    {"reboot",    "Reboot the system",                 "reboot",                  cmd_reboot,    0},
    {"halt",      "Halt the system",                   "halt",                    cmd_halt,      0},
    {"sysinfo",   "Show system information",           "sysinfo",                   cmd_sysinfo,   0},
    {"theme",     "Change color theme",                "theme <name>",            cmd_theme,     0},
    {"mex",       ".mex file operations",              "mex <file> [info|exec]",  cmd_mex,       1},
    {NULL, NULL, NULL, NULL, 0}
};

/* ============================================================
 *  Shell Core
 * ============================================================ */

void shell_init(void) {
    memset(&shell, 0, sizeof(shell));
    shell.running = 1;
    shell.echo = 1;
    shell.last_exit_code = 0;
    strcpy(shell.cwd, "/");
    
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    shell_welcome();
}

void shell_run(void) {
    while (shell.running) {
        shell_prompt();
        shell_read_line();
    }
}

void shell_prompt(void) {
    if (shell.ai_mode) {
        vga_setcolor(VGA_COLOR_LMAGENTA, VGA_COLOR_BLACK);
        vga_puts(SHELL_PROMPT_AI);
    } else {
        vga_setcolor(VGA_COLOR_LBLUE, VGA_COLOR_BLACK);
        vga_puts(SHELL_PROMPT);
    }
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

static void shell_read_line(void) {
    char c;
    int pos = 0;
    
    memset(shell.input, 0, SHELL_INPUT_SIZE);
    
    while (1) {
        c = keyboard_getchar();
        
        if (c == '\n' || c == '\r') {
            vga_putc('\n');
            shell.input[pos] = '\0';
            if (pos > 0) {
                shell_history_add(shell.input);
                shell_process_line(shell.input);
            }
            return;
        } else if (c == '\b' || c == 0x7F) {
            if (pos > 0) {
                pos--;
                shell.input[pos] = '\0';
                vga_putc('\b');
                vga_putc(' ');
                vga_putc('\b');
            }
        } else if (c == '\t') {
            shell_tab_complete(shell.input, &pos);
        } else if (c == 0x11) {
            /* Up arrow - history prev */
            const char *prev = shell_history_prev();
            if (prev) {
                while (pos > 0) { vga_putc('\b'); vga_putc(' '); vga_putc('\b'); pos--; }
                strcpy(shell.input, prev);
                pos = strlen(shell.input);
                vga_puts(shell.input);
            }
        } else if (c == 0x12) {
            /* Down arrow - history next */
            const char *next = shell_history_next();
            if (next) {
                while (pos > 0) { vga_putc('\b'); vga_putc(' '); vga_putc('\b'); pos--; }
                strcpy(shell.input, next);
                pos = strlen(shell.input);
                vga_puts(shell.input);
            } else {
                while (pos > 0) { vga_putc('\b'); vga_putc(' '); vga_putc('\b'); pos--; }
                shell.input[0] = '\0';
                pos = 0;
            }
        } else if (c >= 0x20 && c < 0x7F && pos < SHELL_INPUT_SIZE - 1) {
            shell.input[pos++] = c;
            if (shell.echo) vga_putc(c);
        }
    }
}

void shell_process_line(const char *line) {
    char buf[SHELL_INPUT_SIZE];
    char *argv[SHELL_MAX_ARGS];
    int argc;
    
    strncpy(buf, line, SHELL_INPUT_SIZE - 1);
    buf[SHELL_INPUT_SIZE - 1] = '\0';
    
    argc = shell_tokenize(buf, argv, SHELL_MAX_ARGS);
    if (argc == 0) return;
    
    shell.commands_executed++;
    int ret = shell_execute(argc, argv);
    shell.last_exit_code = ret;
    
    if (ret != 0) {
        vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
        vga_puts("  [exit code ");
        vga_print_dec(ret);
        vga_puts("]\n");
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
}

static int shell_tokenize(char *line, char **argv, int max_argv) {
    int argc = 0;
    char *p = line;
    
    while (*p && argc < max_argv) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        
        argv[argc++] = p;
        
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
    }
    
    return argc;
}

int shell_execute(int argc, char **argv) {
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            return commands[i].handler(argc, argv);
        }
    }
    
    /* AI intent fallback */
    if (shell.ai_mode) {
        return cmd_intent(argc, argv);
    }
    
    vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
    vga_puts("mexa: command not found: ");
    vga_puts(argv[0]);
    vga_puts("\n  Type 'help' for available commands.\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    return 1;
}

/* ============================================================
 *  Command Handlers
 * ============================================================ */

int cmd_help(int argc, char **argv) {
    if (argc > 1) {
        for (int i = 0; commands[i].name != NULL; i++) {
            if (strcmp(argv[1], commands[i].name) == 0) {
                vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
                vga_puts("\n  Command: ");
                vga_puts(commands[i].name);
                vga_puts("\n");
                vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                vga_puts("  Description: ");
                vga_puts(commands[i].description);
                vga_puts("\n  Usage: ");
                vga_setcolor(VGA_COLOR_LGRAY, VGA_COLOR_BLACK);
                vga_puts(commands[i].usage);
                vga_puts("\n\n");
                vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                return 0;
            }
        }
        vga_puts("  Unknown command: ");
        vga_puts(argv[1]);
        vga_puts("\n");
        return 1;
    }
    
    vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
    vga_puts("\n  MexaShell Commands:\n");
    vga_puts("  ═════════════════════════════════════════════════════\n\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    for (int i = 0; commands[i].name != NULL; i++) {
        vga_setcolor(VGA_COLOR_LBLUE, VGA_COLOR_BLACK);
        vga_puts("  ");
        vga_puts(commands[i].name);
        int pad = 12 - strlen(commands[i].name);
        for (int j = 0; j < pad; j++) vga_putc(' ');
        vga_setcolor(VGA_COLOR_LGRAY, VGA_COLOR_BLACK);
        vga_puts(commands[i].description);
        if (commands[i].ai_enabled) {
            vga_setcolor(VGA_COLOR_LMAGENTA, VGA_COLOR_BLACK);
            vga_puts("  [AI]");
        }
        vga_puts("\n");
    }
    
    vga_setcolor(VGA_COLOR_DIM, VGA_COLOR_BLACK);
    vga_puts("\n  Commands tagged with [AI] can be invoked via natural language intent.\n");
    vga_puts("  Try: intent \"open a workspace for coding\"\n\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    return 0;
}

int cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_clear();
    return 0;
}

int cmd_ls(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : shell.cwd;
    
    vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
    vga_puts("\n  Contents of ");
    vga_puts(path);
    vga_puts(":\n");
    vga_puts("  ═══════════════════════════════════════\n\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Enumerate files from MexFS */
    struct mexfs_dir *dir = mexfs_opendir(path);
    if (!dir) {
        vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
        vga_puts("  Cannot access directory: ");
        vga_puts(path);
        vga_puts("\n");
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return 1;
    }
    
    struct mexfs_dirent *entry;
    while ((entry = mexfs_readdir(dir)) != NULL) {
        if (entry->type == MEX_TYPE_DIRECTORY) {
            vga_setcolor(VGA_COLOR_LBLUE, VGA_COLOR_BLACK);
            vga_puts("  [DIR]  ");
        } else if (entry->type == MEX_TYPE_AI_MODEL) {
            vga_setcolor(VGA_COLOR_LMAGENTA, VGA_COLOR_BLACK);
            vga_puts("  [AI]   ");
        } else if (entry->type == MEX_TYPE_INTENT) {
            vga_setcolor(VGA_COLOR_LGREEN, VGA_COLOR_BLACK);
            vga_puts("  [INT]  ");
        } else {
            vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            vga_puts("  [FILE] ");
        }
        vga_puts(entry->name);
        vga_puts("\n");
    }
    mexfs_closedir(dir);
    
    vga_puts("\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    return 0;
}

int cmd_cd(int argc, char **argv) {
    if (argc < 2) {
        strcpy(shell.cwd, "/");
        return 0;
    }
    
    const char *path = argv[1];
    if (mexfs_isdir(path)) {
        strncpy(shell.cwd, path, MEX_MAX_PATH - 1);
        shell.cwd[MEX_MAX_PATH - 1] = '\0';
    } else {
        vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
        vga_puts("  Not a directory: ");
        vga_puts(path);
        vga_puts("\n");
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return 1;
    }
    return 0;
}

int cmd_pwd(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_puts(" ");
    vga_puts(shell.cwd);
    vga_puts("\n");
    return 0;
}

int cmd_cat(int argc, char **argv) {
    if (argc < 2) {
        vga_puts("  Usage: cat <file>\n");
        return 1;
    }
    
    struct mexfs_file *fp = mexfs_fopen(argv[1], MEX_O_READ);
    if (!fp) {
        vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
        vga_puts("  Cannot open file: ");
        vga_puts(argv[1]);
        vga_puts("\n");
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return 1;
    }
    
    vga_puts("\n");
    char buf[512];
    size_t n;
    while ((n = mexfs_fread(buf, 1, sizeof(buf) - 1, fp)) > 0) {
        buf[n] = '\0';
        vga_puts(buf);
    }
    mexfs_fclose(fp);
    vga_puts("\n");
    return 0;
}

int cmd_touch(int argc, char **argv) {
    if (argc < 2) {
        vga_puts("  Usage: touch <file>\n");
        return 1;
    }
    
    struct mexfs_file *fp = mexfs_fopen(argv[1], MEX_O_CREATE | MEX_O_WRITE);
    if (!fp) {
        vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
        vga_puts("  Cannot create file: ");
        vga_puts(argv[1]);
        vga_puts("\n");
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return 1;
    }
    mexfs_fclose(fp);
    shell_print_success("File created.");
    return 0;
}

int cmd_rm(int argc, char **argv) {
    if (argc < 2) {
        vga_puts("  Usage: rm <path>\n");
        return 1;
    }
    
    if (mexfs_unlink(argv[1]) == 0) {
        shell_print_success("Removed.");
        return 0;
    } else {
        shell_print_error("Cannot remove file.");
        return 1;
    }
}

int cmd_mkdir(int argc, char **argv) {
    if (argc < 2) {
        vga_puts("  Usage: mkdir <path>\n");
        return 1;
    }
    
    if (mexfs_mkdir(argv[1]) == 0) {
        shell_print_success("Directory created.");
        return 0;
    } else {
        shell_print_error("Cannot create directory.");
        return 1;
    }
}

int cmd_ps(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
    vga_puts("\n  PID  Name                           State\n");
    vga_puts("  ══════════════════════════════════════════════════\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    struct process_info procs[MAX_PROCESSES];
    int count = process_list(procs, MAX_PROCESSES);
    
    for (int i = 0; i < count; i++) {
        vga_puts("  ");
        vga_print_dec(procs[i].pid);
        int pad = 4 - (procs[i].pid > 999 ? 4 : procs[i].pid > 99 ? 3 : procs[i].pid > 9 ? 2 : 1);
        for (int j = 0; j < pad; j++) vga_putc(' ');
        vga_puts(" ");
        vga_puts(procs[i].name);
        pad = 30 - strlen(procs[i].name);
        for (int j = 0; j < pad; j++) vga_putc(' ');
        
        switch (procs[i].state) {
            case PROC_RUNNING:  vga_setcolor(VGA_COLOR_LGREEN, VGA_COLOR_BLACK);  vga_puts("RUNNING");   break;
            case PROC_READY:    vga_setcolor(VGA_COLOR_LYELLOW, VGA_COLOR_BLACK); vga_puts("READY");     break;
            case PROC_BLOCKED:  vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);    vga_puts("BLOCKED");   break;
            case PROC_SLEEPING: vga_setcolor(VGA_COLOR_LBLUE, VGA_COLOR_BLACK);   vga_puts("SLEEPING");  break;
            default:            vga_setcolor(VGA_COLOR_DIM, VGA_COLOR_BLACK);     vga_puts("UNKNOWN");   break;
        }
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_puts("\n");
    }
    vga_puts("\n");
    return 0;
}

int cmd_kill(int argc, char **argv) {
    if (argc < 2) {
        vga_puts("  Usage: kill <pid>\n");
        return 1;
    }
    
    pid_t pid = atoi(argv[1]);
    if (process_kill(pid) == 0) {
        shell_print_success("Process terminated.");
        return 0;
    } else {
        shell_print_error("Cannot terminate process.");
        return 1;
    }
}

int cmd_mem(int argc, char **argv) {
    (void)argc; (void)argv;
    struct mem_stats stats;
    mem_get_stats(&stats);
    
    vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
    vga_puts("\n  Memory Usage:\n");
    vga_puts("  ═══════════════════════════════════════\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("  Total:    "); vga_print_size(stats.total); vga_puts("\n");
    vga_puts("  Used:     "); vga_print_size(stats.used);  vga_puts(" (");
    vga_print_dec(stats.used * 100 / stats.total); vga_puts("%)\n");
    vga_puts("  Free:     "); vga_print_size(stats.free);  vga_puts("\n");
    vga_puts("  Heap:     "); vga_print_size(stats.heap_used); vga_puts(" / ");
    vga_print_size(stats.heap_total); vga_puts("\n");
    vga_puts("\n");
    return 0;
}

int cmd_df(int argc, char **argv) {
    (void)argc; (void)argv;
    struct mexfs_stats fs;
    mexfs_statfs(&fs);
    
    vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
    vga_puts("\n  Disk Usage (.mex filesystem):\n");
    vga_puts("  ═══════════════════════════════════════\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("  Total blocks:  "); vga_print_dec(fs.total_blocks); vga_puts("\n");
    vga_puts("  Used blocks:   "); vga_print_dec(fs.used_blocks); vga_puts("\n");
    vga_puts("  Free blocks:   "); vga_print_dec(fs.free_blocks); vga_puts("\n");
    vga_puts("  Files:         "); vga_print_dec(fs.file_count); vga_puts(" / ");
    vga_print_dec(fs.max_files); vga_puts("\n");
    vga_puts("\n");
    return 0;
}

int cmd_uptime(int argc, char **argv) {
    (void)argc; (void)argv;
    uint64_t ticks = timer_get_uptime();
    uint64_t seconds = ticks / TIMER_FREQ;
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    
    vga_puts(" ");
    if (hours > 0) { vga_print_dec(hours); vga_puts("h "); }
    if (minutes > 0) { vga_print_dec(minutes % 60); vga_puts("m "); }
    vga_print_dec(seconds % 60);
    vga_puts("s (");
    vga_print_dec(ticks);
    vga_puts(" ticks)\n");
    return 0;
}

int cmd_echo(int argc, char **argv) {
    vga_puts(" ");
    for (int i = 1; i < argc; i++) {
        vga_puts(argv[i]);
        if (i < argc - 1) vga_putc(' ');
    }
    vga_puts("\n");
    return 0;
}

int cmd_intent(int argc, char **argv) {
    if (argc < 2) {
        vga_puts("  Usage: intent <natural language description>\n");
        vga_puts("  Example: intent \"organize my desktop for coding\"\n");
        return 1;
    }
    
    /* Concatenate all arguments into intent string */
    char intent_str[SHELL_INPUT_SIZE];
    intent_str[0] = '\0';
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(intent_str, " ");
        strcat(intent_str, argv[i]);
    }
    
    vga_setcolor(VGA_COLOR_LMAGENTA, VGA_COLOR_BLACK);
    vga_puts("\n  [AI] Processing intent: \"");
    vga_puts(intent_str);
    vga_puts("\"\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    struct intent_result result;
    if (ai_process_intent(intent_str, &result) == 0) {
        vga_setcolor(VGA_COLOR_LGREEN, VGA_COLOR_BLACK);
        vga_puts("  [AI] Recognized: ");
        vga_puts(result.category_name);
        vga_puts("\n  [AI] Confidence: ");
        vga_print_dec(result.confidence);
        vga_puts("%\n  [AI] Actions:\n");
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        for (int i = 0; i < result.action_count; i++) {
            vga_puts("       ");
            vga_print_dec(i + 1);
            vga_puts(". ");
            vga_puts(result.actions[i]);
            vga_puts("\n");
        }
        ai_execute_actions(&result);
        vga_puts("\n");
        return 0;
    } else {
        vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
        vga_puts("  [AI] Could not understand intent. Try rephrasing.\n\n");
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        return 1;
    }
}

int cmd_ai(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "on") == 0) {
            shell_ai_mode_enter();
            return 0;
        } else if (strcmp(argv[1], "off") == 0) {
            shell_ai_mode_exit();
            return 0;
        }
    }
    
    /* Toggle */
    if (shell.ai_mode) {
        shell_ai_mode_exit();
    } else {
        shell_ai_mode_enter();
    }
    return 0;
}

int cmd_workspace(int argc, char **argv) {
    if (argc < 2) {
        vga_puts("  Usage: workspace <name>\n");
        vga_puts("  Creates or switches to an AI workspace.\n");
        return 1;
    }
    
    vga_setcolor(VGA_COLOR_LMAGENTA, VGA_COLOR_BLACK);
    vga_puts("\n  [AI] Activating workspace: \"");
    vga_puts(argv[1]);
    vga_puts("\"\n");
    
    ai_workspace_load(argv[1]);
    
    vga_setcolor(VGA_COLOR_LGREEN, VGA_COLOR_BLACK);
    vga_puts("  [AI] Workspace ready.\n\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    return 0;
}

int cmd_motd(int argc, char **argv) {
    (void)argc; (void)argv;
    shell_welcome();
    return 0;
}

int cmd_reboot(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_setcolor(VGA_COLOR_LYELLOW, VGA_COLOR_BLACK);
    vga_puts("\n  System is rebooting...\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    /* Send reboot command to keyboard controller */
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
    
    /* Should not reach here */
    while (1) hlt();
    return 0;
}

int cmd_halt(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
    vga_puts("\n  System halted.\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    cli();
    while (1) hlt();
    return 0;
}

int cmd_sysinfo(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
    vga_puts("\n  System Information:\n");
    vga_puts("  ═══════════════════════════════════════════════════\n\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    
    vga_puts("  OS Name:     MexaOS\n");
    vga_puts("  Version:     " MEXAOS_VERSION "\n");
    vga_puts("  Build:       ");
    vga_print_dec(MEXAOS_BUILD);
    vga_puts("\n");
    vga_puts("  Codename:    \"" MEXAOS_CODENAME "\"\n");
    vga_puts("  Build Date:  " MEXAOS_BUILD_DATE "\n");
    vga_puts("  Architecture: x86_64\n");
    vga_puts("  Kernel:      MexaOS Kernel\n");
    vga_puts("  Shell:       MexaShell\n");
    vga_puts("  Filesystem:  .mex (Mexa File System)\n");
    vga_puts("  AI Engine:   Intent Engine v1\n");
    vga_puts("\n");
    return 0;
}

int cmd_theme(int argc, char **argv) {
    if (argc < 2) {
        vga_puts("  Available themes: default, dark, matrix, ocean, sunset\n");
        return 0;
    }
    
    if (strcmp(argv[1], "matrix") == 0) {
        vga_setcolor(VGA_COLOR_LGREEN, VGA_COLOR_BLACK);
    } else if (strcmp(argv[1], "ocean") == 0) {
        vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
    } else if (strcmp(argv[1], "sunset") == 0) {
        vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
    } else {
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    }
    
    vga_puts("  Theme applied: ");
    vga_puts(argv[1]);
    vga_puts("\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    return 0;
}

int cmd_mex(int argc, char **argv) {
    if (argc < 2) {
        vga_puts("  Usage: mex <file> [info|exec]\n");
        return 1;
    }
    
    const char *op = (argc > 2) ? argv[2] : "info";
    
    if (strcmp(op, "info") == 0) {
        struct mexfs_stat st;
        if (mexfs_stat(argv[1], &st) == 0) {
            vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
            vga_puts("\n  .mex File Info:\n");
            vga_puts("  ═══════════════════════════════════════\n");
            vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            vga_puts("  Name: "); vga_puts(argv[1]); vga_puts("\n");
            vga_puts("  Size: "); vga_print_dec(st.size); vga_puts(" bytes\n");
            vga_puts("  Type: ");
            switch (st.type) {
                case MEX_TYPE_REGULAR:   vga_puts("Regular file\n"); break;
                case MEX_TYPE_DIRECTORY: vga_puts("Directory\n"); break;
                case MEX_TYPE_SYMLINK:   vga_puts("Symbolic link\n"); break;
                case MEX_TYPE_DEVICE:    vga_puts("Device\n"); break;
                case MEX_TYPE_PIPE:      vga_puts("Pipe\n"); break;
                case MEX_TYPE_AI_MODEL:  vga_puts("AI Model\n"); break;
                case MEX_TYPE_INTENT:    vga_puts("Intent Script\n"); break;
                default:                 vga_puts("Unknown\n"); break;
            }
            vga_puts("\n");
            return 0;
        }
    } else if (strcmp(op, "exec") == 0) {
        vga_setcolor(VGA_COLOR_LGREEN, VGA_COLOR_BLACK);
        vga_puts("  Executing .mex intent: ");
        vga_puts(argv[1]);
        vga_puts("\n");
        vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        ai_execute_mex_file(argv[1]);
        return 0;
    }
    
    shell_print_error("Operation failed.");
    return 1;
}

/* ============================================================
 *  AI Mode
 * ============================================================ */

void shell_ai_mode_enter(void) {
    shell.ai_mode = 1;
    vga_setcolor(VGA_COLOR_LMAGENTA, VGA_COLOR_BLACK);
    vga_puts("\n  [AI] MexaOS AI Mode activated.\n");
    vga_puts("  Describe what you want to do in natural language.\n");
    vga_puts("  Type 'ai off' or press Ctrl+C to exit AI mode.\n\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void shell_ai_mode_exit(void) {
    shell.ai_mode = 0;
    vga_setcolor(VGA_COLOR_LCYAN, VGA_COLOR_BLACK);
    vga_puts("\n  [AI] AI mode deactivated.\n\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

void shell_ai_prompt(void) {
    vga_setcolor(VGA_COLOR_LMAGENTA, VGA_COLOR_BLACK);
    vga_puts("  AI> What would you like to do?\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

/* ============================================================
 *  Tab Completion
 * ============================================================ */

void shell_tab_complete(char *input, int *pos) {
    if (*pos == 0) return;
    
    /* Simple command completion */
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strncmp(input, commands[i].name, *pos) == 0) {
            int len = strlen(commands[i].name);
            strcpy(input, commands[i].name);
            /* Clear and redraw */
            for (int j = 0; j < *pos; j++) {
                vga_putc('\b');
                vga_putc(' ');
                vga_putc('\b');
            }
            vga_puts(commands[i].name);
            *pos = len;
            return;
        }
    }
}

/* ============================================================
 *  History
 * ============================================================ */

void shell_history_add(const char *line) {
    if (!line || line[0] == '\0') return;
    
    /* Don't add duplicates of the most recent entry */
    if (history_count > 0 && strcmp(history[0], line) == 0)
        return;
    
    /* Shift entries down */
    for (int i = SHELL_HISTORY_SIZE - 1; i > 0; i--) {
        strcpy(history[i], history[i - 1]);
    }
    
    strncpy(history[0], line, SHELL_INPUT_SIZE - 1);
    history[0][SHELL_INPUT_SIZE - 1] = '\0';
    
    if (history_count < SHELL_HISTORY_SIZE)
        history_count++;
    
    history_pos = -1;
}

const char *shell_history_prev(void) {
    if (history_count == 0) return NULL;
    if (history_pos < history_count - 1)
        history_pos++;
    return history[history_pos];
}

const char *shell_history_next(void) {
    if (history_pos <= 0) {
        history_pos = -1;
        return NULL;
    }
    history_pos--;
    return history[history_pos];
}

/* ============================================================
 *  Utility
 * ============================================================ */

static void shell_welcome(void) {
    vga_setcolor(VGA_COLOR_LBLUE, VGA_COLOR_BLACK);
    vga_puts("\n");
    vga_puts("  ╔═══════════════════════════════════════════════════════════╗\n");
    vga_puts("  ║           MexaShell — Intent-Driven Shell v");
    vga_print_dec(1);
    vga_puts(".0          ║\n");
    vga_puts("  ╠═══════════════════════════════════════════════════════════╣\n");
    vga_setcolor(VGA_COLOR_LGRAY, VGA_COLOR_BLACK);
    vga_puts("  ║  Type 'help' for commands  |  'ai' for AI mode          ║\n");
    vga_puts("  ║  Intent: try \"intent open a coding workspace\"           ║\n");
    vga_puts("  ╚═══════════════════════════════════════════════════════════╝\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("\n");
}

static void shell_print_error(const char *msg) {
    vga_setcolor(VGA_COLOR_LRED, VGA_COLOR_BLACK);
    vga_puts("  [E] ");
    vga_puts(msg);
    vga_puts("\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

static void shell_print_success(const char *msg) {
    vga_setcolor(VGA_COLOR_LGREEN, VGA_COLOR_BLACK);
    vga_puts("  [OK] ");
    vga_puts(msg);
    vga_puts("\n");
    vga_setcolor(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

/* ─── Simple string helpers (kernel has no libc) ─── */
static int atoi(const char *s) {
    int n = 0;
    while (*s >= '0' && *s <= '9')
        n = n * 10 + (*s++ - '0');
    return n;
}

static size_t strlen(const char *s) {
    size_t n = 0;
    while (*s++) n++;
    return n;
}

static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *a == *b) { a++; b++; n--; }
    return n ? (unsigned char)*a - (unsigned char)*b : 0;
}

static char *strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++));
    return dst;
}

static char *strncpy(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++)
        dst[i] = src[i];
    for (; i < n; i++)
        dst[i] = '\0';
    return dst;
}

static char *strcat(char *dst, const char *src) {
    char *d = dst + strlen(dst);
    while ((*d++ = *src++));
    return dst;
}

static void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}
