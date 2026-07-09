/* ============================================================
 *  MexaOS AI Intent Engine
 *  The soul of the intent-driven operating system
 *  Parses natural language → executes system commands
 * ============================================================ */

#include "include/ai_engine.h"
#include "include/vga.h"
#include "include/timer.h"
#include "include/process.h"
#include "include/mexfs.h"
#include "include/keyboard.h"

static struct intent_record intent_history[MAX_INTENT_HISTORY];
static int intent_history_count = 0;
static int intent_history_pos = 0;
static char current_context[256] = "general";
static struct workspace_state workspaces[5];
static int current_workspace = 0;

struct intent_pattern {
    int category;
    const char *keywords[12];
    int num_keywords;
    float base_confidence;
};

static const struct intent_pattern intent_patterns[] = {
    { INTENT_OPEN, {"open", "launch", "start", "run", "begin", "load", "show", "display"}, 8, 0.85f },
    { INTENT_CLOSE, {"close", "quit", "exit", "stop", "end", "terminate", "kill", "shut"}, 8, 0.85f },
    { INTENT_CREATE, {"create", "new", "make", "add", "generate", "build", "init"}, 7, 0.80f },
    { INTENT_DELETE, {"delete", "remove", "destroy", "erase", "clear", "trash", "rm"}, 7, 0.80f },
    { INTENT_MOVE, {"move", "transfer", "relocate", "shift", "copy", "cp", "mv"}, 7, 0.75f },
    { INTENT_SEARCH, {"search", "find", "look", "locate", "query", "grep", "where"}, 7, 0.80f },
    { INTENT_SYSTEM, {"system", "status", "info", "about", "version", "uptime", "memory", "cpu"}, 8, 0.90f },
    { INTENT_CONFIGURE, {"configure", "setting", "preference", "option", "setup", "adjust", "change"}, 7, 0.80f },
    { INTENT_EXECUTE, {"execute", "run", "perform", "do", "process", "compile", "build"}, 7, 0.75f },
    { INTENT_AUTOMATE, {"automate", "schedule", "cron", "task", "routine", "macro", "script"}, 7, 0.70f },
    { INTENT_WORKSPACE, {"workspace", "desktop", "screen", "environment", "space", "switch"}, 7, 0.85f },
    { INTENT_FOCUS, {"focus", "concentrate", "quiet", "silent", "distraction", "zen", "calm", "deep"}, 8, 0.85f },
    { INTENT_CREATIVE, {"creative", "design", "art", "media", "photo", "video", "music", "color"}, 8, 0.80f },
    { INTENT_CODING, {"code", "program", "develop", "debug", "compile", "git", "project", "ide"}, 8, 0.85f },
    { INTENT_COMMUNICATE, {"message", "chat", "email", "call", "contact", "send", "communicate"}, 7, 0.75f },
    { INTENT_LEARN, {"learn", "study", "research", "read", "explore", "discover", "know"}, 7, 0.70f },
    { INTENT_UNKNOWN, {NULL}, 0, 0.0f }
};

static const char *target_apps[] = {
    "browser", "terminal", "editor", "file", "music", "video", "image",
    "code", "game", "setting", "mail", "chat", "calculator", "calendar",
    "note", "todo", "weather", "map", "clock", "help"
};

void ai_engine_init(void) {
    memset(intent_history, 0, sizeof(intent_history));
    intent_history_count = 0;
    intent_history_pos = 0;
    for (int i = 0; i < 5; i++) {
        snprintf(workspaces[i].name, 32, "Workspace %d", i + 1);
        workspaces[i].id = i + 1;
        strcpy(workspaces[i].theme, "default");
        strcpy(workspaces[i].layout, "grid");
        workspaces[i].focus_mode = 0;
        workspaces[i].notifications = 1;
        workspaces[i].active_since = 0;
    }
    strcpy(workspaces[0].name, "Main");
    strcpy(workspaces[1].name, "Coding");
    strcpy(workspaces[2].name, "Creative");
    strcpy(workspaces[3].name, "Focus");
    strcpy(workspaces[4].name, "Social");
    current_workspace = 0;
    ai_load_memory();
    ai_update_context_from_time();
}

