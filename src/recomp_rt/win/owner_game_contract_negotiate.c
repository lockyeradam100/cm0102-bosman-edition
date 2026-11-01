/* Owner Game contract-screen refusal-path diagnostics.
 *
 * Build 19 keeps the proven Figo -> Aston Villa score experiment in place, but adds no new
 * contract-screen behaviour. Build 18 proved CONTRACT_MANAGER::will_negotiate_offer already
 * returns native 1 for the exact Basic-tab call, while the UI still takes the refusal-text path.
 *
 * The generated Basic-tab code has several later checks that can all jump to the same refusal
 * renderer. This wrapper records the caller registers, contract fields and caller-stack locals
 * immediately after the native will-negotiate decision, so we can identify which later check is
 * actually failing without guessing or overriding another variable.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>

#define OG_TEST_STAFF_ID          1730
#define OG_TEST_CLUB_ID           374
#define OG_BASIC_TAB_RETURN       0x004e0ac2u
#define OG_CONTRACT_STAFF_ID      0x00u
#define OG_CONTRACT_CLUB_ID       0x04u

static int og_negotiate_memory_ok(uint32_t p, uint32_t bytes)
{
    if (p < 0x10000u || p >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - p;
}

static uint32_t og_read_u32(uint32_t p)
{
    return og_negotiate_memory_ok(p, 4) ? rd32(g_mem, p) : 0u;
}

static uint8_t og_read_u8(uint32_t p)
{
    return og_negotiate_memory_ok(p, 1) ? g_mem[p] : 0u;
}

static void og_negotiate_write_event(int32_t staff_id, int32_t club_id,
                                     uint32_t return_address,
                                     int native_result,
                                     uint32_t entry_esp,
                                     uint32_t entry_ebx,
                                     uint32_t entry_ebp,
                                     uint32_t entry_edi,
                                     uint32_t contract)
{
    if (!win_user_dir[0]) return;

    char path[1200], tmp[1200];
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);

    /* At 0x4e0abd the caller has pushed edi and then CALL has pushed the return address.
     * Therefore the Basic-tab function's own ESP is entry_esp + 8. */
    uint32_t caller_esp = entry_esp + 8u;
    uint32_t context_ptr = og_read_u32(caller_esp + 0x88cu);

    uint32_t stack_18 = og_read_u32(caller_esp + 0x18u);
    uint32_t stack_20 = og_read_u32(caller_esp + 0x20u);
    uint32_t stack_3c = og_read_u32(caller_esp + 0x3cu);
    uint32_t stack_44_lo = og_read_u32(caller_esp + 0x44u);
    uint32_t stack_44_hi = og_read_u32(caller_esp + 0x48u);
    uint8_t stack_1e = og_read_u8(caller_esp + 0x1eu);

    uint32_t contract_8 = og_read_u32(contract + 0x08u);
    uint32_t contract_c = og_read_u32(contract + 0x0cu);
    uint8_t contract_18 = og_read_u8(contract + 0x18u);
    uint8_t contract_35 = og_read_u8(contract + 0x35u);
    uint8_t contract_4f = og_read_u8(contract + 0x4fu);
    uint8_t context_18 = context_ptr ? og_read_u8(context_ptr + 0x18u) : 0u;

    uint32_t const_968940_lo = og_read_u32(0x00968940u);
    uint32_t const_968940_hi = og_read_u32(0x00968944u);
    uint32_t const_9690a8_lo = og_read_u32(0x009690a8u);
    uint32_t const_9690a8_hi = og_read_u32(0x009690acu);

    snprintf(path, sizeof path, "%s/%s", win_user_dir,
             "owner-game-contract-branch-inputs.json");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);

    FILE *f = host_fopen(tmp, "wb");
    if (!f) return;

    fprintf(f,
            "{\n"
            "  \"bridge_version\": \"1.0-poc\",\n"
            "  \"event\": \"contract_refusal_branch_inputs\",\n"
            "  \"date\": \"%04d-%02d-%02d\",\n"
            "  \"staff_id\": %d,\n"
            "  \"staff_name\": \"Figo\",\n"
            "  \"human_club_id\": %d,\n"
            "  \"human_club_name\": \"Aston Villa\",\n"
            "  \"return_address\": \"0x%08x\",\n"
            "  \"native_will_negotiate\": %d,\n"
            "  \"behaviour_override_applied\": false,\n"
            "  \"entry_esp\": \"0x%08x\",\n"
            "  \"entry_ebx\": %u,\n"
            "  \"entry_ebp\": \"0x%08x\",\n"
            "  \"entry_edi\": \"0x%08x\",\n"
            "  \"contract_ptr\": \"0x%08x\",\n"
            "  \"contract_field_8\": %u,\n"
            "  \"contract_field_c\": %u,\n"
            "  \"contract_byte_18\": %u,\n"
            "  \"contract_byte_35\": %u,\n"
            "  \"contract_byte_4f\": %u,\n"
            "  \"caller_stack_18\": %u,\n"
            "  \"caller_stack_20_raw\": %u,\n"
            "  \"caller_stack_3c\": %u,\n"
            "  \"caller_stack_44_double_lo\": %u,\n"
            "  \"caller_stack_44_double_hi\": %u,\n"
            "  \"caller_stack_1e_byte\": %u,\n"
            "  \"caller_context_ptr\": \"0x%08x\",\n"
            "  \"caller_context_byte_18\": %u,\n"
            "  \"const_968940_double_lo\": %u,\n"
            "  \"const_968940_double_hi\": %u,\n"
            "  \"const_9690a8_double_lo\": %u,\n"
            "  \"const_9690a8_double_hi\": %u,\n"
            "  \"experiment\": \"observe_post_will_negotiate_refusal_inputs_only\"\n"
            "}\n",
            year, month, day, staff_id, club_id, return_address,
            native_result, entry_esp, entry_ebx, entry_ebp, entry_edi,
            contract, contract_8, contract_c, contract_18, contract_35,
            contract_4f, stack_18, stack_20, stack_3c, stack_44_lo,
            stack_44_hi, stack_1e, context_ptr, context_18,
            const_968940_lo, const_968940_hi, const_9690a8_lo, const_9690a8_hi);

    int ok = fflush(f) == 0;
    if (ok) host_sync(f);
    fclose(f);

    if (!ok || host_replace(tmp, path))
        host_unlink(tmp);
}

