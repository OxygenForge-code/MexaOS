/* ============================================================
 *  MexaOS Process Manager
 * ============================================================ */

#ifndef MEXAOS_PROCESS_H
#define MEXAOS_PROCESS_H

#include <stdint.h>
#include <stddef.h>

/* ─── Type Definitions ─── */
typedef int pid_t;
typedef uint64_t virt_addr_t;

/* ─── Constants ─── */
#define PROC_NAME_LEN   64
#define PROC_MAX        256
#define PROC_STACK_SIZE 65536
#define MAX_PROCESSES   256
#define KERNEL_PID      0

/* ─── Process States ─── */
#define PROC_EMPTY      0
#define PROC_READY      1
#define PROC_RUNNING    2
#define PROC_BLOCKED    3
#define PROC_SLEEPING   4
#define PROC_ZOMBIE     5

/* ─── Process Flags ─── */
#define PROC_FLAG_KERNEL      (1 << 0)
#define PROC_FLAG_DAEMON      (1 << 1)
#define PROC_FLAG_CRITICAL    (1 << 2)
#define PROC_FLAG_NO_KILL     (1 << 3)
#define PROC_FLAG_AI_CREATED  (1 << 4)

/* ─── Process Structure ─── */
struct process {
    pid_t pid;
    pid_t ppid;
    char name[PROC_NAME_LEN];
    uint8_t state;
    uint8_t priority;
    uint64_t ticks;
    uint64_t sleep_until;
    
    virt_addr_t stack_bottom;
    virt_addr_t stack_top;
    virt_addr_t entry_point;
    
    uint64_t *page_table;
    struct process *next;
    struct process *prev;
    
    /* Additional fields for process.c compatibility */
    uint32_t quantum;
    uint32_t flags;
    uint64_t cpu_time;
    uint64_t start_time;
    uint64_t wake_time;
    uint64_t rsp;
    uint64_t rip;
    uint64_t rflags;
    uint16_t cs;
    uint16_t ss;
    char intent_context[256];
    uint8_t ai_assisted;
};

/* ─── Scheduler Functions ─── */
void scheduler_init(void);
void scheduler_tick(void);
void scheduler_switch_task(void);
void scheduler_yield(void);
struct process *scheduler_current(void);

/* ─── Process Functions ─── */
pid_t process_create(const char *name, void (*entry)(void), uint8_t priority);
pid_t process_create_from_intent(const char *intent, uint8_t priority);
int process_kill(pid_t pid);
int process_block(pid_t pid);
int process_unblock(pid_t pid);
int process_sleep(pid_t pid, uint64_t ms);
void process_exit(int code);

struct process *process_get(pid_t pid);
int process_set_priority(pid_t pid, uint8_t priority);
int process_set_name(pid_t pid, const char *name);

void process_list(void);
void process_dump_info(pid_t pid);

/* ─── Intent Integration ─── */
pid_t process_spawn_for_intent(const char *intent_text);
void process_attach_intent(pid_t pid, const char *intent);

#endif
