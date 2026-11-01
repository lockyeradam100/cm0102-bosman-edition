/* Owner Game player-willingness experiment.
 *
 * Build 13 keeps Build 12's deliberately narrow Figo -> Aston Villa score override, and adds a
 * flight recorder around every matching call to CM01/02's native player-interest scorer. The aim
 * is observational: distinguish the transfer-willingness call from the later contract-screen call
 * by recording the original return address, emulated CPU registers and raw stack words.
 *
 * Nothing new is overridden in Build 13. The native reason bits are still left untouched and the
 * only behavioural change remains the proven Build 12 test pair: Luis Figo (staff 1730) considering
 * Aston Villa (club 374) receives an effective returned interest score of 1.0.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define OG_PLAYER_MANAGER       0x00b74c26u
#define OG_PLAYER_SLOT          0x00b77c7au
#define OG_STAFF_CLUB           0x39u
#define OG_STAFF_SECOND_NAME    0x08u
#define OG_STAFF_COMMON_NAME    0x0cu
#define OG_CLUB_NAME            0x04u
#define OG_CLUB_COUNT           0x00ae235cu
#define OG_CLUB_BASE_PTR        0x00ae23b4u
#define OG_STAFF_BASE_PTR       0x00ae23bcu
#define OG_CLUB_STRIDE          0x245u
#define OG_STAFF_STRIDE         0x6eu

/* TRANSFER_OFFER fields used by the original transfer code. */
#define OG_OFFER_BUYING_CLUB_ID 0x08u
#define OG_OFFER_STAFF_ID       0x0cu

/* Builds 12/13's intentionally exact test pair. */
#define OG_TEST_STAFF_ID        1730
#define OG_TEST_CLUB_ID         374
#define OG_TEST_INTEREST_SCORE  1.0
#define OG_TRACE_STACK_WORDS    12

typedef struct {
    int valid;
    int32_t staff_id;
    int32_t club_id;
    double native_score;
    double effective_score;
    uint32_t reason_bits;
    int score_override_applied;
    int check_wages;
    int mode;
    int squad_status;
} OgInterestObservation;

static OgInterestObservation og_interest;
static uint32_t og_trace_sequence;

