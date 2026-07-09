/* ============================================================
 *  MexaOS Shell Header (MexaShell)
 *  Intent-aware command line interface
 * ============================================================ */

#ifndef SHELL_H
#define SHELL_H

#include "../../include/mexaos.h"

/* ─── Shell Configuration ─── */
#define SHELL_PROMPT        "mexa> "
#define SHELL_PROMPT_AI     "mexa✦> "
#define SHELL_INPUT_SIZE    4096
#define SHELL_HISTORY_SIZE  128
#define SHELL_MAX_ARGS      32

/* ─── Shell State ─── */
struct shell_state {
    char cwd[MEX_MAX_PATH];
    char input[SHELL_INPUT_SIZE];
    int input_pos;
    uint8_t ai_mode;            /* AI intent mode active */
    uint8_t echo;               /* Echo input */
    uint8_t running;
    int last_exit_code;
    uint64_t commands_executed;
};

/* ─── Command Structure ─── */
struct shell_command {
    const char *name;
    const char *description;
    const char *usage;
    int (*handler)(int argc, char **argv);
    uint8_t ai_enabled;         /* Can be invoked via intent */
};

/* ─── Functions ─── */
void shell_init(void);
void shell_run(void);
void shell_prompt(void);
void shell_process_line(const char *line);
int shell_execute(int argc, char **argv);

/* Command handlers */
int cmd_help(int argc, char **argv);
int cmd_clear(int argc, char **argv);
int cmd_ls(int argc, char **argv);
int cmd_cd(int argc, char **argv);
int cmd_pwd(int argc, char **argv);
int cmd_cat(int argc, char **argv);
int cmd_touch(int argc, char **argv);
int cmd_rm(int argc, char **argv);
int cmd_mkdir(int argc, char **argv);
int cmd_ps(int argc, char **argv);
int cmd_kill(int argc, char **argv);
int cmd_mem(int argc, char **argv);
int cmd_df(int argc, char **argv);
int cmd_uptime(int argc, char **argv);
int cmd_echo(int argc, char **argv);
int cmd_intent(int argc, char **argv);
int cmd_ai(int argc, char **argv);
int cmd_workspace(int argc, char **argv);
int cmd_motd(int argc, char **argv);
int cmd_reboot(int argc, char **argv);
int cmd_halt(int argc, char **argv);
int cmd_sysinfo(int argc, char **argv);
int cmd_theme(int argc, char **argv);
int cmd_mex(int argc, char **argv);       /* .mex file operations */

/* AI mode */
void shell_ai_mode_enter(void);
void shell_ai_mode_exit(void);
void shell_ai_prompt(void);

/* Tab completion */
void shell_tab_complete(char *input, int *pos);

/* History */
void shell_history_add(const char *line);
const char *shell_history_prev(void);
const char *shell_history_next(void);

#endif /* SHELL_H */