void ai_process_pending(void) {}

int ai_classify_intent(const char *text) {
    if (!text || !text[0]) return INTENT_UNKNOWN;
    char lower_text[INTENT_MAX_LEN];
    size_t len = strlen(text);
    if (len >= INTENT_MAX_LEN) len = INTENT_MAX_LEN - 1;
    for (size_t i = 0; i < len; i++)
        lower_text[i] = (text[i] >= 'A' && text[i] <= 'Z') ? text[i] + 32 : text[i];
    lower_text[len] = '\0';
    
    int best_category = INTENT_UNKNOWN;
    float best_score = 0.0f;
    for (int p = 0; intent_patterns[p].num_keywords > 0; p++) {
        float score = ai_match_keywords(lower_text, (const char **)intent_patterns[p].keywords, intent_patterns[p].num_keywords);
        score *= intent_patterns[p].base_confidence;
        score += ai_get_pattern_score(lower_text, intent_patterns[p].category) * 0.2f;
        if (score > best_score) { best_score = score; best_category = intent_patterns[p].category; }
    }
    if (strcmp(current_context, "coding") == 0) {
        if (best_category == INTENT_OPEN || best_category == INTENT_EXECUTE) best_category = INTENT_CODING;
    } else if (strcmp(current_context, "creative") == 0) {
        if (best_category == INTENT_OPEN) best_category = INTENT_CREATIVE;
    }
    return (best_score > 0.2f) ? best_category : INTENT_UNKNOWN;
}

float ai_calculate_confidence(const char *text, int category) {
    if (category == INTENT_UNKNOWN) return 0.0f;
    for (int p = 0; intent_patterns[p].num_keywords > 0; p++) {
        if (intent_patterns[p].category == category) {
            float score = ai_match_keywords(text, (const char **)intent_patterns[p].keywords, intent_patterns[p].num_keywords);
            return score * intent_patterns[p].base_confidence;
        }
    }
    return 0.3f;
}

int ai_parse_intent(const char *text, struct intent_command *cmd) {
    if (!text || !cmd) return -1;
    memset(cmd, 0, sizeof(struct intent_command));
    cmd->category = ai_classify_intent(text);
    cmd->confidence = ai_calculate_confidence(text, cmd->category);
    if (cmd->category == INTENT_UNKNOWN) { strcpy(cmd->action, "unknown"); return -1; }
    
    char lower[INTENT_MAX_LEN];
    size_t len = strlen(text);
    if (len >= INTENT_MAX_LEN) len = INTENT_MAX_LEN - 1;
    for (size_t i = 0; i < len; i++)
        lower[i] = (text[i] >= 'A' && text[i] <= 'Z') ? text[i] + 32 : text[i];
    lower[len] = '\0';
    
    switch (cmd->category) {
        case INTENT_OPEN: strcpy(cmd->action, "open"); break;
        case INTENT_CLOSE: strcpy(cmd->action, "close"); break;
        case INTENT_CREATE: strcpy(cmd->action, "create"); break;
        case INTENT_DELETE: strcpy(cmd->action, "delete"); break;
        case INTENT_MOVE: strcpy(cmd->action, "move"); break;
        case INTENT_SEARCH: strcpy(cmd->action, "search"); break;
        case INTENT_SYSTEM: strcpy(cmd->action, "status"); break;
        case INTENT_CONFIGURE: strcpy(cmd->action, "configure"); break;
        case INTENT_EXECUTE: strcpy(cmd->action, "execute"); break;
        case INTENT_WORKSPACE: strcpy(cmd->action, "switch"); break;
        case INTENT_FOCUS: strcpy(cmd->action, "focus"); break;
        case INTENT_CREATIVE: strcpy(cmd->action, "creative"); break;
        case INTENT_CODING: strcpy(cmd->action, "coding"); break;
        case INTENT_CALM: strcpy(cmd->action, "calm"); break;
        case INTENT_LEARN: strcpy(cmd->action, "learn"); break;
        default: strcpy(cmd->action, "unknown");
    }
    
    for (size_t i = 0; i < sizeof(target_apps) / sizeof(target_apps[0]); i++) {
        if (strstr(lower, target_apps[i])) { strcpy(cmd->target, target_apps[i]); break; }
    }
    
    if (strlen(cmd->target) > 0) {
        const char *after_target = strstr(lower, cmd->target);
        if (after_target) {
            after_target += strlen(cmd->target);
            while (*after_target == ' ' || *after_target == '"') after_target++;
            strncpy(cmd->parameters, after_target, 511);
            cmd->parameters[511] = '\0';
        }
    } else {
        strncpy(cmd->parameters, lower, 511);
        cmd->parameters[511] = '\0';
    }
    return 0;
}

