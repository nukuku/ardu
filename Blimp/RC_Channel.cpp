#include "Blimp.h"

#include "RC_Channel.h"


// defining these two macros and including the RC_Channels_VarInfo header defines the parameter information common to all vehicle types
#define RC_CHANNELS_SUBCLASS RC_Channels_Blimp
#define RC_CHANNEL_SUBCLASS RC_Channel_Blimp

#include <RC_Channel/RC_Channels_VarInfo.h>

int8_t RC_Channels_Blimp::flight_mode_channel_number() const
{
    return blimp.g.flight_mode_chan.get();
}

void RC_Channel_Blimp::mode_switch_changed(modeswitch_pos_t new_pos)
{
    if (new_pos < 0 || (uint8_t)new_pos > blimp.num_flight_modes) {
        // should not have been called
        return;
    }

    if (!blimp.set_mode((Mode::Number)blimp.flight_modes[new_pos].get(), ModeReason::RC_COMMAND)) {
        // alert user to mode change failure
        if (blimp.ap.initialised) {
            AP_Notify::events.user_mode_change_failed = 1;
        }
        return;
    }

    // play a tone
    // alert user to mode change (except if autopilot is just starting up)
    if (blimp.ap.initialised) {
        AP_Notify::events.user_mode_change = 1;
    }
}

bool RC_Channels_Blimp::has_valid_input() const
{
    if (blimp.failsafe.radio) {
        return false;
    }
    if (blimp.failsafe.radio_counter != 0) {
        return false;
    }
    return true;
}

RC_Channel * RC_Channels_Blimp::get_arming_channel(void) const
{
    return blimp.channel_yaw;
}

// init_aux_switch_function - initialize aux functions
void RC_Channel_Blimp::init_aux_function(const aux_func_t ch_option, const AuxSwitchPos ch_flag)
{
    // init channel options
    switch (ch_option) {
    // the following functions do not need to be initialised:
    case AUX_FUNC::MANUAL:
        break;
    default:
        RC_Channel::init_aux_function(ch_option, ch_flag);
        break;
    }
}

// do_aux_function_change_mode - change mode based on an aux switch
// being moved
void RC_Channel_Blimp::do_aux_function_change_mode(const Mode::Number mode,
        const AuxSwitchPos ch_flag)
{
    switch (ch_flag) {
    case AuxSwitchPos::HIGH: {
        // engage mode (if not possible we remain in current flight mode)
        const bool success = blimp.set_mode(mode, ModeReason::RC_COMMAND);
        if (blimp.ap.initialised) {
            if (success) {
                AP_Notify::events.user_mode_change = 1;
            } else {
                AP_Notify::events.user_mode_change_failed = 1;
            }
        }
        break;
    }
    default:
        // return to flight mode switch's flight mode if we are currently
        // in this mode
        if (blimp.control_mode == mode) {
            rc().reset_mode_switch();
        }
    }
}

void RC_Channel_Blimp::do_aux_function_armdisarm(const AuxSwitchPos ch_flag)
{
    RC_Channel::do_aux_function_armdisarm(ch_flag);
    if (blimp.arming.is_armed()) {
        // remember that we are using an arming switch, for use by
        // set_throttle_zero_flag
        blimp.ap.armed_with_switch = true;
    }
}

