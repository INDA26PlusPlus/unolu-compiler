
#ifndef TAP_H
#define TAP_H

#include <stdlib.h>

// Guard if TOSTRING is needed later in another file, then we move it to a seperate preprocessing file 
#ifdef TOSTRING
#warning "TOSTRING already defined, most likely 2 seperate defs. Consider making them 1"
#endif
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// You can include a plan comment as an optional arg
#define TAP_INIT(num_tests, ...) printf("TAP version 14\n1.." TOSTRING(num_tests) __VA_OPT__(" # " __VA_ARGS__) "\n"); 

#define INTERNAL_TAP_TP_DECL(tp_id, description, skip, skip_reason, todo, todo_explanation) \
__label__ INTERNAL_tap_tp_end_label; \
const unsigned long long INTERNAL_tap_tp_id = tp_id; \
static const char* const INTERNAL_tap_tp_description = description; \
static const char* const INTERNAL_tap_tp_skip_reason = skip_reason; \
static const char* const INTERNAL_tap_tp_todo_explanation = todo_explanation; \
int INTERNAL_tap_tp_skip = skip; \
int INTERNAL_tap_tp_todo = todo; \
int INTERNAL_tap_tp_success = 1;


#define TAP_TP_BEGIN(tp_id, description) \
{ INTERNAL_TAP_TP_DECL(tp_id, description, 0, "", 0, "") 

// Optionally you can add the reason for skipping
#define TAP_TP_BEGIN_SKIPPABLE(tp_id, description, skip, ...) \
{ INTERNAL_TAP_TP_DECL(tp_id, description, skip, "" __VA_OPT__(__VA_ARGS__), 0, "") \
if (INTERNAL_tap_tp_skip) {goto INTERNAL_tap_tp_end_label;}

#define TAP_TP_BEGIN_UNIMPLEMENTED(tp_id, description, explanation) \
{ INTERNAL_TAP_TP_DECL(tp_id, description, 0, "", 1, explanation) \
INTERNAL_tap_tp_success = 0; \
goto INTERNAL_tap_tp_end_label; 


#define TAP_TP_END() \
INTERNAL_tap_tp_end_label: \
if (INTERNAL_tap_tp_success) {fputs("ok", stdout);} \
else {fputs("not ok", stdout);} \
printf(" %lld", INTERNAL_tap_tp_id); \
if (INTERNAL_tap_tp_description[0] != '\0') {fputs(" - ", stdout); fputs(INTERNAL_tap_tp_description, stdout);} \
if (INTERNAL_tap_tp_skip) { fputs(" # SKIP ", stdout); if (INTERNAL_tap_tp_skip_reason[0] != '\0') {fputs(INTERNAL_tap_tp_skip_reason, stdout);}} \
else if (INTERNAL_tap_tp_todo) { fputs(" # TODO ", stdout); fputs(INTERNAL_tap_tp_todo_explanation, stdout);} \
fputc('\n', stdout); \
if (0) {goto INTERNAL_tap_tp_end_label;} \
}
// (goto is to shut up the compiler)

#define TAP_TP_FAIL() INTERNAL_tap_tp_success = 0; goto INTERNAL_tap_tp_end_label;

#define TAP_TP_SUCCEED() goto INTERNAL_tap_tp_end_label; 

#define TAP_TP_COMMENT(str, ...) fputs("# ", stdout);printf(str, ##__VA_ARGS__);fputc('\n', stdout);

#define TAP_BAILOUT(bailout_message) { fputs("Bail out! " bailout_message "\n"); exit(EXIT_FAILURE); }

// Optionally include description
#define TAP_ASSERT(expr, tp_id, ...) \
{ \
if (expr) {fputs("ok", stdout);} \
else {fputs("not ok", stdout);} \
printf(" %d", tp_id); \
__VA_OPT__(fputs(" - " __VA_ARGS__, stdout);) \
fputc('\n', stdout); \
}
// Optionally include description
#define TAP_ASSERT_BAIL(expr, tp_id, bailout_message, ...) \
{ \
if (expr) { fputs("ok", stdout); __VA_OPT__(fputs(" - " __VA_ARGS__, stdout);) fputc('\n', stdout);} \
else {TAP_BAILOUT(bailout_message);} \
fputc('\n', stdout); \
}





#endif