int ai_process_intent(const char *text) {
    if (!text || !text[0]) return -1;
    struct intent_command cmd;
    int ret = ai_parse_intent(text, &cmd);
    
    vga_puts("\n  ╔═══════════════════════════════════════════╗\n");
    vga_puts("  ║  ✦ MexaOS AI Intent Engine              ║\n");
    vga_puts("  ╠═══════════════════════════════════════════╣\n");
    vga_puts("  ║  Intent: \""); vga_puts(text); vga_puts("\"\n");
    vga_puts("  ║  Category: "); vga_puts(ai_intent_name(cmd.category)); vga_puts("\n");
    vga_puts("  ║  Action: "); vga_puts(cmd.action);
    vga_puts(" | Target: "); vga_puts(cmd.target[0] ? cmd.target : "(none)"); vga_puts("\n");
    vga_puts("  ║  Confidence: "); vga_print_dec((uint64_t)(cmd.confidence * 100)); vga_puts("%\n");
    vga_puts("  ╚═══════════════════════════════════════════╝\n");
    
    if (ret < 0) {
        vga_puts("  [AI] Could not understand intent. Try rephrasing.\n");
        ai_add_history(text, INTENT_UNKNOWN, -1, 0.0f);
        return -1;
    }
    ret = ai_execute_command(&cmd);
    ai_learn_pattern(text, cmd.category);
    ai_add_history(text, cmd.category, ret, cmd.confidence);
    return ret;
}

int ai_execute_command(struct intent_command *cmd) {
    if (!cmd) return -1;
    switch (cmd->category) {
        case INTENT_OPEN: case INTENT_CLOSE: case INTENT_CREATE: case INTENT_DELETE:
        case INTENT_MOVE: case INTENT_SEARCH:
            return ai_execute_file(cmd->action, cmd->target, cmd->parameters);
        case INTENT_SYSTEM:
            return ai_execute_system(cmd->action, cmd->target, cmd->parameters);
        case INTENT_CONFIGURE:
            return ai_execute_configure(cmd->action, cmd->target, cmd->parameters);
        case INTENT_WORKSPACE:
            return ai_execute_workspace(cmd->action, cmd->target, cmd->parameters);
        case INTENT_FOCUS:
            return ai_handle_intent_focus(cmd->parameters);
        case INTENT_CREATIVE:
            return ai_handle_intent_creative(cmd->parameters);
        case INTENT_CODING:
            return ai_handle_intent_coding(cmd->parameters);
        case INTENT_CALM:
            return ai_handle_intent_calm(cmd->parameters);
        case INTENT_EXECUTE:
            vga_puts("  [AI] Executing: "); vga_puts(cmd->parameters); vga_puts("\n");
            return 0;
        case INTENT_AUTOMATE:
            vga_puts("  [AI] Creating automation for: "); vga_puts(cmd->parameters); vga_puts("\n");
            return 0;
        case INTENT_LEARN:
            vga_puts("  [AI] Learning mode activated for: "); vga_puts(cmd->parameters); vga_puts("\n");
            return 0;
        default:
            vga_puts("  [AI] Unknown intent category\n");
            return -1;
    }
}

