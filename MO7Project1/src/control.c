#include <string.h>
#include <stdio.h>
#include "control.h"
#include "command_parser.h"
#include "uart.h"
#include "uart_ext.h"
#include "cheby2.h"
#include "gpio_driver.h"
#include "logger.h"

const double gain_linear[GAIN_STEPS] = {
    0.31623, 0.35481, 0.39811, 0.44668, 0.50119,
    0.56234, 0.63096, 0.70795, 0.79433, 0.89125,
    1.00000,
    1.12202, 1.25893, 1.41254, 1.58489, 1.77828,
    1.99526, 2.23872, 2.51189, 2.81838, 3.16228
};

/*
 * CALIB_OFFSET — passband-gain correction divisor applied to filtered output.
 *
 * Set to 1.0 (no correction) by default: the original mo7 Chebyshev ran with
 * no calibration, so this preserves known-good behavior.
 *
 * To calibrate: play a tone in the passband, compare filtered vs bypass output
 * level, and set CALIB_OFFSET = (filtered_level / bypass_level). Then filtered
 * output is divided back down to match bypass.
 *   e.g. Ricard's filter used 1.99526 * 1.04713 ≈ 2.089
 */
static const double CALIB_OFFSET = 1.0;

SystemState g_state = {
    .cheby_active    = 0,
    .volume          = 80,
    .muted           = 0,
    .filter_gain_idx = GAIN_IDX_UNITY,
    .master_gain_idx = GAIN_IDX_UNITY
};

static Cheby2State cheby_l;
static Cheby2State cheby_r;

void control_init(void)
{
    cheby2_reset(&cheby_l);
    cheby2_reset(&cheby_r);
    LOG_INFO("Control ready. cheby=%d V=%d%% FG=%d MG=%d",
             g_state.cheby_active, g_state.volume,
             g_state.filter_gain_idx, g_state.master_gain_idx);
}

void control_set_filter(int active)
{
    g_state.cheby_active = (active != 0) ? 1 : 0;
    cheby2_reset(&cheby_l);
    cheby2_reset(&cheby_r);
}

void control_set_volume(int value)
{
    if (value < 0)   value = 0;
    if (value > 100) value = 100;
    g_state.volume = value;
}

void control_mute(void)   { g_state.muted = 1; }
void control_unmute(void) { g_state.muted = 0; }

void control_fgain_up(void)
{
    if (g_state.filter_gain_idx < GAIN_STEPS - 1)
        g_state.filter_gain_idx++;
    gpio_set_leds(LED_FGAIN_UP);
    LOG_INFO("FilterGain idx=%d", g_state.filter_gain_idx);
}

void control_fgain_down(void)
{
    if (g_state.filter_gain_idx > 0)
        g_state.filter_gain_idx--;
    gpio_set_leds(LED_FGAIN_DOWN);
    LOG_INFO("FilterGain idx=%d", g_state.filter_gain_idx);
}

void control_mgain_up(void)
{
    if (g_state.master_gain_idx < GAIN_STEPS - 1)
        g_state.master_gain_idx++;
    gpio_set_leds(LED_MGAIN_UP);
    LOG_INFO("MasterGain idx=%d", g_state.master_gain_idx);
}

void control_mgain_down(void)
{
    if (g_state.master_gain_idx > 0)
        g_state.master_gain_idx--;
    gpio_set_leds(LED_MGAIN_DOWN);
    LOG_INFO("MasterGain idx=%d", g_state.master_gain_idx);
}

#define AUDIO_SCALE_D ((double)(1 << 23))

void control_process_audio(int in_l, int in_r, int *out_l, int *out_r)
{
    double y_l = (double)in_l / AUDIO_SCALE_D;
    double y_r = (double)in_r / AUDIO_SCALE_D;

    if (g_state.cheby_active) {
        y_l = cheby2_process(y_l, &cheby_l);
        y_r = cheby2_process(y_r, &cheby_r);
        y_l /= CALIB_OFFSET;
        y_r /= CALIB_OFFSET;
    }

    double fg = gain_linear[g_state.filter_gain_idx];
    y_l *= fg;
    y_r *= fg;

    if (g_state.muted) {
        y_l = 0.0;
        y_r = 0.0;
    } else {
        double vol = (double)g_state.volume / 100.0;
        y_l *= vol;
        y_r *= vol;
    }

    double mg = gain_linear[g_state.master_gain_idx];
    y_l *= mg;
    y_r *= mg;

    double ld = y_l * AUDIO_SCALE_D;
    double rd = y_r * AUDIO_SCALE_D;

    if (ld >  8388607.0) ld =  8388607.0;
    if (ld < -8388608.0) ld = -8388608.0;
    if (rd >  8388607.0) rd =  8388607.0;
    if (rd < -8388608.0) rd = -8388608.0;

    *out_l = (int)ld;
    *out_r = (int)rd;
}

void control_send_status(void)
{
    char buf[80];
    snprintf(buf, sizeof(buf),
             "STATUS F%d V%d M%d FG%d MG%d\r\n",
             g_state.cheby_active, g_state.volume, g_state.muted,
             g_state.filter_gain_idx, g_state.master_gain_idx);
    uart_write_string(buf);
}

void control_process_command(const char *cmd)
{
    ParsedCommand pc = command_parse(cmd);

    switch (pc.type) {
    case CMD_FILTER_BYPASS:
        control_set_filter(0);
        LOG_INFO("Filter -> BYPASS");
        break;
    case CMD_FILTER_CHEBY:
        control_set_filter(1);
        LOG_INFO("Filter -> CHEBYSHEV_II");
        break;
    case CMD_VOLUME:
        control_set_volume(pc.value);
        LOG_INFO("Volume -> %d%%", g_state.volume);
        break;
    case CMD_MUTE:
        control_mute();
        LOG_INFO("Muted");
        break;
    case CMD_UNMUTE:
        control_unmute();
        LOG_INFO("Unmuted");
        break;
    case CMD_FGAIN_UP:
        control_fgain_up();
        break;
    case CMD_FGAIN_DOWN:
        control_fgain_down();
        break;
    case CMD_MGAIN_UP:
        control_mgain_up();
        break;
    case CMD_MGAIN_DOWN:
        control_mgain_down();
        break;
    default:
        LOG_ERROR("Unknown cmd: %s", cmd);
        break;
    }
}

void control_inject_command(const char *cmd)
{
    LOG_DEBUG("Inject: %s", cmd);
    control_process_command(cmd);
}
