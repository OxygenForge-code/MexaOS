/* ============================================================
 *  MexaOS Process Scheduler Header
 *  Multitasking with intent-driven process management
 * ============================================================ */

#ifndef PROCESS_H
#define PROCESS_H

#include "../../include/mexaos.h"

/* ─── Process Control Block ─── */
struct process {
    pid_t pid;
    pid_t ppid;
    char name[PROC_NAME_LEN];
    
    /* State */
    uint8_t state;
    uint8_t priority;           /* 0-255, higher = more priority */
    uint8_t quantum;            /* Time slices remaining */
    uint8_t flags;
    
    /* Memory */
    struct page_table *page_table;
    virt_addr_t stack_bottom;
    virt_addr_t stack_top;
    virt_addr_t entry_point;
    
    /* Context (saved registers) */
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip, rflags, cr3;
    uint64_t cs, ss, ds, es, fs, gs;
    
    /* Scheduling */
    uint64_t cpu_time;          /* Total CPU ticks used */
    uint64_t start_time;        /* Process start tick */
    uint64_t wake_time;         /* For sleeping processes */
    
    /* MexaOS: Intent context */
    char intent_context[256];   /* Current intent associated with process */
    uint8_t ai_assisted;        /* Process was created by AI */
    
    /* Linked list */
    struct process *next;
    struct process *prev;
};

/* ─── Process Flags ─── */
#define PROC_FLAG_KERNEL    0x01
#define PROC_FLAG_DAEMON    0x02
#define PROC_FLAG_CRITICAL  0x04
#define PROC_FLAG_NO_KILL   0x08
#define PROC_FLAG_AI_CREATED 0x10

/* ─── Scheduler Functions ─── */
void scheduler_init(void);
void scheduler_tick(void);
void scheduler_switch_task(void);

/* ─── Process Management ─── */
pid_t process_create(const char *name, void (*entry)(void), uint8_t priority);
pid_t process_create_from_intent(const char *intent, uint8_t priority);
int process_kill(pid_t pid);
int process_block(pid_t pid);
int process_unblock(pid_t pid);
int process_sleep(pid_t pid, uint64_t ms);
void process_yield(void);
void process_exit(int code);

/* ─── Process Queries ─── */
struct process *process_get_current(void);
struct process *process_get(pid_t pid);
int process_set_priority(pid_t pid, uint8_t priority);
int process_set_name(pid_t pid, const char *name);

/* ─── Process List ─── */
void process_list(void);
int process_get_count(void);
void process_dump_info(pid_t pid);

/* ─── MexaOS: Intent-driven process creation ─── */
pid_t process_spawn_for_intent(const char *intent_text);
void process_attach_intent(pid_t pid, const char *intent);

#endif /* PROCESS_H */