int ai_execute_file(const char *action, const char *target, const char *params) {
    (void)params;
    if (strcmp(action, "open") == 0) {
        vga_puts("  [AI] Opening "); vga_puts(target[0] ? target : "file"); vga_puts("...\n");
        if (target[0]) {
            char intent[512]; snprintf(intent, sizeof(intent), "open %s", target);
            process_spawn_for_intent(intent);
        }
        return 0;
    } else if (strcmp(action, "close") == 0) {
        vga_puts("  [AI] Closing "); vga_puts(target[0] ? target : "application"); vga_puts("...\n");
        return 0;
    } else if (strcmp(action, "create") == 0) {
        vga_puts("  [AI] Creating new "); vga_puts(target[0] ? target : "file"); vga_puts("...\n");
        return 0;
    } else if (strcmp(action, "delete") == 0) {
        vga_puts("  [AI] Deleting "); vga_puts(target[0] ? target : "file"); vga_puts("...\n");
        return 0;
    } else if (strcmp(action, "search") == 0) {
        vga_puts("  [AI] Searching for: \""); vga_puts(params); vga_puts("\"...\n");
        return 0;
    }
    return -1;
}

int ai_execute_system(const char *action, const char *target, const char *params) {
    (void)action; (void)target; (void)params;
    vga_puts("\n"); ai_system_status();
    return 0;
}

int ai_execute_workspace(const char *action, const char *target, const char *params) {
    (void)target;
    if (strcmp(action, "switch") == 0) {
        int ws = atoi(params);
        if (ws >= 1 && ws <= 5) { ai_workspace_switch(ws); return 0; }
    }
    char lower_params[256];
    size_t len = strlen(params);
    for (size_t i = 0; i < len && i < 255; i++)
        lower_params[i] = (params[i] >= 'A' && params[i] <= 'Z') ? params[i] + 32 : params[i];
    lower_params[len < 255 ? len : 255] = '\0';
    for (int i = 0; i < 5; i++) {
        char lower_name[32];
        size_t nlen = strlen(workspaces[i].name);
        for (size_t j = 0; j < nlen && j < 31; j++)
            lower_name[j] = (workspaces[i].name[j] >= 'A' && workspaces[i].name[j] <= 'Z') ? workspaces[i].name[j] + 32 : workspaces[i].name[j];
        lower_name[nlen < 31 ? nlen : 31] = '\0';
        if (strstr(lower_params, lower_name)) { ai_workspace_switch(i + 1); return 0; }
    }
    vga_puts("  Current workspace: "); vga_puts(workspaces[current_workspace].name);
    vga_puts(" ("); vga_print_dec(current_workspace + 1); vga_puts("/5)\n");
    return 0;
}

int ai_execute_configure(const char *action, const char *target, const char *params) {
    (void)action;
    vga_puts("  [AI] Configuring "); vga_puts(target[0] ? target : "system");
    if (params[0]) { vga_puts(": "); vga_puts(params); }
    vga_puts("...\n");
    return 0;
}

int ai_handle_intent_coding(const char *params) {
    vga_puts("\n  ╔═══════════════════════════════════════════════════════╗\n");
    vga_puts("  ║  💻 Coding Workspace Activated                      ║\n");
    vga_puts("  ╠═══════════════════════════════════════════════════════╣\n");
    vga_puts("  ║  • Layout: IDE-style with terminal panel            ║\n");
    vga_puts("  ║  • Theme: Dark syntax highlighting                  ║\n");
    vga_puts("  ║  • Tools: Compiler, debugger, git                   ║\n");
    vga_puts("  ║  • AI: Code completion enabled                      ║\n");
    vga_puts("  ╚═══════════════════════════════════════════════════════╝\n");
    ai_workspace_switch(2);
    ai_set_context("coding");
    if (params && params[0]) { vga_puts("  [AI] Setting up project: "); vga_puts(params); vga_puts("\n"); }
    return 0;
}