static int og_memory_ok(uint32_t p, uint32_t bytes)
{
    if (p < 0x10000u || p >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - p;
}

static void og_copy_string(char *out, size_t room, uint32_t at)
{
    size_t n = 0;
    if (!room) return;
    if (!at || !og_memory_ok(at, 1)) {
        out[0] = 0;
        return;
    }
    while (n + 1 < room && n < 64 && og_memory_ok(at + (uint32_t)n, 1)) {
        unsigned char c = g_mem[at + (uint32_t)n];
        if (!c) break;
        if (c < 32 || c == '"' || c == '\\') c = ' ';
        out[n++] = (char)c;
    }
    out[n] = 0;
}

static void og_copy_staff_name(char *out, size_t room, uint32_t staff)
{
    if (!room) return;
    out[0] = 0;
    if (!og_memory_ok(staff, OG_STAFF_COMMON_NAME + 4)) return;
    uint32_t common = rd32(g_mem, staff + OG_STAFF_COMMON_NAME);
    uint32_t second = rd32(g_mem, staff + OG_STAFF_SECOND_NAME);
    uint32_t at = common && og_memory_ok(common, 1) && g_mem[common] ? common : second;
    og_copy_string(out, room, at);
}

static uint32_t og_current_human_club(int32_t *club_id)
{
    if (club_id) *club_id = -1;
    if (!g_mem) return 0;
    int slot = (int16_t)rd16(g_mem, OG_PLAYER_SLOT);
    if (slot < 0 || slot >= 16) return 0;
    uint32_t manager_at = OG_PLAYER_MANAGER + (uint32_t)slot * 0x300u;
    if (!og_memory_ok(manager_at, 4)) return 0;
    uint32_t manager = rd32(g_mem, manager_at);
    if (!og_memory_ok(manager, OG_STAFF_CLUB + 4)) return 0;
    uint32_t club = rd32(g_mem, manager + OG_STAFF_CLUB);
    if (!og_memory_ok(club, OG_CLUB_NAME + 4)) return 0;
    int32_t id = (int32_t)rd32(g_mem, club);
    int32_t count = (int32_t)rd32(g_mem, OG_CLUB_COUNT);
    if (count <= 0 || id < 0 || id >= count) return 0;
    if (club_id) *club_id = id;
    return club;
}

static uint32_t og_staff_from_id(int32_t id)
{
    if (id < 0 || id > 200000) return 0;
    uint32_t base = rd32(g_mem, OG_STAFF_BASE_PTR);
    uint64_t at64 = (uint64_t)base + (uint64_t)(uint32_t)id * OG_STAFF_STRIDE;
    if (at64 > 0xffffffffu) return 0;
    uint32_t at = (uint32_t)at64;
    return og_memory_ok(at, OG_STAFF_CLUB + 4) ? at : 0;
}

static int og_replace_text_file(const char *name, const char *text)
{
    if (!win_user_dir[0]) return 0;
    char path[1200], tmp[1200];
    snprintf(path, sizeof path, "%s/%s", win_user_dir, name);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = host_fopen(tmp, "wb");
    if (!f) return 0;
    int ok = fputs(text, f) >= 0 && fflush(f) == 0;
    if (ok) host_sync(f);
    fclose(f);
    if (!ok || host_replace(tmp, path)) {
        host_unlink(tmp);
        return 0;
    }
    return 1;
}

static uint32_t og_stack_word(uint32_t esp, uint32_t word)
{
    uint64_t at64 = (uint64_t)esp + (uint64_t)word * 4u;
    if (at64 > 0xffffffffu) return 0;
    uint32_t at = (uint32_t)at64;
    return og_memory_ok(at, 4) ? rd32(g_mem, at) : 0;
}

static void og_write_trace(uint32_t sequence, uint32_t staff, uint32_t club,
                           int32_t staff_id, int32_t club_id,
                           uint32_t entry_esp, const Cpu *entry_cpu,
                           const uint32_t *stack_words,
                           double native_score, double effective_score,
                           uint32_t reason_bits, int check_wages,
                           int mode, int squad_status)
{
    if (!win_user_dir[0]) return;
    char path[1200], staff_name[80], club_name[80];
    int year = 0, month = 0, day = 0;
    og_copy_staff_name(staff_name, sizeof staff_name, staff);
    og_copy_string(club_name, sizeof club_name, club + OG_CLUB_NAME);
    win_game_date(&year, &month, &day);
    snprintf(path, sizeof path, "%s/%s", win_user_dir, "owner-game-willingness-trace.jsonl");

    FILE *f = host_fopen(path, sequence == 1 ? "wb" : "ab");
    if (!f) return;
    fprintf(f,
            "{\"bridge_version\":\"0.7-poc\",\"event\":\"player_willingness_call_trace\","
            "\"sequence\":%u,\"date\":\"%04d-%02d-%02d\","
            "\"staff_id\":%d,\"staff_name\":\"%s\","
            "\"human_club_id\":%d,\"human_club_name\":\"%s\","
            "\"return_address\":\"0x%08x\",\"entry_esp\":\"0x%08x\","
            "\"entry_eax\":\"0x%08x\",\"entry_ecx\":\"0x%08x\","
            "\"entry_edx\":\"0x%08x\",\"entry_ebx\":\"0x%08x\","
            "\"entry_ebp\":\"0x%08x\",\"entry_esi\":\"0x%08x\","
            "\"entry_edi\":\"0x%08x\",\"raw_stack\":[",
            sequence, year, month, day, staff_id, staff_name,
            club_id, club_name, stack_words[0], entry_esp,
            entry_cpu->eax, entry_cpu->ecx, entry_cpu->edx, entry_cpu->ebx,
            entry_cpu->ebp, entry_cpu->esi, entry_cpu->edi);
    for (int i = 0; i < OG_TRACE_STACK_WORDS; i++)
        fprintf(f, "%s%u", i ? "," : "", stack_words[i]);
    fprintf(f,
            "],\"raw_arg0\":%u,\"raw_arg1\":%u,\"raw_arg2\":%u,"
            "\"raw_arg3\":%u,\"raw_arg4\":%u,\"raw_arg5\":%u,"
            "\"raw_arg6\":%u,\"raw_arg7\":%u,\"raw_arg8\":%u,"
            "\"native_interest_score\":%.6f,\"effective_interest_score\":%.6f,"
            "\"interest_reason_bits\":%u,\"check_wages\":%d,\"mode\":%d,"
            "\"squad_status_interpreted\":%d}\n",
            stack_words[1], stack_words[2], stack_words[3], stack_words[4],
            stack_words[5], stack_words[6], stack_words[7], stack_words[8], stack_words[9],
            native_score, effective_score, reason_bits, check_wages, mode, squad_status);
    fflush(f);
    host_sync(f);
    fclose(f);
}

static void og_write_score_event(uint32_t sequence, uint32_t return_address,
                                 uint32_t staff, uint32_t club,
                                 int32_t staff_id, int32_t club_id,
                                 double native_score, double effective_score,
                                 uint32_t reason_bits, int check_wages,
                                 int mode, int squad_status)
{
    char staff_name[80], club_name[80], text[2000];
    int year = 0, month = 0, day = 0;
    og_copy_staff_name(staff_name, sizeof staff_name, staff);
    og_copy_string(club_name, sizeof club_name, club + OG_CLUB_NAME);
    win_game_date(&year, &month, &day);

    snprintf(text, sizeof text,
             "{\n"
             "  \"bridge_version\": \"0.7-poc\",\n"
             "  \"event\": \"player_willingness_score_experiment\",\n"
             "  \"call_sequence\": %u,\n"
             "  \"return_address\": \"0x%08x\",\n"
             "  \"date\": \"%04d-%02d-%02d\",\n"
             "  \"staff_id\": %d,\n"
             "  \"staff_name\": \"%s\",\n"
             "  \"human_club_id\": %d,\n"
             "  \"human_club_name\": \"%s\",\n"
             "  \"native_interest_score\": %.6f,\n"
             "  \"effective_interest_score\": %.6f,\n"
             "  \"score_override_applied\": true,\n"
             "  \"interest_reason_bits\": %u,\n"
             "  \"check_wages\": %d,\n"
             "  \"mode\": %d,\n"
             "  \"squad_status_interpreted\": %d,\n"
             "  \"experiment\": \"score_only_reason_bits_unchanged_with_call_trace\"\n"
             "}\n",
             sequence, return_address,
             year, month, day, staff_id, staff_name, club_id, club_name,
             native_score, effective_score, reason_bits,
             check_wages, mode, squad_status);

    og_replace_text_file("owner-game-willingness-score.json", text);
}

static void og_write_rejection_event(uint32_t offer, int reason)
{
    if (!og_memory_ok(offer, OG_OFFER_STAFF_ID + 4)) return;

    int32_t buying_club_id = (int32_t)rd32(g_mem, offer + OG_OFFER_BUYING_CLUB_ID);
    int32_t staff_id = (int32_t)rd32(g_mem, offer + OG_OFFER_STAFF_ID);
    int32_t human_club_id = -1;
    uint32_t human_club = og_current_human_club(&human_club_id);
    if (!human_club || buying_club_id != human_club_id) return;

    uint32_t staff = og_staff_from_id(staff_id);
    char staff_name[80], club_name[80], text[2100];
    og_copy_staff_name(staff_name, sizeof staff_name, staff);
    og_copy_string(club_name, sizeof club_name, human_club + OG_CLUB_NAME);

    int matched = og_interest.valid && og_interest.staff_id == staff_id &&
                  og_interest.club_id == human_club_id;
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);

    snprintf(text, sizeof text,
             "{\n"
             "  \"bridge_version\": \"0.7-poc\",\n"
             "  \"event\": \"player_willingness_rejection\",\n"
             "  \"date\": \"%04d-%02d-%02d\",\n"
             "  \"staff_id\": %d,\n"
             "  \"staff_name\": \"%s\",\n"
             "  \"human_club_id\": %d,\n"
             "  \"human_club_name\": \"%s\",\n"
             "  \"rejection_reason_code\": %d,\n"
             "  \"interest_observed\": %s,\n"
             "  \"interest_score\": %.6f,\n"
             "  \"native_interest_score\": %.6f,\n"
             "  \"effective_interest_score\": %.6f,\n"
             "  \"score_override_applied\": %s,\n"
             "  \"interest_reason_bits\": %u,\n"
             "  \"check_wages\": %d,\n"
             "  \"mode\": %d,\n"
             "  \"squad_status_interpreted\": %d,\n"
             "  \"result\": \"cm_rejected_move_before_contract_terms\"\n"
             "}\n",
             year, month, day, staff_id, staff_name,
             human_club_id, club_name, reason,
             matched ? "true" : "false",
             matched ? og_interest.effective_score : 0.0,
             matched ? og_interest.native_score : 0.0,
             matched ? og_interest.effective_score : 0.0,
             matched && og_interest.score_override_applied ? "true" : "false",
             matched ? og_interest.reason_bits : 0u,
             matched ? og_interest.check_wages : -1,
             matched ? og_interest.mode : -1,
             matched ? og_interest.squad_status : -1);

    og_replace_text_file("owner-game-willingness-event.json", text);
    fprintf(stderr,
            "owner-game: CM rejected move: staff %d (%s) -> club %d (%s), reason=%d, interest=%s native=%.3f effective=%.3f override=%s bits=%u\n",
            staff_id, staff_name, human_club_id, club_name, reason,
            matched ? "matched" : "not-seen",
            matched ? og_interest.native_score : 0.0,
            matched ? og_interest.effective_score : 0.0,
            matched && og_interest.score_override_applied ? "yes" : "no",
            matched ? og_interest.reason_bits : 0u);
}

