/* ============================================================
 *  MexaOS AI Engine
 * ============================================================ */

#ifndef MEXAOS_AI_ENGINE_H
#define MEXAOS_AI_ENGINE_H

#include <stdint.h>
#include <stddef.h>

/* ─── Constants ─── */
#define INTENT_MAX_LEN      256
#define INTENT_MAX_TOKENS   64
#define AI_CONFIDENCE_THRESHOLD 0.75f

/* ─── Intent Types ─── */
#define INTENT_UNKNOWN      0
#define INTENT_OPEN         1
#define INTENT_CREATE       2
#define INTENT_DELETE       3
#define INTENT_SEARCH       4
#define INTENT_RUN          5
#define INTENT_KILL         6
#define INTENT_CONFIG       7
#define INTENT_QUERY        8

/* ─── Intent Structure ─── */
struct intent {
    uint32_t type;
    float confidence;
    char text[INTENT_MAX_LEN];
    char target[INTENT_MAX_LEN];
    uint32_t param_count;
    char params[INTENT_MAX_TOKENS][INTENT_MAX_LEN];
};

/* ─── AI Engine Functions ─── */
void ai_engine_init(void);
void ai_engine_shutdown(void);

/* ─── Intent Processing ─── */
struct intent *ai_parse_intent(const char *text);
float ai_score_intent(const char *text, uint32_t intent_type);
const char *ai_intent_name(uint32_t intent_type);

/* ─── Natural Language ─── */
char *ai_suggest_completion(const char *partial);
char *ai_generate_response(const char *query);

/* ─── Learning ─── */
void ai_learn_from_action(const struct intent *intent, int success);
void ai_save_model(void);
void ai_load_model(void);

#endif