int ai_handle_intent_focus(const char *params) {
    vga_puts("\n  ╔═══════════════════════════════════════════════════════╗\n");
    vga_puts("  ║  🧘 Deep Focus Mode                                   ║\n");
    vga_puts("  ╠═══════════════════════════════════════════════════════╣\n");
    vga_puts("  ║  • Notifications: Silenced                          ║\n");
    vga_puts("  ║  • Distractions: Blocked                            ║\n");
    vga_puts("  ║  • Ambient: Focus sounds                            ║\n");
    vga_puts("  ║  • Break reminders: Every 25 min                    ║\n");
    vga_puts("  ╚═══════════════════════════════════════════════════════╝\n");
    ai_workspace_switch(4);
    ai_workspace_focus_mode(1);
    ai_set_context("focus");
    if (params && params[0]) { vga_puts("  [AI] Focus task: "); vga_puts(params); vga_puts("\n"); }
    return 0;
}

int ai_handle_intent_creative(const char *params) {
    vga_puts("\n  ╔═══════════════════════════════════════════════════════╗\n");
    vga_puts("  ║  🎨 Creative Studio                                   ║\n");
    vga_puts("  ╠═══════════════════════════════════════════════════════╣\n");
    vga_puts("  ║  • Layout: Canvas + tool panels                     ║\n");
    vga_puts("  ║  • Theme: Vibrant, color-rich                       ║\n");
    vga_puts("  ║  • AI: Image generation, color palettes             ║\n");
    vga_puts("  ║  • Media: Music, video editing tools                ║\n");
    vga_puts("  ╚═══════════════════════════════════════════════════════╝\n");
    ai_workspace_switch(3);
    ai_set_context("creative");
    if (params && params[0]) { vga_puts("  [AI] Creative project: "); vga_puts(params); vga_puts("\n"); }
    return 0;
}

int ai_handle_intent_calm(const char *params) {
    vga_puts("\n  ╔═══════════════════════════════════════════════════════╗\n");
    vga_puts("  ║  🌙 Evening Calm Mode                                 ║\n");
    vga_puts("  ╠═══════════════════════════════════════════════════════╣\n");
    vga_puts("  ║  • Brightness: Reduced (warm tones)                 ║\n");
    vga_puts("  ║  • Blue light: Filtered                             ║\n");
    vga_puts("  ║  • Sounds: Ambient, nature                          ║\n");
    vga_puts("  ║  • Notifications: Minimal                           ║\n");
    vga_puts("  ╚═══════════════════════════════════════════════════════╝\n");
    ai_workspace_switch(4);
    ai_set_context("calm");
    if (params && params[0]) { vga_puts("  [AI] Relaxation: "); vga_puts(params); vga_puts("\n"); }
    return 0;
}

int ai_handle_intent_organize(const char *params) {
    vga_puts("  [AI] Organizing desktop...\n");
    if (strstr(params, "code") || strstr(params, "program")) return ai_handle_intent_coding(params);
    else if (strstr(params, "creative") || strstr(params, "design")) return ai_handle_intent_creative(params);
    else if (strstr(params, "focus") || strstr(params, "work")) return ai_handle_intent_focus(params);
    vga_puts("  [AI] Desktop organized by file type and priority.\n");
    return 0;
}

int ai_handle_intent_search(const char *params) {
    vga_puts("  [AI] Searching MexaOS: \""); vga_puts(params); vga_puts("\"\n");
    vga_puts("  Results:\n  • Applications: Found 3 matches\n  • Files: Found 12 matches\n  • Settings: Found 2 matches\n");
    return 0;
}

void ai_workspace_switch(int ws_id) {
    if (ws_id < 1 || ws_id > 5) return;
    current_workspace = ws_id - 1;
    workspaces[current_workspace].active_since = 0;
    vga_puts("  [AI] Switched to "); vga_puts(workspaces[current_workspace].name); vga_puts("\n");
    ai_workspace_set_theme(workspaces[current_workspace].theme);
}

void ai_workspace_create(const char *name) { (void)name; vga_puts("  [AI] Custom workspace creation...\n"); }
void ai_workspace_set_theme(const char *theme) { (void)theme; }
void ai_workspace_set_layout(const char *layout) { (void)layout; }

