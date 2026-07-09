/* ============================================================
 *  MexaOS Process Scheduler
 *  Round-robin with priority + intent-driven process creation
 * ============================================================ */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../../include/mexaos.h"
#include "include/process.h"
#include "include/interrupt.h"
#include "include/vga.h"
#include "include/timer.h"
#include "include/memory.h"


static struct process process_table[MAX_PROCESSES];
static struct process *current_process = NULL;
static struct process *ready_queue = NULL;
static pid_t next_pid = 1;
static int process_count = 0;
static uint64_t scheduler_ticks = 0;
static struct process *idle_proc = NULL;

static const uint8_t priority_quantum[] = {
    1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6
};

void scheduler_init(void) {
    memset(process_table, 0, sizeof(process_table));
    idle_proc = &process_table[0];
    idle_proc->pid = KERNEL_PID;
    idle_proc->ppid = KERNEL_PID;
    strcpy(idle_proc->name, "idle");
    idle_proc->state = PROC_READY;
    idle_proc->priority = 0;
    idle_proc->quantum = 1;
    idle_proc->flags = PROC_FLAG_KERNEL | PROC_FLAG_NO_KILL;
    idle_proc->page_table = &kernel_page_table;
    idle_proc->next = idle_proc;
    idle_proc->prev = idle_proc;
    current_process = idle_proc;
    ready_queue = idle_proc;
    next_pid = 1;
    process_count = 1;
    scheduler_ticks = 0;
}

pid_t process_create(const char *name, void (*entry)(void), uint8_t priority) {
    if (process_count >= MAX_PROCESSES) return -1;
    int idx = -1;
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_EMPTY) { idx = i; break; }
    }
    if (idx < 0) return -1;
    
    struct process *proc = &process_table[idx];
    memset(proc, 0, sizeof(struct process));
    proc->pid = next_pid++;
    proc->ppid = current_process ? current_process->pid : KERNEL_PID;
    strncpy(proc->name, name, PROC_NAME_LEN - 1);
    proc->name[PROC_NAME_LEN - 1] = '\0';
    proc->state = PROC_READY;
    proc->priority = priority > 15 ? 15 : priority;
    proc->quantum = priority_quantum[proc->priority];
    proc->entry_point = (virt_addr_t)entry;
    proc->start_time = timer_get_ticks();
    
    proc->page_table = vmm_create_page_table();
    if (!proc->page_table) { proc->state = PROC_EMPTY; return -1; }
    
    proc->stack_bottom = vmm_alloc_region(proc->page_table, 16, PTE_PRESENT | PTE_WRITABLE | PTE_USER);
    if (!proc->stack_bottom) {
        vmm_destroy_page_table(proc->page_table);
        proc->state = PROC_EMPTY;
        return -1;
    }
    proc->stack_top = proc->stack_bottom + 16 * PAGE_SIZE;
    proc->rsp = proc->stack_top - 8;
    proc->rip = (uint64_t)entry;
    proc->rflags = 0x200;
    proc->cs = 0x08;
    proc->ss = 0x10;
    
    if (ready_queue) {
        proc->next = ready_queue;
        proc->prev = ready_queue->prev;
        ready_queue->prev->next = proc;
        ready_queue->prev = proc;
    } else {
        ready_queue = proc;
        proc->next = proc;
        proc->prev = proc;
    }
    process_count++;
    return proc->pid;
}

pid_t process_create_from_intent(const char *intent, uint8_t priority) {
    char proc_name[PROC_NAME_LEN];
    if (strstr(intent, "browser") || strstr(intent, "web") || strstr(intent, "internet"))
        strcpy(proc_name, "mexa-browser");
    else if (strstr(intent, "terminal") || strstr(intent, "shell") || strstr(intent, "command"))
        strcpy(proc_name, "mexa-terminal");
    else if (strstr(intent, "editor") || strstr(intent, "text") || strstr(intent, "write"))
        strcpy(proc_name, "mexa-editor");
    else if (strstr(intent, "file") || strstr(intent, "folder") || strstr(intent, "explore"))
        strcpy(proc_name, "mexa-files");
    else if (strstr(intent, "music") || strstr(intent, "audio") || strstr(intent, "sound"))
        strcpy(proc_name, "mexa-music");
    else if (strstr(intent, "video") || strstr(intent, "movie") || strstr(intent, "watch"))
        strcpy(proc_name, "mexa-video");
    else if (strstr(intent, "image") || strstr(intent, "photo") || strstr(intent, "picture"))
        strcpy(proc_name, "mexa-gallery");
    else if (strstr(intent, "code") || strstr(intent, "program") || strstr(intent, "develop"))
        strcpy(proc_name, "mexa-code");
    else if (strstr(intent, "game") || strstr(intent, "play"))
        strcpy(proc_name, "mexa-game");
    else if (strstr(intent, "setting") || strstr(intent, "config") || strstr(intent, "preference"))
        strcpy(proc_name, "mexa-settings");
    else
        snprintf(proc_name, sizeof(proc_name), "mexa-app-%d", next_pid);
    
    pid_t pid = process_create(proc_name, NULL, priority);
    if (pid > 0) {
        struct process *proc = process_get(pid);
        if (proc) {
            strncpy(proc->intent_context, intent, 255);
            proc->intent_context[255] = '\0';
            proc->ai_assisted = 1;
            proc->flags |= PROC_FLAG_AI_CREATED;
        }
    }
    return pid;
}

