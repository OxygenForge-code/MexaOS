/* ============================================================
 *  MexaOS AI Intent Engine Header
 *  The heart of the intent-driven operating system
 * ============================================================ */

#ifndef AI_ENGINE_H
#define AI_ENGINE_H

#include "../../include/mexaos.h"

/* ─── Intent Command ─── */
struct intent_command {
    int category;
    char action[64];
    char target[256];
    char parameters[512];
    float confidence;
    uint32_t flags;
};

/* ─── Intent History ─── */
#define MAX_INTENT_HISTORY  128
struct intent_record {
    char text[INTENT_MAX_LEN];
    int category;
    int result;
    uint64_t timestamp;
    float confidence;
};

/* ─── Workspace State ─── */
struct workspace_state {
    char name[32];
    int id;
    char theme[32];
    char layout[32];
    uint8_t focus_mode;
    uint8_t notifications;
    uint64_t active_since;
};

/* ─── AI Engine Functions ─── */
void ai_engine_init(void);
void ai_process_pending(void);

/* Intent processing */
int ai_process_intent(const char *text);
int ai_parse_intent(const char *text, struct intent_command *cmd);
float ai_calculate_confidence(const char *text, int category);

/* Intent categories */
int ai_classify_intent(const char *text);
const char *ai_intent_name(int category);

/* Intent execution */
int ai_execute_command(struct intent_command *cmd);
int ai_execute_system(const char *action, const char *target, const char *params);
int ai_execute_file(const char *action, const char *target, const char *params);
int ai_execute_workspace(const char *action, const char *target, const char *params);
int ai_execute_configure(const char *action, const char *target, const char *params);

/* Workspace management */
void ai_workspace_switch(int ws_id);
void ai_workspace_create(const char *name);
void ai_workspace_set_theme(const char *theme);
void ai_workspace_set_layout(const char *layout);
void ai_workspace_focus_mode(uint8_t enabled);
void ai_workspace_quick_actions(void);

/* Learning & memory */
void ai_learn_pattern(const char *text, int category);
void ai_save_memory(void);
void ai_load_memory(void);
float ai_get_pattern_score(const char *text, int category);

/* Context awareness */
void ai_set_context(const char *context);
const char *ai_get_context(void);
void ai_update_context_from_time(void);

/* Voice / Natural language helpers */
int ai_tokenize(const char *text, char tokens[][64], int max_tokens);
int ai_stem_word(const char *word, char *stem);
int ai_match_keywords(const char *text, const char **keywords, int num_keywords);

/* History */
void ai_add_history(const char *text, int category, int result, float confidence);
void ai_show_history(void);

/* MexaOS: Special intent handlers */
int ai_handle_intent_coding(const char *params);
int ai_handle_intent_focus(const char *params);
int ai_handle_intent_creative(const char *params);
int ai_handle_intent_calm(const char *params);
int ai_handle_intent_organize(const char *params);
int ai_handle_intent_search(const char *params);

/* System state queries */
void ai_system_status(void);
void ai_system_usage(void);

/* Status */
void ai_engine_status(void);

#endif /* AI_ENGINE_H */