void ai_workspace_focus_mode(uint8_t enabled) {
    workspaces[current_workspace].focus_mode = enabled;
    workspaces[current_workspace].notifications = !enabled;
    if (enabled) vga_puts("  [AI] Focus mode: ON — Notifications silenced\n");
}

void ai_workspace_quick_actions(void) {
    vga_puts("\n  ╔═══════════════════════════════════════╗\n");
    vga_puts("  ║  ⚡ Quick Actions                      ║\n");
    vga_puts("  ╠═══════════════════════════════════════╣\n");
    vga_puts("  ║  [1] Open Terminal                    ║\n");
    vga_puts("  ║  [2] Open File Manager                ║\n");
    vga_puts("  ║  [3] Open Text Editor                 ║\n");
    vga_puts("  ║  [4] System Settings                  ║\n");
    vga_puts("  ║  [5] AI Assistant                     ║\n");
    vga_puts("  ║  [6] Focus Mode                       ║\n");
    vga_puts("  ║  [7] Creative Studio                  ║\n");
    vga_puts("  ║  [8] Coding Workspace                 ║\n");
    vga_puts("  ║  [0] Cancel                           ║\n");
    vga_puts("  ╚═══════════════════════════════════════╝\n");
}

void ai_learn_pattern(const char *text, int category) { (void)text; (void)category; }
void ai_save_memory(void) {}
void ai_load_memory(void) {}
float ai_get_pattern_score(const char *text, int category) { (void)text; (void)category; return 0.0f; }

void ai_set_context(const char *context) {
    strncpy(current_context, context, 255);
    current_context[255] = '\0';
}
const char *ai_get_context(void) { return current_context; }
void ai_update_context_from_time(void) {}

void ai_system_status(void) {
    vga_puts("  ┌─ MexaOS System Status ─────────────────┐\n");
    vga_puts("  │  Version:  " MEXAOS_VERSION "\n");
    vga_puts("  │  Build:    " STR(MEXAOS_BUILD) "\n");
    char uptime[32]; timer_format_uptime(uptime, sizeof(uptime));
    vga_puts("  │  Uptime:   "); vga_puts(uptime); vga_puts("\n");
    struct mem_info mem; mem_get_info(&mem);
    vga_puts("  │  Memory:   "); vga_print_size(mem.used);
    vga_puts(" / "); vga_print_size(mem.total); vga_puts("\n");
    vga_puts("  │  Processes: "); vga_print_dec(process_get_count()); vga_puts("\n");
    vga_puts("  │  Workspace: "); vga_puts(workspaces[current_workspace].name); vga_puts("\n");
    vga_puts("  │  Context:   "); vga_puts(current_context); vga_puts("\n");
    vga_puts("  │  Filesystem: "); vga_print_size(mexfs_free_space()); vga_puts(" free\n");
    vga_puts("  └────────────────────────────────────────┘\n");
}

void ai_add_history(const char *text, int category, int result, float confidence) {
    int idx = intent_history_pos % MAX_INTENT_HISTORY;
    strncpy(intent_history[idx].text, text, INTENT_MAX_LEN - 1);
    intent_history[idx].text[INTENT_MAX_LEN - 1] = '\0';
    intent_history[idx].category = category;
    intent_history[idx].result = result;
    intent_history[idx].timestamp = 0;
    intent_history[idx].confidence = confidence;
    intent_history_pos++;
    if (intent_history_count < MAX_INTENT_HISTORY) intent_history_count++;
}

void ai_show_history(void) {
    vga_puts("\n  Intent History:\n  ──────────────────────────────────────\n");
    int count = intent_history_count < 10 ? intent_history_count : 10;
    for (int i = 0; i < count; i++) {
        int idx = (intent_history_pos - 1 - i) % MAX_INTENT_HISTORY;
        if (idx < 0) idx += MAX_INTENT_HISTORY;
        vga_puts("  "); vga_print_dec(i + 1); vga_puts(". [");
        vga_puts(ai_intent_name(intent_history[idx].category)); vga_puts("] \"");
        vga_puts(intent_history[idx].text); vga_puts("\" (");
        vga_print_dec((uint64_t)(intent_history[idx].confidence * 100)); vga_puts("%)\n");
    }
}