/* CONTRACT_MANAGER::will_negotiate_offer(this, STAFF_CONTRACT *offer), returns 1 or 0 in EAX. */
void o_004d4660(void);
RT_WRAP(004d4660, owner_game_contract_will_negotiate)
{
    uint32_t entry_esp = cpu.esp;
    uint32_t return_address = og_read_u32(entry_esp);
    uint32_t contract = rt_arg(0);

    uint32_t entry_ebx = cpu.ebx;
    uint32_t entry_ebp = cpu.ebp;
    uint32_t entry_edi = cpu.edi;

    int32_t staff_id = -1;
    int32_t club_id = -1;
    if (og_negotiate_memory_ok(contract, OG_CONTRACT_CLUB_ID + 4)) {
        staff_id = (int32_t)rd32(g_mem, contract + OG_CONTRACT_STAFF_ID);
        club_id = (int32_t)rd32(g_mem, contract + OG_CONTRACT_CLUB_ID);
    }

    int exact_test = return_address == OG_BASIC_TAB_RETURN &&
                     staff_id == OG_TEST_STAFF_ID &&
                     club_id == OG_TEST_CLUB_ID;

    o_004d4660();
    if (rt_unwinding || !exact_test) return;

    int native_result = cpu.eax ? 1 : 0;

    /* Build 19 is diagnostic only at this layer. Preserve CM's native boolean exactly. */
    og_negotiate_write_event(staff_id, club_id, return_address,
                             native_result, entry_esp, entry_ebx,
                             entry_ebp, entry_edi, contract);

    fprintf(stderr,
            "owner-game: contract branch trace: staff %d -> club %d caller=%#x native=%d\n",
            staff_id, club_id, return_address, native_result);
}