void scheduler_tick(void) {
    scheduler_ticks++;
    if (!current_process) return;
    current_process->cpu_time++;
    if (current_process->quantum > 0) { current_process->quantum--; return; }
    scheduler_switch_task();
}

void scheduler_switch_task(void) {
    if (!ready_queue || !current_process) return;
    struct process *next = current_process->next;
    int attempts = 0;
    while (attempts < MAX_PROCESSES) {
        if (next->state == PROC_READY && next != idle_proc) break;
        next = next->next;
        attempts++;
    }
    if (next->state != PROC_READY) next = idle_proc;
    if (next == current_process) return;
    
    struct process *prev = current_process;
    if (next->page_table != prev->page_table)
        vmm_switch_page_table(next->page_table);
    
    if (prev->state == PROC_RUNNING) {
        prev->state = PROC_READY;
        prev->quantum = priority_quantum[prev->priority];
    }
    next->state = PROC_RUNNING;
    next->quantum = priority_quantum[next->priority];
    current_process = next;
}

void process_yield(void) {
    if (current_process) current_process->quantum = 0;
    scheduler_switch_task();
}

int process_kill(pid_t pid) {
    if (pid <= 0 || pid >= next_pid) return -1;
    struct process *proc = process_get(pid);
    if (!proc || proc->state == PROC_EMPTY) return -1;
    if (proc->flags & PROC_FLAG_NO_KILL) return -1;
    if (proc->next && proc->prev) {
        proc->prev->next = proc->next;
        proc->next->prev = proc->prev;
    }
    if (proc->page_table && proc->page_table != &kernel_page_table)
        vmm_destroy_page_table(proc->page_table);
    proc->state = PROC_EMPTY;
    process_count--;
    if (proc == current_process) scheduler_switch_task();
    return 0;
}

int process_block(pid_t pid) {
    struct process *proc = process_get(pid);
    if (!proc || proc->state != PROC_RUNNING) return -1;
    proc->state = PROC_BLOCKED;
    if (proc == current_process) scheduler_switch_task();
    return 0;
}

int process_unblock(pid_t pid) {
    struct process *proc = process_get(pid);
    if (!proc || proc->state != PROC_BLOCKED) return -1;
    proc->state = PROC_READY;
    proc->quantum = priority_quantum[proc->priority];
    return 0;
}

int process_sleep(pid_t pid, uint64_t ms) {
    struct process *proc = process_get(pid);
    if (!proc) return -1;
    proc->wake_time = timer_get_ms() + ms;
    proc->state = PROC_SLEEPING;
    if (proc == current_process) scheduler_switch_task();
    return 0;
}

void process_exit(int code) {
    (void)code;
    if (current_process && !(current_process->flags & PROC_FLAG_NO_KILL))
        process_kill(current_process->pid);
    scheduler_switch_task();
}

struct process *process_get_current(void) { return current_process; }

struct process *process_get(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (process_table[i].pid == pid && process_table[i].state != PROC_EMPTY)
            return &process_table[i];
    return NULL;
}

int process_set_priority(pid_t pid, uint8_t priority) {
    struct process *proc = process_get(pid);
    if (!proc) return -1;
    proc->priority = priority > 15 ? 15 : priority;
    proc->quantum = priority_quantum[proc->priority];
    return 0;
}

int process_set_name(pid_t pid, const char *name) {
    struct process *proc = process_get(pid);
    if (!proc) return -1;
    strncpy(proc->name, name, PROC_NAME_LEN - 1);
    proc->name[PROC_NAME_LEN - 1] = '\0';
    return 0;
}