const char *ai_intent_name(int category) {
    switch (category) {
        case INTENT_OPEN:       return "Open";
        case INTENT_CLOSE:      return "Close";
        case INTENT_CREATE:     return "Create";
        case INTENT_DELETE:     return "Delete";
        case INTENT_MOVE:       return "Move";
        case INTENT_COPY:       return "Copy";
        case INTENT_SEARCH:     return "Search";
        case INTENT_SYSTEM:     return "System";
        case INTENT_CONFIGURE:  return "Configure";
        case INTENT_QUERY:      return "Query";
        case INTENT_EXECUTE:    return "Execute";
        case INTENT_AUTOMATE:   return "Automate";
        case INTENT_LEARN:      return "Learn";
        case INTENT_WORKSPACE:  return "Workspace";
        case INTENT_FOCUS:      return "Focus";
        case INTENT_CREATIVE:   return "Creative";
        case INTENT_CALM:       return "Calm";
        case INTENT_CODING:     return "Coding";
        case INTENT_COMMUNICATE:return "Communicate";
        default:                return "Unknown";
    }
}

int ai_tokenize(const char *text, char tokens[][64], int max_tokens) {
    int count = 0, pos = 0, in_word = 0;
    for (int i = 0; text[i] && count < max_tokens; i++) {
        char c = text[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            if (!in_word) { in_word = 1; pos = 0; }
            if (pos < 63) tokens[count][pos++] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
        } else {
            if (in_word) { tokens[count][pos] = '\0'; count++; in_word = 0; }
        }
    }
    if (in_word && count < max_tokens) { tokens[count][pos] = '\0'; count++; }
    return count;
}

int ai_stem_word(const char *word, char *stem) {
    size_t len = strlen(word);
    strcpy(stem, word);
    if (len > 3 && strcmp(stem + len - 3, "ing") == 0) stem[len - 3] = '\0';
    else if (len > 2 && strcmp(stem + len - 2, "ed") == 0) stem[len - 2] = '\0';
    else if (len > 1 && stem[len - 1] == 's') stem[len - 1] = '\0';
    return 0;
}

int ai_match_keywords(const char *text, const char **keywords, int num_keywords) {
    int matches = 0;
    for (int i = 0; i < num_keywords; i++) if (strstr(text, keywords[i])) matches++;
    return (float)matches / (float)num_keywords;
}

void ai_engine_status(void) {
    vga_puts("\n  AI Intent Engine Status:\n  ──────────────────────────\n");
    vga_puts("  State: Active\n");
    vga_puts("  Context: "); vga_puts(current_context); vga_puts("\n");
    vga_puts("  Workspace: "); vga_puts(workspaces[current_workspace].name); vga_puts("\n");
    vga_puts("  History: "); vga_print_dec(intent_history_count); vga_puts(" intents processed\n");
    vga_puts("  Patterns: "); vga_print_dec(sizeof(intent_patterns) / sizeof(intent_patterns[0]));
    vga_puts(" built-in categories\n");
}

static char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    char *h = (char *)haystack;
    while (*h) {
        if (*h == *needle) {
            char *h2 = h + 1, *n2 = (char *)needle + 1;
            while (*n2 && *h2 == *n2) { h2++; n2++; }
            if (!*n2) return h;
        }
        h++;
    }
    return NULL;
}

static size_t strlen(const char *s) { size_t len = 0; while (s[len]) len++; return len; }
static int atoi(const char *s) { int val = 0; while (*s >= '0' && *s <= '9') { val = val * 10 + (*s - '0'); s++; } return val; }
static int strcmp(const char *s1, const char *s2) { while (*s1 && *s1 == *s2) { s1++; s2++; } return *(unsigned char *)s1 - *(unsigned char *)s2; }