/* Native player/staff interest scorer.
 *
 * Build 13 deliberately records the raw emulated call frame before invoking the original. This lets us
 * establish whether the later contract-screen invocation really has the same parameter layout and, more
 * importantly, identifies the exact native caller by the return address at [esp]. */
void o_00846920(void);
RT_WRAP(00846920, owner_game_staff_interest)
{
    uint32_t entry_esp = cpu.esp;
    Cpu entry_cpu = cpu;
    uint32_t stack_words[OG_TRACE_STACK_WORDS];
    for (int i = 0; i < OG_TRACE_STACK_WORDS; i++)
        stack_words[i] = og_stack_word(entry_esp, (uint32_t)i);

    uint32_t staff = rt_arg(0);
    uint32_t club = rt_arg(1);
    int check_wages = (int32_t)rt_arg(2);
    int mode = (int8_t)rt_arg(3);
    int squad_status = (int32_t)rt_arg(4);
    uint32_t out_reasons = rt_arg(5);
    int32_t human_club_id = -1;
    uint32_t human_club = og_current_human_club(&human_club_id);
    int observe = human_club && club == human_club && og_memory_ok(staff, 4);
    int32_t staff_id = observe ? (int32_t)rd32(g_mem, staff) : -1;

    o_00846920();
    if (rt_unwinding || !observe) return;

    double native_score = fpu.st[fpu.top];
    uint32_t reason_bits = out_reasons && og_memory_ok(out_reasons, 4) ? rd32(g_mem, out_reasons) : 0u;
    int override = staff_id == OG_TEST_STAFF_ID && human_club_id == OG_TEST_CLUB_ID;

    if (override)
        fpu.st[fpu.top] = OG_TEST_INTEREST_SCORE;

    og_interest.valid = 1;
    og_interest.staff_id = staff_id;
    og_interest.club_id = human_club_id;
    og_interest.native_score = native_score;
    og_interest.effective_score = fpu.st[fpu.top];
    og_interest.reason_bits = reason_bits;
    og_interest.score_override_applied = override;
    og_interest.check_wages = check_wages;
    og_interest.mode = mode;
    og_interest.squad_status = squad_status;

    if (override) {
        uint32_t sequence = ++og_trace_sequence;
        uint32_t return_address = stack_words[0];
        og_write_trace(sequence, staff, human_club, staff_id, human_club_id,
                       entry_esp, &entry_cpu, stack_words,
                       native_score, og_interest.effective_score,
                       reason_bits, check_wages, mode, squad_status);
        og_write_score_event(sequence, return_address,
                             staff, human_club, staff_id, human_club_id,
                             native_score, og_interest.effective_score,
                             reason_bits, check_wages, mode, squad_status);
        fprintf(stderr,
                "owner-game: score-only trace #%u: staff %d -> club %d caller=%#x native=%.6f effective=%.6f bits=%u raw_status=%d\n",
                sequence, staff_id, human_club_id, return_address, native_score,
                og_interest.effective_score, reason_bits, squad_status);
    }
}

/* Exact CM event behind the headline "<player> rejects move to <club>".
 * thiscall(TRANSFER_MANAGER *this, TRANSFER_OFFER *offer, int reason). */
void o_008d8310(void);
RT_WRAP(008d8310, owner_game_player_rejects_move)
{
    uint32_t offer = rt_arg(0);
    int reason = (int32_t)rt_arg(1);
    o_008d8310();
    if (rt_unwinding) return;
    og_write_rejection_event(offer, reason);
}