void process_list(void) {
    vga_puts("\n  PID  PPID  STATE      CPU(ms)  PRI  NAME              INTENT\n");
    vga_puts("  ───  ────  ────────   ───────  ───  ────────────────  ──────────────────\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        struct process *p = &process_table[i];
        if (p->state == PROC_EMPTY) continue;
        vga_puts("  "); vga_print_dec(p->pid);
        vga_puts("    "); vga_print_dec(p->ppid); vga_puts("    ");
        const char *state_str;
        switch (p->state) {
            case PROC_READY:    state_str = "ready   "; break;
            case PROC_RUNNING:  state_str = "running "; break;
            case PROC_BLOCKED:  state_str = "blocked "; break;
            case PROC_SLEEPING: state_str = "sleeping"; break;
            case PROC_ZOMBIE:   state_str = "zombie  "; break;
            default:            state_str = "unknown "; break;
        }
        vga_puts(state_str); vga_puts("   ");
        vga_print_dec(p->cpu_time); vga_puts("     ");
        vga_print_dec(p->priority); vga_puts("    ");
        vga_puts(p->name);
        if (p->intent_context[0]) { vga_puts("  "); vga_puts(p->intent_context); }
        if (p->ai_assisted) vga_puts(" [AI]");
        vga_puts("\n");
    }
    vga_puts("\n  "); vga_print_dec(process_count); vga_puts(" processes total\n");
}

int process_get_count(void) { return process_count; }

void process_dump_info(pid_t pid) {
    struct process *p = process_get(pid);
    if (!p) { vga_puts("Process not found\n"); return; }
    vga_puts("\n  Process #"); vga_print_dec(p->pid);
    vga_puts(": "); vga_puts(p->name); vga_puts("\n");
    vga_puts("  ──────────────────────────\n");
    vga_puts("  Parent:     "); vga_print_dec(p->ppid); vga_puts("\n");
    vga_puts("  State:      ");
    switch (p->state) {
        case PROC_READY:    vga_puts("Ready\n"); break;
        case PROC_RUNNING:  vga_puts("Running\n"); break;
        case PROC_BLOCKED:  vga_puts("Blocked\n"); break;
        case PROC_SLEEPING: vga_puts("Sleeping\n"); break;
        case PROC_ZOMBIE:   vga_puts("Zombie\n"); break;
        default:            vga_puts("Unknown\n"); break;
    }
    vga_puts("  Priority:   "); vga_print_dec(p->priority); vga_puts("\n");
    vga_puts("  CPU time:   "); vga_print_dec(p->cpu_time); vga_puts(" ticks\n");
    vga_puts("  Stack:      0x"); vga_print_hex(p->stack_bottom);
    vga_puts(" - 0x"); vga_print_hex(p->stack_top); vga_puts("\n");
    vga_puts("  Entry:      0x"); vga_print_hex(p->entry_point); vga_puts("\n");
    vga_puts("  RIP:        0x"); vga_print_hex(p->rip); vga_puts("\n");
    vga_puts("  RSP:        0x"); vga_print_hex(p->rsp); vga_puts("\n");
    vga_puts("  RFLAGS:     0x"); vga_print_hex(p->rflags); vga_puts("\n");
    if (p->intent_context[0]) {
        vga_puts("  Intent:     \""); vga_puts(p->intent_context); vga_puts("\"\n");
    }
    vga_puts("  Flags:      ");
    if (p->flags & PROC_FLAG_KERNEL)     vga_puts("KERNEL ");
    if (p->flags & PROC_FLAG_DAEMON)     vga_puts("DAEMON ");
    if (p->flags & PROC_FLAG_CRITICAL)   vga_puts("CRITICAL ");
    if (p->flags & PROC_FLAG_NO_KILL)    vga_puts("NO_KILL ");
    if (p->flags & PROC_FLAG_AI_CREATED) vga_puts("AI_CREATED ");
    vga_puts("\n");
}

pid_t process_spawn_for_intent(const char *intent_text) {
    return process_create_from_intent(intent_text, 8);
}

void process_attach_intent(pid_t pid, const char *intent) {
    struct process *proc = process_get(pid);
    if (proc) {
        strncpy(proc->intent_context, intent, 255);
        proc->intent_context[255] = '\0';
    }
}

static char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    char *h = (char *)haystack;
    while (*h) {
        if (*h == *needle) {
            char *h2 = h + 1;
            char *n2 = (char *)needle + 1;
            while (*n2 && *h2 == *n2) { h2++; n2++; }
            if (!*n2) return h;
        }
        h++;
    }
    return NULL;
}