// do_aux_function - implement the function invoked by auxiliary switches
void RC_Channel_Blimp::do_aux_function(const aux_func_t ch_option, const AuxSwitchPos ch_flag)
{
    switch (ch_option) {

    case AUX_FUNC::SAVE_TRIM:
        if ((ch_flag == AuxSwitchPos::HIGH) &&
            (blimp.control_mode <= Mode::Number::MANUAL) &&
            (blimp.channel_down->get_control_in() == 0)) {
            blimp.save_trim();
        }
        break;

    //         case AUX_FUNC::SAVE_WP:
    // #if MODE_AUTO_ENABLED == ENABLED
    //             // save waypoint when switch is brought high
    //             if (ch_flag == RC_Channel::AuxSwitchPos::HIGH) {

    //                 // do not allow saving new waypoints while we're in auto or disarmed
    //                 if (blimp.control_mode == Mode::Number::AUTO || !blimp.motors->armed()) {
    //                     return;
    //                 }

    //                 // do not allow saving the first waypoint with zero throttle
    //                 if ((blimp.mode_auto.mission.num_commands() == 0) && (blimp.channel_down->get_control_in() == 0)) {
    //                     return;
    //                 }

    //                 // create new mission command
    //                 AP_Mission::Mission_Command cmd  = {};

    //                 // if the mission is empty save a takeoff command
    //                 if (blimp.mode_auto.mission.num_commands() == 0) {
    //                     // set our location ID to 16, MAV_CMD_NAV_WAYPOINT
    //                     cmd.id = MAV_CMD_NAV_TAKEOFF;
    //                     cmd.content.location.alt = MAX(blimp.current_loc.alt,100);

    //                     // use the current altitude for the target alt for takeoff.
    //                     // only altitude will matter to the AP mission script for takeoff.
    //                     if (blimp.mode_auto.mission.add_cmd(cmd)) {
    //                         // log event
    //                         AP::logger().Write_Event(LogEvent::SAVEWP_ADD_WP);
    //                     }
    //                 }

    //                 // set new waypoint to current location
    //                 cmd.content.location = blimp.current_loc;

    //                 // if throttle is above zero, create waypoint command
    //                 if (blimp.channel_down->get_control_in() > 0) {
    //                     cmd.id = MAV_CMD_NAV_WAYPOINT;
    //                 } else {
    //                     // with zero throttle, create LAND command
    //                     cmd.id = MAV_CMD_NAV_LAND;
    //                 }

    //                 // save command
    //                 if (blimp.mode_auto.mission.add_cmd(cmd)) {
    //                     // log event
    //                     AP::logger().Write_Event(LogEvent::SAVEWP_ADD_WP);
    //                 }
    //             }
    // #endif
    //              break;

    //         case AUX_FUNC::AUTO:
    // #if MODE_AUTO_ENABLED == ENABLED
    //             do_aux_function_change_mode(Mode::Number::AUTO, ch_flag);
    // #endif
    //             break;

    // case AUX_FUNC::LOITER:
    //     do_aux_function_change_mode(Mode::Number::LOITER, ch_flag);
    //     break;

    case AUX_FUNC::MANUAL:
        do_aux_function_change_mode(Mode::Number::MANUAL, ch_flag);
        break;

    default:
        RC_Channel::do_aux_function(ch_option, ch_flag);
        break;
    }
}

// save_trim - adds roll and pitch trims from the radio to ahrs
void Blimp::save_trim()
{
    // save roll and pitch trim
    float roll_trim = ToRad((float)channel_right->get_control_in()/100.0f);
    float pitch_trim = ToRad((float)channel_front->get_control_in()/100.0f);
    ahrs.add_trim(roll_trim, pitch_trim);
    AP::logger().Write_Event(LogEvent::SAVE_TRIM);
    gcs().send_text(MAV_SEVERITY_INFO, "Trim saved");
}

// auto_trim - slightly adjusts the ahrs.roll_trim and ahrs.pitch_trim towards the current stick positions
// meant to be called continuously while the pilot attempts to keep the blimp level
void Blimp::auto_trim_cancel()
{
    auto_trim_counter = 0;
    AP_Notify::flags.save_trim = false;
    gcs().send_text(MAV_SEVERITY_INFO, "AutoTrim cancelled");
}

void Blimp::auto_trim()
{
    if (auto_trim_counter > 0) {
        if (blimp.flightmode != &blimp.mode_manual ||
            !blimp.motors->armed()) {
            auto_trim_cancel();
            return;
        }

        // flash the leds
        AP_Notify::flags.save_trim = true;

        if (!auto_trim_started) {
            if (ap.land_complete) {
                // haven't taken off yet
                return;
            }
            auto_trim_started = true;
        }

        if (ap.land_complete) {
            // landed again.
            auto_trim_cancel();
            return;
        }

        auto_trim_counter--;

        // calculate roll trim adjustment
        float roll_trim_adjustment = ToRad((float)channel_right->get_control_in() / 4000.0f);

        // calculate pitch trim adjustment
        float pitch_trim_adjustment = ToRad((float)channel_front->get_control_in() / 4000.0f);

        // add trim to ahrs object
        // save to eeprom on last iteration
        ahrs.add_trim(roll_trim_adjustment, pitch_trim_adjustment, (auto_trim_counter == 0));

        // on last iteration restore leds and accel gains to normal
        if (auto_trim_counter == 0) {
            AP_Notify::flags.save_trim = false;
            gcs().send_text(MAV_SEVERITY_INFO, "AutoTrim: Trims saved");
        }
    }
}