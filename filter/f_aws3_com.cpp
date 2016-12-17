#include "stdafx.h"
// Copyright(c) 2016 Yohei Matsumoto, All right reserved. 

// f_aws3_com.cpp is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// f_aws3_com.cpp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with f_aws3_com.cpp.  If not, see <http://www.gnu.org/licenses/>. 


#include <cstdio>
#include <cstring>
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <list>
#include <thread>
#include <mutex>

using namespace std;

#include "../util/aws_sock.h"
#include "../util/aws_stdlib.h"
#include "../util/aws_thread.h"
#include "../util/c_clock.h"

#include "f_aws3_com.h"

f_aws3_com::f_aws3_com(const char * name) :f_base(name), m_port(14550), m_sys_id(255), max_retry_load_param(10), m_bcon(false)
{
	create_param(k_param_surface_depth, "SURFACE_DEPTH", "Depth reading at surface", &surface_depth);
	create_param(k_param_format_version, "SYSID_SW_MREV", "Eeprom format version number.", &format_version);
	create_param(k_param_software_type, "SYSID_SW_TYPE", "Software Type.", &software_type);
	create_param(k_param_sysid_this_mav, "SYSID_THISMAV", "Mavlink system ID of this vehicle", &sysid_this_mav);
	create_param(k_param_sysid_my_gcs, "SYSID_MYGCS", "My ground station number", &sysid_my_gcs);
#if CLI_ENABLED == ENABLED
	create_param(k_param_cli_enabled, "CLI_ENABLED", "CLI Enabled", &cli_enabled);
#endif
	create_param(k_param_throttle_filt, "PILOT_THR_FILT", "Throttle filter cutoff", &throttle_filt);
	//create_param(k_param_serial_manager, "SERIAL", "")
	create_param(k_param_gcs_pid_mask, "GCS_PID_MASK", "GCS PID tuning mask", &gcs_pid_mask);
	create_param(k_param_rangefinder_gain, "RNGFND_GAIN", "Rangefinder gain", &rangefinder_gain);
	create_param(k_param_failsafe_battery_enabled, "FS_BATT_ENABLED", "Battery Failsafe Enable", &failsafe_battery_enabled);
	create_param(k_param_fs_batt_voltage, "FS_BATT_VOLTAGE", "Failsafe battery voltage", &fs_batt_voltage);
	create_param(k_param_fs_batt_mah, "FS_BATT_MAH", "Failsafe battery millAmpHours", &fs_batt_mah);
	create_param(k_param_failsafe_gcs, "FS_GCS_ENABLE", "Ground Station Failsafe Enable", &failsafe_gcs);
	create_param(k_param_failsafe_leak, "FS_LEAK_ENABLE", "Leak Failsafe Enable", &failsafe_leak);
	create_param(k_param_failsafe_pressure, "FS_PRESS_ENABLE", "Internal Pressure Failsafe Enable", &failsafe_pressure);
	create_param(k_param_failsafe_temperature, "FS_TEMP_ENABLE", "Internal Temprature Failsafe Enable", &failsafe_temperature);
	create_param(k_param_failsafe_pressure_max, "FS_PRESS_MAX", "Internal Pressure Failsafe Threshold", &failsafe_pressure_max);
	create_param(k_param_failsafe_temperature_max, "FS_TEMP_MAX", "Internal Temperature Failsafe Threshold", &failsafe_temperature_max);
	create_param(k_param_failsafe_terrain, "FS_TERRAIN_ENABLE", "Terrain Failsafe Enable", &failsafe_terrain);
	create_param(k_param_xtrack_angle_limit, "XTRACK_ANG_LIM", "Crosstrack correction angle limit", &xtrack_angle_limit);
	create_param(k_param_gps_hdop_good, "GPS_HDOP_GOOD", "GPS Hdop Good", &gps_hdop_good);
	create_param(k_param_compass_enabled, "MAG_ENABLE", "Compass enable/disable", &compass_enabled);
	create_param(k_param_wp_yaw_behavior, "WP_YAW_BEHAVIOR", "Yaw behaviour during missions", &wp_yaw_behavior);
	create_param(k_param_pilot_velocity_z_max, "PILOT_VELZ_MAX", "Pilot maximum vertical speed", &pilot_velocity_z_max);
	create_param(k_param_pilot_accel_z, "PILOT_ACCEL_Z", "Pilot vertical acceleration", &pilot_accel_z);
	create_param(k_param_failsafe_throttle, "FS_THR_ENABLE", "Throttle Failsafe Enable", &failsafe_throttle);
	create_param(k_param_failsafe_throttle_value, "FS_THR_VALUE", "Throttle Failsafe Value", &failsafe_throttle_value);
	create_param(k_param_throttle_deadzone, "THR_DZ", "Throttle deadzone", &throttle_deadzone);
	create_param(k_param_flight_mode1, "FLTMODE1", "Flight Mode 1", &flight_mode1);
	create_param(k_param_flight_mode2, "FLTMODE2", "Flight Mode 2", &flight_mode1);
	create_param(k_param_flight_mode3, "FLTMODE3", "Flight Mode 3", &flight_mode1);
	create_param(k_param_flight_mode4, "FLTMODE4", "Flight Mode 4", &flight_mode1);
	create_param(k_param_flight_mode5, "FLTMODE5", "Flight Mode 5", &flight_mode1);
	create_param(k_param_flight_mode6, "FLTMODE6", "Flight Mode 6", &flight_mode1);
	create_param(k_param_log_bitmask, "LOG_BITMASK", "Log bitmask", &log_bitmask);
	create_param(k_param_esc_calibrate, "ESC_CALIBRATION", "ESC Calibration", &esc_calibrate);
#if CH6_TUNE_ENABLED == ENABLED
	create_param(k_param_radio_tuning, "TUNE", "Channel 6 Tuning", &radio_tuning);
	create_param(k_param_radio_tuning_low, "TUNE_LOW", "Tuning minimum", &radio_tuning_low);
	create_param(k_param_radio_tuning_high, "TUNE_HIGH", "Tuning maximum", &radio_tuning_high);

#endif
#if AUXSW_ENABLED == ENABLED
	create_param(k_param_ch7_option, "CH7_OPT", "Channel 7 optioin", &ch7_option);
	create_param(k_param_ch8_option, "CH8_OPT", "Channel 8 optioin", &ch8_option);
	create_param(k_param_ch9_option, "CH9_OPT", "Channel 9 optioin", &ch9_option);
	create_param(k_param_ch7_option, "CH10_OPT", "Channel 10 optioin", &ch10_option);
	create_param(k_param_ch8_option, "CH11_OPT", "Channel 11 optioin", &ch11_option);
	create_param(k_param_ch9_option, "CH12_OPT", "Channel 12 optioin", &ch12_option);
#endif
	create_param(k_param_arming_check, "ARMING_CHECK", "Arming check", &arming_check);
	create_param(k_param_disarm_delay, "DISARM_DELAY", "Disarm delay", &disarm_delay);
	create_param(k_param_angle_max, "ANGLE_MAX", "Angle Max", &angle_max);
	create_param(k_param_rc_feel_rp, "RC_FEEL_RP", "RC Feel Roll/Pitch", &rc_feel_rp);
	create_param(k_param_fs_ekf_action, "FS_EKF_ACTION", "EKF Failsafe Action", &fs_ekf_action);
	create_param(k_param_fs_ekf_thresh, "FS_EKF_THRESH", "EKF failsafe variance theshold", &fs_ekf_thresh);
	create_param(k_param_fs_crash_check, "FS_CRASH_CHECK", "Crash check enable", &fs_crash_check);
	create_param(k_param_rc_1, "RC1_DZ", "RC Channel 1 deadzone", &rc_1.dz);
	create_param(k_param_rc_2, "RC2_DZ", "RC Channel 2 deadzone", &rc_2.dz);
	create_param(k_param_rc_3, "RC3_DZ", "RC Channel 3 deadzone", &rc_3.dz);
	create_param(k_param_rc_4, "RC4_DZ", "RC Channel 4 deadzone", &rc_4.dz);
	create_param(k_param_rc_5, "RC5_DZ", "RC Channel 5 deadzone", &rc_5.dz);
	create_param(k_param_rc_6, "RC6_DZ", "RC Channel 6 deadzone", &rc_6.dz);
	create_param(k_param_rc_7, "RC7_DZ", "RC Channel 7 deadzone", &rc_7.dz);
	create_param(k_param_rc_8, "RC8_DZ", "RC Channel 8 deadzone", &rc_8.dz);
	create_param(k_param_rc_9, "RC9_DZ", "RC Channel 9 deadzone", &rc_9.dz);
	create_param(k_param_rc_10, "RC10_DZ", "RC Channel 10 deadzone", &rc_10.dz);
	create_param(k_param_rc_11, "RC11_DZ", "RC Channel 11 deadzone", &rc_11.dz);
	create_param(k_param_rc_12, "RC12_DZ", "RC Channel 12 deadzone", &rc_12.dz);
	create_param(k_param_rc_13, "RC13_DZ", "RC Channel 13 deadzone", &rc_13.dz);
	create_param(k_param_rc_14, "RC14_DZ", "RC Channel 14 deadzone", &rc_14.dz);
	create_param(k_param_rc_1, "RC1_MAX", "RC Channel 1 maximum", &rc_1.max);
	create_param(k_param_rc_2, "RC2_MAX", "RC Channel 2 maximum", &rc_2.max);
	create_param(k_param_rc_3, "RC3_MAX", "RC Channel 3 maximum", &rc_3.max);
	create_param(k_param_rc_4, "RC4_MAX", "RC Channel 4 maximum", &rc_4.max);
	create_param(k_param_rc_5, "RC5_MAX", "RC Channel 5 maximum", &rc_5.max);
	create_param(k_param_rc_6, "RC6_MAX", "RC Channel 6 maximum", &rc_6.max);
	create_param(k_param_rc_7, "RC7_MAX", "RC Channel 7 maximum", &rc_7.max);
	create_param(k_param_rc_8, "RC8_MAX", "RC Channel 8 maximum", &rc_8.max);
	create_param(k_param_rc_9, "RC9_MAX", "RC Channel 9 maximum", &rc_9.max);
	create_param(k_param_rc_10, "RC10_MAX", "RC Channel 10 maximum", &rc_10.max);
	create_param(k_param_rc_11, "RC11_MAX", "RC Channel 11 maximum", &rc_11.max);
	create_param(k_param_rc_12, "RC12_MAX", "RC Channel 12 maximum", &rc_12.max);
	create_param(k_param_rc_13, "RC13_MAX", "RC Channel 13 maximum", &rc_13.max);
	create_param(k_param_rc_14, "RC14_MAX", "RC Channel 14 maximum", &rc_14.max);
	create_param(k_param_rc_1, "RC1_MIN", "RC Channel 1 minimum", &rc_1.min);
	create_param(k_param_rc_2, "RC2_MIN", "RC Channel 2 minimum", &rc_2.min);
	create_param(k_param_rc_3, "RC3_MIN", "RC Channel 3 minimum", &rc_3.min);
	create_param(k_param_rc_4, "RC4_MIN", "RC Channel 4 minimum", &rc_4.min);
	create_param(k_param_rc_5, "RC5_MIN", "RC Channel 5 minimum", &rc_5.min);
	create_param(k_param_rc_6, "RC6_MIN", "RC Channel 6 minimum", &rc_6.min);
	create_param(k_param_rc_7, "RC7_MIN", "RC Channel 7 minimum", &rc_7.min);
	create_param(k_param_rc_8, "RC8_MIN", "RC Channel 8 minimum", &rc_8.min);
	create_param(k_param_rc_9, "RC9_MIN", "RC Channel 9 minimum", &rc_9.min);
	create_param(k_param_rc_10, "RC10_MIN", "RC Channel 10 minimum", &rc_10.min);
	create_param(k_param_rc_11, "RC11_MIN", "RC Channel 11 minimum", &rc_11.min);
	create_param(k_param_rc_12, "RC12_MIN", "RC Channel 12 minimum", &rc_12.min);
	create_param(k_param_rc_13, "RC13_MIN", "RC Channel 13 minimum", &rc_13.min);
	create_param(k_param_rc_14, "RC14_MIN", "RC Channel 14 minimum", &rc_14.min);
	create_param(k_param_rc_1, "RC1_REV", "RC Channel 1 rev", &rc_1.rev);
	create_param(k_param_rc_2, "RC2_REV", "RC Channel 2 rev", &rc_2.rev);
	create_param(k_param_rc_3, "RC3_REV", "RC Channel 3 rev", &rc_3.rev);
	create_param(k_param_rc_4, "RC4_REV", "RC Channel 4 rev", &rc_4.rev);
	create_param(k_param_rc_5, "RC5_REV", "RC Channel 5 rev", &rc_5.rev);
	create_param(k_param_rc_6, "RC6_REV", "RC Channel 6 rev", &rc_6.rev);
	create_param(k_param_rc_7, "RC7_REV", "RC Channel 7 rev", &rc_7.rev);
	create_param(k_param_rc_8, "RC8_REV", "RC Channel 8 rev", &rc_8.rev);
	create_param(k_param_rc_9, "RC9_REV", "RC Channel 9 rev", &rc_9.rev);
	create_param(k_param_rc_10, "RC10_REV", "RC Channel 10 rev", &rc_10.rev);
	create_param(k_param_rc_11, "RC11_REV", "RC Channel 11 rev", &rc_11.rev);
	create_param(k_param_rc_12, "RC12_REV", "RC Channel 12 rev", &rc_12.rev);
	create_param(k_param_rc_13, "RC13_REV", "RC Channel 13 rev", &rc_13.rev);
	create_param(k_param_rc_14, "RC14_REV", "RC Channel 14 rev", &rc_14.rev);
	create_param(k_param_rc_1, "RC1_TRIM", "RC Channel 1 trim", &rc_1.trim);
	create_param(k_param_rc_2, "RC2_TRIM", "RC Channel 2 trim", &rc_2.trim);
	create_param(k_param_rc_3, "RC3_TRIM", "RC Channel 3 trim", &rc_3.trim);
	create_param(k_param_rc_4, "RC4_TRIM", "RC Channel 4 trim", &rc_4.trim);
	create_param(k_param_rc_5, "RC5_TRIM", "RC Channel 5 trim", &rc_5.trim);
	create_param(k_param_rc_6, "RC6_TRIM", "RC Channel 6 trim", &rc_6.trim);
	create_param(k_param_rc_7, "RC7_TRIM", "RC Channel 7 trim", &rc_7.trim);
	create_param(k_param_rc_8, "RC8_TRIM", "RC Channel 8 trim", &rc_8.trim);
	create_param(k_param_rc_9, "RC9_TRIM", "RC Channel 9 trim", &rc_9.trim);
	create_param(k_param_rc_10, "RC10_TRIM", "RC Channel 10 trim", &rc_10.trim);
	create_param(k_param_rc_11, "RC11_TRIM", "RC Channel 11 trim", &rc_11.trim);
	create_param(k_param_rc_12, "RC12_TRIM", "RC Channel 12 trim", &rc_12.trim);
	create_param(k_param_rc_13, "RC13_TRIM", "RC Channel 13 trim", &rc_13.trim);
	create_param(k_param_rc_14, "RC14_TRIM", "RC Channel 14 trim", &rc_14.trim);
	create_param(k_param_rc_5, "RC5_FUNCTION", "RC Channel 5 function", &rc_5.function);
	create_param(k_param_rc_6, "RC6_FUNCTION", "RC Channel 6 function", &rc_6.function);
	create_param(k_param_rc_7, "RC7_FUNCTION", "RC Channel 7 function", &rc_7.function);
	create_param(k_param_rc_8, "RC8_FUNCTION", "RC Channel 8 function", &rc_8.function);
	create_param(k_param_rc_9, "RC9_FUNCTION", "RC Channel 9 function", &rc_9.function);
	create_param(k_param_rc_10, "RC10_FUNCTION", "RC Channel 10 function", &rc_10.function);
	create_param(k_param_rc_11, "RC11_FUNCTION", "RC Channel 11 function", &rc_11.function);
	create_param(k_param_rc_12, "RC12_FUNCTION", "RC Channel 12 function", &rc_12.function);
	create_param(k_param_rc_13, "RC13_FUNCTION", "RC Channel 13 function", &rc_13.function);
	create_param(k_param_rc_14, "RC14_FUNCTION", "RC Channel 14 function", &rc_14.function);
	create_param(k_param_gain_default, "JS_GAIN_DEFAULT", "Default gain at boot", &gain_default);
	create_param(k_param_maxGain, "JS_GAIN_MAX", "Maximum joystick gain", &maxGain);
	create_param(k_param_minGain, "JS_GAIN_MIN", "Minimum jyostick gain", &minGain);
	create_param(k_param_numGainSettings, "JS_GAIN_STEPS", "Gain steps", &numGainSettings);
	create_param(k_param_cam_tilt_step, "JS_CAM_TILT_STEP", "Camera tilt step size", &cam_tilt_step);
	create_param(k_param_lights_step, "JS_LIGHTS_STEP", "Lights step size", &lights_step);
	create_param(k_param_jbtn_0, "BTN0_FUNCTION", "Button 1 function", &jbtn_0);
	create_param(k_param_jbtn_0, "BTN0_SFUNCTION", "Button 1 shift function", &jbtn_s_0);
	create_param(k_param_jbtn_1, "BTN0_FUNCTION", "Button 2 function", &jbtn_1);
	create_param(k_param_jbtn_1, "BTN0_SFUNCTION", "Button 2 shift function", &jbtn_s_1);
	create_param(k_param_jbtn_2, "BTN0_FUNCTION", "Button 3 function", &jbtn_2);
	create_param(k_param_jbtn_2, "BTN0_SFUNCTION", "Button 3 shift function", &jbtn_s_2);
	create_param(k_param_jbtn_3, "BTN0_FUNCTION", "Button 4 function", &jbtn_3);
	create_param(k_param_jbtn_3, "BTN0_SFUNCTION", "Button 4 shift function", &jbtn_s_3);
	create_param(k_param_jbtn_4, "BTN0_FUNCTION", "Button 5 function", &jbtn_4);
	create_param(k_param_jbtn_4, "BTN0_SFUNCTION", "Button 5 shift function", &jbtn_s_4);
	create_param(k_param_jbtn_5, "BTN0_FUNCTION", "Button 6 function", &jbtn_5);
	create_param(k_param_jbtn_5, "BTN0_SFUNCTION", "Button 6 shift function", &jbtn_s_5);
	create_param(k_param_jbtn_6, "BTN0_FUNCTION", "Button 7 function", &jbtn_6);
	create_param(k_param_jbtn_6, "BTN0_SFUNCTION", "Button 7 shift function", &jbtn_s_6);
	create_param(k_param_jbtn_7, "BTN0_FUNCTION", "Button 8 function", &jbtn_7);
	create_param(k_param_jbtn_7, "BTN0_SFUNCTION", "Button 8 shift function", &jbtn_s_7);
	create_param(k_param_jbtn_8, "BTN0_FUNCTION", "Button 9 function", &jbtn_8);
	create_param(k_param_jbtn_8, "BTN0_SFUNCTION", "Button 9 shift function", &jbtn_s_8);
	create_param(k_param_jbtn_9, "BTN0_FUNCTION", "Button 10 function", &jbtn_9);
	create_param(k_param_jbtn_9, "BTN0_SFUNCTION", "Button 10 shift function", &jbtn_s_9);
	create_param(k_param_jbtn_10, "BTN0_FUNCTION", "Button 11 function", &jbtn_10);
	create_param(k_param_jbtn_10, "BTN0_SFUNCTION", "Button 11 shift function", &jbtn_s_10);
	create_param(k_param_jbtn_11, "BTN0_FUNCTION", "Button 12 function", &jbtn_11);
	create_param(k_param_jbtn_11, "BTN0_SFUNCTION", "Button 12 shift function", &jbtn_s_11);
	create_param(k_param_jbtn_12, "BTN0_FUNCTION", "Button 13 function", &jbtn_12);
	create_param(k_param_jbtn_12, "BTN0_SFUNCTION", "Button 13 shift function", &jbtn_s_12);
	create_param(k_param_jbtn_13, "BTN0_FUNCTION", "Button 14 function", &jbtn_13);
	create_param(k_param_jbtn_13, "BTN0_SFUNCTION", "Button 14 shift function", &jbtn_s_13);
	create_param(k_param_jbtn_14, "BTN0_FUNCTION", "Button 15 function", &jbtn_14);
	create_param(k_param_jbtn_14, "BTN0_SFUNCTION", "Button 15 shift function", &jbtn_s_14);
	create_param(k_param_jbtn_15, "BTN0_FUNCTION", "Button 16 function", &jbtn_15);
	create_param(k_param_jbtn_15, "BTN0_SFUNCTION", "Button 16 shift function", &jbtn_s_15);
	create_param(k_param_rc_speed, "RC_SPEED", "ESC Update Speed", &rc_speed);
	create_param(k_param_acro_rp_p, "ACRO_RP_P", "Acro Roll and Pitch P gain", &acro_rp_p);
	create_param(k_param_acro_yaw_p, "ACRO_YAW_P", "Acro Yaw P gain", &acro_yaw_p);
	create_param(k_param_acro_balance_roll, "ACRO_BAL_ROLL", "Acro Balance Roll", &acro_balance_roll);
	create_param(k_param_acro_balance_pitch, "ACRO_BAL_PITCH", "Acro Balance Pitch", &acro_balance_pitch);
	create_param(k_param_acro_trainer, "ACRO_TRAINER", "Acro Trainer", &acro_trainer);
	create_param(k_param_acro_expo, "ACRO_EXPO", "Acro Expo", &acro_expo);
	create_param(k_param_pi_vel_xy, "VEL_XY_FILT_HZ", "Velocity (horizontal) filter", &xy_filt_hz);
	create_param(k_param_pi_vel_xy, "VEL_XY_I", "Velocity (horizontal) I gain", &xy_filt_hz);
	create_param(k_param_pi_vel_xy, "VEL_XY_IMAX", "Velocity (horizontal) integrator maximum", &xy_filt_hz);
	create_param(k_param_pi_vel_xy, "VEL_XY_P", "Velocity (horizontal) P gain", &xy_filt_hz);
	create_param(k_param_p_vel_z, "VEL_Z_P", "Velocity (vertical) P gain", &z_p);

	create_param(k_param_pid_accel_z, "ACCEL_Z_P", "Throttle acceleration controller P gain", &accel_z_p);
	create_param(k_param_pid_accel_z, "ACCEL_Z_I", "Throttle acceleration controller I gain", &accel_z_i);
	create_param(k_param_pid_accel_z, "ACCEL_Z_IMAX", "Throttle acceleration controller I gain maximum", &accel_z_imax);
	create_param(k_param_pid_accel_z, "ACCEL_Z_D", "Throttle acceleation controller D gain", &accel_z_d);
	create_param(k_param_pid_accel_z, "ACCEL_Z_FILT", "Throttle Acceleration filter", &accel_z_filt);
	create_param(k_param_p_alt_hold, "POS_Z_P", "Position (vertical) controller P gain", &pos_z_p);
	create_param(k_param_p_pos_xy, "POS_XY_P", "Position (horizontal) controller P gain", &pos_xy_p);
#if TRANSECT_ENABLED == ENABLED
	create_param(k_param_pid_crosstrack_control, "XTRACK_ANG_LIM", "", &xtrack_angle_limit);
#endif
#if CAMERA == ENABLED
	create_param(k_param_camera, "CAM_CENTER", "", &cam.center);
	create_param(k_param_camera, "CAM_DURATION", "Duration that shutter is held open", &cam.duration);
	create_param(k_param_camera, "CAM_FEEDBACK_PIN", "Camera feedback pin", &cam.feedback_pin);
	create_param(k_param_camera, "CAM_FEEDBACK_POL", "Camera feedback pin polarity", &cam.feedback_pol);
	create_param(k_param_camera, "CAM_MAX_ROLL", "Maximum photo roll angle", &cam.max_roll);
	create_param(k_param_camera, "CAM_MIN_INTERVAL", "Minimum time between photos", &cam.min_interval);
	create_param(k_param_camera, "CAM_RELAY_ON", "Relay on value", &cam.relay_on);
	create_param(k_param_camera, "CAM_SERVO_OFF", "Servo OFF PWM value", &cam.servo_off);
	create_param(k_param_camera, "CAM_SERVO_ON", "Servo ON PWM value", &cam.servo_on);
	create_param(k_param_camera, "CAM_TRIGG_DIST", "Camera trigger distance", &cam.trigg_dist);
	create_param(k_param_camera, "CAM_TRIGG_TYPE", "Camera shutter (trigger) type", &cam.trigg_type);
#endif

	create_param(k_param_relay, "RELAY_DEFAULT", "Default relay state", &relay.default);
	create_param(k_param_relay, "RELAY_PIN", "First Relay Pin", &relay.pin);
	create_param(k_param_relay, "RELAY_PIN2", "Second Relay Pin", &relay.pin2);
	create_param(k_param_relay, "RELAY_PIN3", "Third Relay Pin", &relay.pin3);
	create_param(k_param_relay, "RELAY_PIN4", "Fourth Relay Pin", &relay.pin4);

	create_param(k_param_compass, "COMPASS_AUTODEC", "Auto Declination", &compass.autodec);
	create_param(k_param_compass, "COMPASS_OFS_X", "Compass offset in milligauss on the X axis", &compass.ofs_x);
	create_param(k_param_compass, "COMPASS_OFS_Y", "Compass offset in milligauss on the Y axis", &compass.ofs_y);
	create_param(k_param_compass, "COMPASS_OFS_Z", "Compass offset in milligauss on the Z axis", &compass.ofs_z);
	create_param(k_param_compass, "COMPASS_OFS2_X", "Compass 2 offset in milligauss on the X axis", &compass.ofs2_x);
	create_param(k_param_compass, "COMPASS_OFS2_Y", "Compass 2 offset in milligauss on the Y axis", &compass.ofs2_y);
	create_param(k_param_compass, "COMPASS_OFS2_Z", "Compass 2 offset in milligauss on the Z axis", &compass.ofs2_z);
	create_param(k_param_compass, "COMPASS_OFS3_X", "Compass 3 offset in milligauss on the X axis", &compass.ofs3_x);
	create_param(k_param_compass, "COMPASS_OFS3_Y", "Compass 3 offset in milligauss on the Y axis", &compass.ofs3_y);
	create_param(k_param_compass, "COMPASS_OFS3_Z", "Compass 3 offset in milligauss on the Z axis", &compass.ofs3_z);
	create_param(k_param_compass, "COMPASS_DEC", "Compass declination", &compass.dec);
	create_param(k_param_compass, "COMPASS_LEARN", "Learn compass offset automatically", &compass.learn);
	create_param(k_param_compass, "COMPASS_USE", "Use compass for yaw", &compass.use);
	create_param(k_param_compass, "COMPASS_USE2", "Use compass 2 for yaw", &compass.use2);
	create_param(k_param_compass, "COMPASS_USE3", "Use compass 3 for yaw", &compass.use3);
	create_param(k_param_compass, "COMPASS_MOTCT", "Motor interference compensation type", &compass.motct);
	create_param(k_param_compass, "COMPASS_MOT_X", "Motor interference compensation for body frame x axis", &compass.mot_x);
	create_param(k_param_compass, "COMPASS_MOT_Y", "Motor interference compensation for body frame y axis", &compass.mot_y);
	create_param(k_param_compass, "COMPASS_MOT_Z", "Motor interference compensation for body frame z axis", &compass.mot_z);
	create_param(k_param_compass, "COMPASS_MOT2_X", "Motor interference compensation for body frame x axis (2)", &compass.mot2_x);
	create_param(k_param_compass, "COMPASS_MOT2_Y", "Motor interference compensation for body frame y axis (2)", &compass.mot2_y);
	create_param(k_param_compass, "COMPASS_MOT2_Z", "Motor interference compensation for body frame z axis (2)", &compass.mot2_z);
	create_param(k_param_compass, "COMPASS_MOT3_X", "Motor interference compensation for body frame x axis (3)", &compass.mot3_x);
	create_param(k_param_compass, "COMPASS_MOT3_Y", "Motor interference compensation for body frame y axis (3)", &compass.mot3_y);
	create_param(k_param_compass, "COMPASS_MOT3_Z", "Motor interference compensation for body frame z axis (3)", &compass.mot3_z);
	create_param(k_param_compass, "COMPASS_ORIENT", "Compass orientation", &compass.orient);
	create_param(k_param_compass, "COMPASS_ORIENT2", "Compass orientation", &compass.orient2);
	create_param(k_param_compass, "COMPASS_ORIENT3", "Compass orientation", &compass.orient3);
	create_param(k_param_compass, "COMPASS_EXTERNAL", "Compass is attached via an external cable", &compass.external);
	create_param(k_param_compass, "COMPASS_EXTERN2", "Compass is attached via an external cable (2)", &compass.extern2);
	create_param(k_param_compass, "COMPASS_EXTERN3", "Compass is attached via an external cable (3)", &compass.extern3);
	create_param(k_param_compass, "COMPASS_PRIMARY", "Choose primary compass", &compass.primary);
	create_param(k_param_compass, "COMPASS_DEV_ID", "Compass device id", &compass.dev_id);
	create_param(k_param_compass, "COMPASS_DEV_ID", "Compass device id2", &compass.dev_id);
	create_param(k_param_compass, "COMPASS_DEV_ID", "Compass device id3", &compass.dev_id);
	create_param(k_param_compass, "COMPASS_DIA_X", "Compass soft iron diagonal X component", &compass.dia_x);
	create_param(k_param_compass, "COMPASS_DIA_Y", "Compass soft iron diagonal Y component", &compass.dia_y);
	create_param(k_param_compass, "COMPASS_DIA_Z", "Compass soft iron diagonal Z component", &compass.dia_z);
	create_param(k_param_compass, "COMPASS_DIA2_X", "Compass 2 soft iron diagonal X component", &compass.dia2_x);
	create_param(k_param_compass, "COMPASS_DIA2_Y", "Compass 2 soft iron diagonal Y component", &compass.dia2_y);
	create_param(k_param_compass, "COMPASS_DIA2_Z", "Compass 2 soft iron diagonal Z component", &compass.dia2_z);
	create_param(k_param_compass, "COMPASS_DIA3_X", "Compass 3 soft iron diagonal X component", &compass.dia3_x);
	create_param(k_param_compass, "COMPASS_DIA3_Y", "Compass 3 soft iron diagonal Y component", &compass.dia3_y);
	create_param(k_param_compass, "COMPASS_DIA3_Z", "Compass 3 soft iron diagonal Z component", &compass.dia3_z);
	create_param(k_param_compass, "COMPASS_ODI_X", "Compass soft iron off diagonal X component", &compass.odi_x);
	create_param(k_param_compass, "COMPASS_ODI_Y", "Compass soft iron off diagonal Y component", &compass.odi_y);
	create_param(k_param_compass, "COMPASS_ODI_Z", "Compass soft iron off diagonal Z component", &compass.odi_z);
	create_param(k_param_compass, "COMPASS_ODI2_X", "Compass 2 soft iron off diagonal X component", &compass.odi2_x);
	create_param(k_param_compass, "COMPASS_ODI2_Y", "Compass 2 soft iron off diagonal Y component", &compass.odi2_y);
	create_param(k_param_compass, "COMPASS_ODI2_Z", "Compass 2 soft iron off diagonal Z component", &compass.odi2_z);
	create_param(k_param_compass, "COMPASS_ODI3_X", "Compass 3 soft iron off diagonal X component", &compass.odi3_x);
	create_param(k_param_compass, "COMPASS_ODI3_Y", "Compass 3 soft iron off diagonal Y component", &compass.odi3_y);
	create_param(k_param_compass, "COMPASS_ODI3_Z", "Compass 3 soft iron off diagonal Z component", &compass.odi3_z);
	create_param(k_param_compass, "COMPASS_CAL_FIT", "Compass calibration fitness", &compass.cal_fit);

	create_param(k_param_ins, "INS_PRODUCT_ID", "IMU Product ID", &ins.product_id);
	create_param(k_param_ins, "INS_GYROFFS_X", "Gyro offsets of X axis", &ins.gyroffs_x);
	create_param(k_param_ins, "INS_GYROFFS_Y", "Gyro offsets of Y axis", &ins.gyroffs_y);
	create_param(k_param_ins, "INS_GYROFFS_Z", "Gyro offsets of Z axis", &ins.gyroffs_z);
	create_param(k_param_ins, "INS_GYR2OFFS_X", "Gyro 2 offsets of X axis", &ins.gyr2offs_x);
	create_param(k_param_ins, "INS_GYR2OFFS_Y", "Gyro 2 offsets of Y axis", &ins.gyr2offs_y);
	create_param(k_param_ins, "INS_GYR2OFFS_Z", "Gyro 2 offsets of Z axis", &ins.gyr2offs_z);
	create_param(k_param_ins, "INS_GYR3OFFS_X", "Gyro 3 offsets of X axis", &ins.gyr3offs_x);
	create_param(k_param_ins, "INS_GYR3OFFS_Y", "Gyro 3 offsets of Y axis", &ins.gyr3offs_y);
	create_param(k_param_ins, "INS_GYR3OFFS_Z", "Gyro 3 offsets of Z axis", &ins.gyr3offs_z);
	create_param(k_param_ins, "INS_ACCSCAL_X", "Accelerometer scale of X axis", &ins.accscal_x);
	create_param(k_param_ins, "INS_ACCSCAL_Y", "Accelerometer scale of Y axis", &ins.accscal_y);
	create_param(k_param_ins, "INS_ACCSCAL_Z", "Accelerometer scale of Z axis", &ins.accscal_z);
	create_param(k_param_ins, "INS_ACC2SCAL_X", "Accelerometer 2 scale of X axis", &ins.acc2scal_x);
	create_param(k_param_ins, "INS_ACC2SCAL_Y", "Accelerometer 2 scale of Y axis", &ins.acc2scal_y);
	create_param(k_param_ins, "INS_ACC2SCAL_Z", "Accelerometer 2 scale of Z axis", &ins.acc2scal_z);
	create_param(k_param_ins, "INS_ACC3SCAL_X", "Accelerometer 3 scale of X axis", &ins.acc3scal_x);
	create_param(k_param_ins, "INS_ACC3SCAL_Y", "Accelerometer 3 scale of Y axis", &ins.acc3scal_y);
	create_param(k_param_ins, "INS_ACC3SCAL_Z", "Accelerometer 3 scale of Z axis", &ins.acc3scal_z);
	create_param(k_param_ins, "INS_ACCOFFS_X", "Accelerometer offset of X axis", &ins.accoffs_x);
	create_param(k_param_ins, "INS_ACCOFFS_Y", "Accelerometer offset of Y axis", &ins.accoffs_y);
	create_param(k_param_ins, "INS_ACCOFFS_Z", "Accelerometer offset of Z axis", &ins.accoffs_z);
	create_param(k_param_ins, "INS_ACC2OFFS_X", "Accelerometer 2 offset of X axis", &ins.acc2offs_x);
	create_param(k_param_ins, "INS_ACC2OFFS_Y", "Accelerometer 2 offset of Y axis", &ins.acc2offs_y);
	create_param(k_param_ins, "INS_ACC2OFFS_Z", "Accelerometer 2 offset of Z axis", &ins.acc2offs_z);
	create_param(k_param_ins, "INS_ACC3OFFS_X", "Accelerometer 3 offset of X axis", &ins.acc3offs_x);
	create_param(k_param_ins, "INS_ACC3OFFS_Y", "Accelerometer 3 offset of Y axis", &ins.acc3offs_y);
	create_param(k_param_ins, "INS_ACC3OFFS_Z", "Accelerometer 3 offset of Z axis", &ins.acc3offs_z);
	create_param(k_param_ins, "INS_GYRO_FILTER", "Gyro filter cutoff frequency", &ins.gyro_filter);
	create_param(k_param_ins, "INS_ACCEL_FILTER", "Accel filter cutoff frequency", &ins.accel_filter);
	create_param(k_param_ins, "INS_USE", "Use first IMU for attitude, velocity and position estimation.", &ins.use);
	create_param(k_param_ins, "INS_USE2", "Use second IMU for attitude, velocity and position estimation.", &ins.use2);
	create_param(k_param_ins, "INS_USE3", "Use thrid IMU for attitude, velocity and position estimation.", &ins.use3);
	create_param(k_param_ins, "INS_STILL_THRESH", "Stillness threshold for detecting if we are moving.", &ins.still_thresh);
	create_param(k_param_ins, "INS_GYR_CAL", "Gyro Calibratioin scheme", &ins.gyro_cal);
	create_param(k_param_ins, "INS_TRIM_OPTION", "Accel cal trim option", &ins.trim_option);
	create_param(k_param_ins, "INS_ACC_BODYFIX", "Body-fixed accelerometer", &ins.acc_bodyfix);
	create_param(k_param_ins, "INS_POS1_X", "IMU accelerometer X position", &ins.pos1_x);
	create_param(k_param_ins, "INS_POS1_Y", "IMU accelerometer Y position", &ins.pos1_y);
	create_param(k_param_ins, "INS_POS1_Z", "IMU accelerometer Z position", &ins.pos1_z);
	create_param(k_param_ins, "INS_POS2_X", "IMU accelerometer 2 X position", &ins.pos2_x);
	create_param(k_param_ins, "INS_POS2_Y", "IMU accelerometer 2 Y position", &ins.pos2_y);
	create_param(k_param_ins, "INS_POS2_Z", "IMU accelerometer 2 Z position", &ins.pos2_z);
	create_param(k_param_ins, "INS_POS3_X", "IMU accelerometer 3 X position", &ins.pos3_x);
	create_param(k_param_ins, "INS_POS3_Y", "IMU accelerometer 3 Y position", &ins.pos3_y);
	create_param(k_param_ins, "INS_POS3_Z", "IMU accelerometer 3 Z position", &ins.pos3_z);
	create_param(k_param_ins, "INS_GYR_ID", "Gyro ID", &ins.gyr_id);
	create_param(k_param_ins, "INS_GYR2_ID", "Gyro2 ID", &ins.gyr2_id);
	create_param(k_param_ins, "INS_GYR3_ID", "Gyro3 ID", &ins.gyr3_id);
	create_param(k_param_ins, "INS_ACC_ID", "Accelerometer ID", &ins.acc_id);
	create_param(k_param_ins, "INS_ACC2_ID", "Accelerometer2 ID", &ins.acc2_id);
	create_param(k_param_ins, "INS_ACC3_ID", "Accelerometer3 ID", &ins.acc3_id);
	create_param(k_param_ins, "INS_FAST_SAMPLE", "Fast sampling mask", &ins.fast_sample);
	
	create_param(k_param_wp_nav, "WPNAV_SPEED", "Waypoint Horizontal Speed Target", &wpnav.speed);
	create_param(k_param_wp_nav, "WPNAV_RADIUS", "Waypoint Radius", &wpnav.radius);
	create_param(k_param_wp_nav, "WPNAV_SPEED_UP", "Waypoint climb speed target", &wpnav.speed_up);
	create_param(k_param_wp_nav, "WPNAV_SPEED_DN", "Waypoint descent speed target", &wpnav.speed_dn);
	create_param(k_param_wp_nav, "WPNAV_LOIT_SPEED", "Loiter horizontal maximum speed", &wpnav.loit_speed);
	create_param(k_param_wp_nav, "WPNAV_ACCEL", "Waypoint acceleration", &wpnav.accel);
	create_param(k_param_wp_nav, "WPNAV_ACCEL_Z", "Waypoint vertical acceleration", &wpnav.accel_z);
	create_param(k_param_wp_nav, "WPNAV_LOIT_JERK", "Loiter horizontal maximum jerk", &wpnav.loit_jerk);
	create_param(k_param_wp_nav, "WPNAV_LOIT_MAXA", "Loiter horizontal maximum acceleration", &wpnav.loit_maxa);
	create_param(k_param_wp_nav, "WPNAV_LOIT_MINA", "Loiter horizontal minimum acceleration", &wpnav.loit_mina);
	create_param(k_param_wp_nav, "WPNAV_RFND_USE", "Use rangefinder for terrain following", &wpnav.rfnd_use);

	create_param(k_param_attitude_control, "ATC_SLEW_YAW", "Yaw target slew rate", &atc.slew_yaw);
	create_param(k_param_attitude_control, "ATC_ACCEL_P_MAX", "Acceleration Max for Pitch", &atc.accel_p_max);
	create_param(k_param_attitude_control, "ATC_ACCEL_R_MAX", "Acceleration Max for Roll", &atc.accel_r_max);
	create_param(k_param_attitude_control, "ATC_ACCEL_Y_MAX", "Acceleration Max for Yaw", &atc.accel_y_max);
	create_param(k_param_attitude_control, "ATC_ANGLE_BOOST", "Angle Boost", &atc.angle_boost);
	create_param(k_param_attitude_control, "ATC_ANG_LIM_TC", "Angle Limit (to maintain altitude) Time Constant", &atc.ang_lim_tc);
	create_param(k_param_attitude_control, "ATC_ANG_PIT_P", "Pitch axis angle controller P gain", &atc.ang_pit_p);
	create_param(k_param_attitude_control, "ATC_ANG_RLL_P", "Roll axis angle controller P gain", &atc.ang_rll_p);
	create_param(k_param_attitude_control, "ATC_ANG_YAW_P", "Yaw axis angle controller P gain", &atc.ang_yaw_p);
	create_param(k_param_attitude_control, "ATC_RATE_FF_ENAB", "Rate Feedforward Enable", &atc.rate_ff_enab);
	create_param(k_param_attitude_control, "ATC_RAT_RLL_P", "Roll axis rate controller P gain", &atc.rat_rll_p);
	create_param(k_param_attitude_control, "ATC_RAT_RLL_I", "Roll axis rate controller I gain", &atc.rat_rll_i);
	create_param(k_param_attitude_control, "ATC_RAT_RLL_IMAX", "Roll axis rate controller I gain maximum", &atc.rat_rll_imax);
	create_param(k_param_attitude_control, "ATC_RAT_RLL_D", "Roll axis rate controller D gain", &atc.rat_rll_d);
	create_param(k_param_attitude_control, "ATC_RAT_RLL_FILT", "Roll axis rate controller input frequency in Hz", &atc.rat_rll_filt);
	create_param(k_param_attitude_control, "ATC_RAT_PIT_P", "Pitch axis rate controller P gain", &atc.rat_pit_p);
	create_param(k_param_attitude_control, "ATC_RAT_PIT_I", "Pitch axis rate controller I gain", &atc.rat_pit_i);
	create_param(k_param_attitude_control, "ATC_RAT_PIT_IMAX", "Pitch axis rate controller I gain maximum", &atc.rat_pit_imax);
	create_param(k_param_attitude_control, "ATC_RAT_PIT_D", "Pitch axis rate controller D gain", &atc.rat_pit_d);
	create_param(k_param_attitude_control, "ATC_RAT_PIT_FILT", "Pitch axis rate controller input frequency in Hz", &atc.rat_pit_filt);
	create_param(k_param_attitude_control, "ATC_RAT_YAW_P", "Yaw axis rate controller P gain", &atc.rat_yaw_p);
	create_param(k_param_attitude_control, "ATC_RAT_YAW_I", "Yaw axis rate controller I gain", &atc.rat_yaw_i);
	create_param(k_param_attitude_control, "ATC_RAT_YAW_IMAX", "Yaw axis rate controller I gain maximum", &atc.rat_yaw_imax);
	create_param(k_param_attitude_control, "ATC_RAT_YAW_D", "Yaw axis rate controller D gain", &atc.rat_yaw_d);
	create_param(k_param_attitude_control, "ATC_RAT_YAW_FILT", "Yaw axis rate controller input frequency in Hz", &atc.rat_yaw_filt);
	create_param(k_param_attitude_control, "ATC_THR_MIX_MIN", "Thhrottle Mix Minimum", &atc.thr_mix_min);
	create_param(k_param_attitude_control, "ATC_THR_MIX_MAX", "Thhrottle Mix Maximum", &atc.thr_mix_max);

	create_param(k_param_pos_control, "PSC_ACC_XY_FILT", "XY Acceleration filter cuttoff frequency", &psc_acc_xy_filt);

	create_param(-1, "SR0_EXTRA1", "Extra data type 1 stream rate to ground station", &sr0.extra1);
	create_param(-1, "SR0_EXTRA2", "Extra data type 2 stream rate to ground station", &sr0.extra2);
	create_param(-1, "SR0_EXTRA3", "Extra data type 3 stream rate to ground station", &sr0.extra3);
	create_param(-1, "SR0_EXT_STAT", "Extended status stream rate to ground station", &sr0.ext_stat);
	create_param(-1, "SR0_PARAMS", "Paramter stream rate to ground station", &sr0.params);
	create_param(-1, "SR0_POSITION", "Position stream rate to ground station", &sr0.position);
	create_param(-1, "SR0_RAW_CTRL", "Raw Control stream rate to ground station", &sr0.raw_ctrl);
	create_param(-1, "SR0_RAW_SENS", "Raw sensor stream rate", &sr0.raw_sens);
	create_param(-1, "SR0_RC_CHAN", "RC Channel stream rate to ground station", &sr0.rc_chan);

	create_param(-1, "SR1_EXTRA1", "Extra data type 1 stream rate to ground station", &sr1.extra1);
	create_param(-1, "SR1_EXTRA2", "Extra data type 2 stream rate to ground station", &sr1.extra2);
	create_param(-1, "SR1_EXTRA3", "Extra data type 3 stream rate to ground station", &sr1.extra3);
	create_param(-1, "SR1_EXT_STAT", "Extended status stream rate to ground station", &sr1.ext_stat);
	create_param(-1, "SR1_PARAMS", "Paramter stream rate to ground station", &sr1.params);
	create_param(-1, "SR1_POSITION", "Position stream rate to ground station", &sr1.position);
	create_param(-1, "SR1_RAW_CTRL", "Raw Control stream rate to ground station", &sr1.raw_ctrl);
	create_param(-1, "SR1_RAW_SENS", "Raw sensor stream rate", &sr1.raw_sens);
	create_param(-1, "SR1_RC_CHAN", "RC Channel stream rate to ground station", &sr1.rc_chan);

	create_param(-1, "SR2_EXTRA1", "Extra data type 1 stream rate to ground station", &sr2.extra1);
	create_param(-1, "SR2_EXTRA2", "Extra data type 2 stream rate to ground station", &sr2.extra2);
	create_param(-1, "SR2_EXTRA3", "Extra data type 3 stream rate to ground station", &sr2.extra3);
	create_param(-1, "SR2_EXT_STAT", "Extended status stream rate to ground station", &sr2.ext_stat);
	create_param(-1, "SR2_PARAMS", "Paramter stream rate to ground station", &sr2.params);
	create_param(-1, "SR2_POSITION", "Position stream rate to ground station", &sr2.position);
	create_param(-1, "SR2_RAW_CTRL", "Raw Control stream rate to ground station", &sr2.raw_ctrl);
	create_param(-1, "SR2_RAW_SENS", "Raw sensor stream rate", &sr2.raw_sens);
	create_param(-1, "SR2_RC_CHAN", "RC Channel stream rate to ground station", &sr2.rc_chan);

	create_param(-1, "SR3_EXTRA1", "Extra data type 1 stream rate to ground station", &sr3.extra1);
	create_param(-1, "SR3_EXTRA2", "Extra data type 2 stream rate to ground station", &sr3.extra2);
	create_param(-1, "SR3_EXTRA3", "Extra data type 3 stream rate to ground station", &sr3.extra3);
	create_param(-1, "SR3_EXT_STAT", "Extended status stream rate to ground station", &sr3.ext_stat);
	create_param(-1, "SR3_PARAMS", "Paramter stream rate to ground station", &sr3.params);
	create_param(-1, "SR3_POSITION", "Position stream rate to ground station", &sr3.position);
	create_param(-1, "SR3_RAW_CTRL", "Raw Control stream rate to ground station", &sr3.raw_ctrl);
	create_param(-1, "SR3_RAW_SENS", "Raw sensor stream rate", &sr3.raw_sens);
	create_param(-1, "SR3_RC_CHAN", "RC Channel stream rate to ground station", &sr3.rc_chan);

	create_param(k_param_ahrs, "AHRS_COMP_BETA", "AHRS velocity complementary filter beta coefficient", &ahrs.comp_beta);
	create_param(k_param_ahrs, "AHRS_EKF_TYPE", "Use NavEKF Kalman filter for attitude and position estimation", &ahrs.ekf_type);
	create_param(k_param_ahrs, "AHRS_GPS_GAIN", "AHRS GPS gain", &ahrs.gps_gain);
	create_param(k_param_ahrs, "AHRS_GPS_MINSATS", "AHRS GPS minimum satellites", &ahrs.gps_minsats);
	create_param(k_param_ahrs, "AHRS_GPS_USE", "AHRS use GPS for navigation", &ahrs.gps_use);
	create_param(k_param_ahrs, "AHRS_ORIENTATION", "Board orientation", &ahrs.orientation);
	create_param(k_param_ahrs, "AHRS_RP_P", "AHRS RP_P", &ahrs.rp_p);
	create_param(k_param_ahrs, "AHRS_TRIM_Z", "AHRS Trim Yaw", &ahrs.trim_z);
	create_param(k_param_ahrs, "AHRS_WIND_MAX", "Maximum wind", &ahrs.wind_max);
	create_param(k_param_ahrs, "AHRS_YAW_P", "Yaw P", &ahrs.yaw_p);

#if MOUNT == ENABLED
	create_param(k_param_camera_mount, "MNT_RETRACT_X", "Mount roll angle when in retracted position", &mnt.retract_x);
	create_param(k_param_camera_mount, "MNT_RETRACT_Y", "Mount tilt angle when in retracted position", &mnt.retract_y);
	create_param(k_param_camera_mount, "MNT_RETRACT_Z", "Mount pan angle when in retracted position", &mnt.retract_z);

	create_param(k_param_camera_mount, "MNT_NEUTRAL_X", "Mount roll angle when in neutral position", &mnt.neutral_x);
	create_param(k_param_camera_mount, "MNT_NEUTRAL_Y", "Mount tilt angle when in neutral position", &mnt.neutral_y);
	create_param(k_param_camera_mount, "MNT_NEUTRAL_Z", "Mount pan angle when in neutral position", &mnt.neutral_z);

	create_param(k_param_camera_mount, "MNT_STAB_ROLL", "Stabilize mount's roll angle", &mnt.stab_roll);
	create_param(k_param_camera_mount, "MNT_STAB_PAN", "Stabilize mount's pan angle", &mnt.stab_pan);
	create_param(k_param_camera_mount, "MNT_STAB_TILT", "Stabilize mount's tilt angle", &mnt.stab_tilt);

	create_param(k_param_camera_mount, "MNT_ANGMAX_ROLL", "Maximum roll angle", &mnt.angmax_rol);
	create_param(k_param_camera_mount, "MNT_ANGMAX_PAN", "Maximum pan angle", &mnt.angmax_pan);
	create_param(k_param_camera_mount, "MNT_ANGMAX_TILT", "Maximum tilt angle", &mnt.angmax_til);

	create_param(k_param_camera_mount, "MNT_ANGMIN_ROLL", "Minimum roll angle", &mnt.angmin_rol);
	create_param(k_param_camera_mount, "MNT_ANGMIN_PAN", "Minimum pan angle", &mnt.angmin_pan);
	create_param(k_param_camera_mount, "MNT_ANGMIN_TILT", "Minimum tilt angle", &mnt.angmin_til);
	create_param(k_param_camera_mount, "MNT_ANGMIN_ROLL", "Stabilize mount's roll angle", &mnt.stab_roll);
	create_param(k_param_camera_mount, "MNT_ANGMIN_PAN", "Stabilize mount's pan angle", &mnt.stab_pan);
	create_param(k_param_camera_mount, "MNT_ANGMIN_TILT", "Stabilize mount's tilt angle", &mnt.stab_tilt);

	create_param(k_param_camera_mount, "MNT_RC_IN_ROLL", "Roll RC input channel", &mnt.rc_in_roll);
	create_param(k_param_camera_mount, "MNT_RC_IN_PAN", "Pan RC input channel", &mnt.rc_in_pan);
	create_param(k_param_camera_mount, "MNT_RC_IN_TILT", "Tilt RC input channel", &mnt.rc_in_tilt);

	create_param(k_param_camera_mount, "MNT_JSTICK_SPD", "Mount joystic speed", &mnt.jstick_spd);

	create_param(k_param_camera_mount, "MNT_LEAD_PTCH", "Pitch stabilization lead time", &mnt.lead_pitch);
	create_param(k_param_camera_mount, "MNT_LEAD_RLL", "Roll stabilization lead time", &mnt.lead_rll);

	create_param(k_param_camera_mount, "MNT_TYPE", "Mount type", &mnt.type);
#endif

	create_param(k_param_DataFlash, "LOG_BACKEND_TYPE", "DataFlash Backend Storage type", &log.backend_type);
	create_param(k_param_DataFlash, "LOG_FILE_BUFSIZE", "Maximum DataFlash File Backend buffer size (in kilobytes)", &log.file_bufsize);
	create_param(k_param_DataFlash, "LOG_DISARMED", "Enable logging while disarmed", &log.disarmed);
	create_param(k_param_DataFlash, "LOG_REPLAY", "Enable logging of information needed for Replay", &log.replay);
	create_param(k_param_DataFlash, "LOG_FILE_DSRMROT", "Stop logging to current file on disarm", &log.file_dsrmrot);
	create_param(k_param_log_bitmask, "LOG_BITMASK", "", &log.bitmask);

	create_param(k_param_battery, "BAT_MONITOR", "Battery monitoring", &batt.monitor);
	create_param(k_param_battery, "BAT_VOLT_PIN", "Battery Voltage sensing pin", &batt.volt_pin);
	create_param(k_param_battery, "BAT_CURR_PIN", "Battery Current sensing pin", &batt.curr_pin);
	create_param(k_param_battery, "BAT_VOLT_MULT", "Voltage Multiplier", &batt.volt_mult);
	create_param(k_param_battery, "BAT_AMP_PERVOLT", "Amps per volt", &batt.amp_pervol);
	create_param(k_param_battery, "BAT_AMP_OFFSET", "AMP offset", &batt.amp_offset);
	create_param(k_param_battery, "BAT_CAPACITY", "Battery capacity", &batt.capacity);

	create_param(k_param_battery, "BAT2_MONITOR", "Battery monitoring", &batt2.monitor);
	create_param(k_param_battery, "BAT2_VOLT_PIN", "Battery Voltage sensing pin", &batt2.volt_pin);
	create_param(k_param_battery, "BAT2_CURR_PIN", "Battery Current sensing pin", &batt2.curr_pin);
	create_param(k_param_battery, "BAT2_VOLT_MULT", "Voltage Multiplier", &batt2.volt_mult);
	create_param(k_param_battery, "BAT2_AMP_PERVOLT", "Amps per volt", &batt2.amp_pervol);
	create_param(k_param_battery, "BAT2_AMP_OFFSET", "AMP offset", &batt2.amp_offset);
	create_param(k_param_battery, "BAT2_CAPACITY", "Battery capacity", &batt2.capacity);

	create_param(k_param_BoardConfig, "BRD_PWM_COUNT", "Auxiliary pin config", &brd.pwm_count);
	create_param(k_param_BoardConfig, "BRD_SER1_RTSCTS", "Serial 1 flow control", &brd.ser1_rtscts); 
	create_param(k_param_BoardConfig, "BRD_SER2_RTSCTS", "Serial 2 flow control", &brd.ser2_rtscts);
	create_param(k_param_BoardConfig, "BRD_SAFETYENABLE", "Enable use of safety arming switch", &brd.safetyenable);
	create_param(k_param_BoardConfig, "BRD_SBUS_OUT", "SBUS output rate", &brd.sbus_out);
	create_param(k_param_BoardConfig, "BRD_SERIAL_NUM", "User-defined serial number", &brd.serial_num);
	create_param(k_param_BoardConfig, "BRD_CAN_ENABLE", "Enable use of UAVCAN devices", &brd.can_enable);
	create_param(k_param_BoardConfig, "BRD_SAFETY_MASK", "Channels to which ignore the safety switch state", &brd.safety_mask);
	create_param(k_param_BoardConfig, "BRD_IMU_TARGTEMP", "Target IMU temperature", &brd.imu_targetemp);
	create_param(k_param_BoardConfig, "BRD_TYPE", "Board type", &brd.type);

	create_param(k_param_barometer, "GND_ABS_PRESS", "Absolute Pressure", &gnd.abs_press);
	create_param(k_param_barometer, "GND_TEMP", "Ground temperature", &gnd.temp);
	create_param(k_param_barometer, "GND_ALT_OFFSET", "Altitude offset", &gnd.alt_offset);
	create_param(k_param_barometer, "GND_PRIMARY", "Primary barometer", &gnd.primary);
	create_param(k_param_barometer, "GND_SPEC_GRAV", "Specific Gravity (for water depth measurement)", &gnd.spec_grav);
	create_param(k_param_barometer, "GND_BASE_PRESS", "Base Pressure (for water depth measurement)", &gnd.base_press);
	create_param(k_param_barometer, "GND_BASE_RESET", "Reset Base Pressure (for water depth measurement)", &gnd.base_reset);

	create_param(k_param_gps, "GPS_TYPE", "GPS Type", &gps.type);
	create_param(k_param_gps, "GPS_TYPE2", "GPS2 Type", &gps.type2);
	create_param(k_param_gps, "GPS_NAVFILTER", "Navigation filter setting", &gps.navfilter);
	create_param(k_param_gps, "GPS_AUTO_SWITCH", "Automatic Switchover Setting", &gps.auto_switch);
	create_param(k_param_gps, "GPS_MIN_DGPS", "Minimum Lock Type Accepted for DGPS", &gps.min_dgps);
	create_param(k_param_gps, "GPS_SBAS_MODE", "SBAS Mode", &gps.sbas_mode);
	create_param(k_param_gps, "GPS_MIN_ELEV", "Minimum elevation", &gps.min_elevation);
	create_param(k_param_gps, "GPS_INJECT_TO", "Destination for GPS_INJECT_DATA Mavlink packets", &gps.inject_to);
	create_param(k_param_gps, "GPS_SBP_LOGMASK", "Swift Binary Protocol Logging Mask", &gps.sbp_logmask);
	create_param(k_param_gps, "GPS_RAW_DATA", "Raw data logging", &gps.raw_data);
	create_param(k_param_gps, "GPS_GNSS_MODE", "GNSS system configuration", &gps.gnss_mode);
	create_param(k_param_gps, "GPS_GNSS_MODE2", "GNSS system 2 configuration", &gps.gnss_mode2);
	create_param(k_param_gps, "GPS_SAVE_CFG", "Save GPS configuration", &gps.save_cfg);
	create_param(k_param_gps, "GPS_AUTOCONFIG", "Automatic GPS configuration", &gps.auto_config);
	create_param(k_param_gps, "GPS_RATE_MS", "GPS update rate in milliseconds", &gps.rate_ms);
	create_param(k_param_gps, "GPS_RATE_MS2", "GPS update rate in milliseconds", &gps.rate_ms2);
	create_param(k_param_gps, "GPS_POS1_X", "Antenna X position offset", &gps.pos1_x);
	create_param(k_param_gps, "GPS_POS1_Y", "Antenna Y position offset", &gps.pos1_y);
	create_param(k_param_gps, "GPS_POS1_Z", "Antenna Z position offset", &gps.pos1_z);
	create_param(k_param_gps, "GPS_POS2_X", "Antenna X position offset", &gps.pos2_x);
	create_param(k_param_gps, "GPS_POS2_Y", "Antenna Y position offset", &gps.pos2_y);
	create_param(k_param_gps, "GPS_POS2_Z", "Antenna Z position offset", &gps.pos2_z);

	create_param(k_param_leak_detector, "LEAK1_LOGIC", "Default reading of leak detector when dry", &leak1.logic);
	create_param(k_param_leak_detector, "LEAK1_PIN", "Pin that leak detector isconnected to", &leak1.pin);
	create_param(k_param_leak_detector, "LEAK2_LOGIC", "Default reading of leak detector when dry", &leak2.logic);
	create_param(k_param_leak_detector, "LEAK2_PIN", "Pin that leak detector isconnected to", &leak2.pin);
	create_param(k_param_leak_detector, "LEAK3_LOGIC", "Default reading of leak detector when dry", &leak3.logic);
	create_param(k_param_leak_detector, "LEAK3_PIN", "Pin that leak detector isconnected to", &leak3.pin);

	create_param(k_param_scheduler, "SCHED_DEBUG", "Scheduler debug level", &sched.debug);
	create_param(k_param_scheduler, "SCHED_LOOP_RATE", "Scheduling main loop rate", &sched.loop_rate);

#if AC_FENCE == ENABLED
	create_param(k_param_fence, "FENCE_ENABLE", "Fence enable/disable", &fence.enable);
	create_param(k_param_fence, "FENCE_TYPE", "Fence Type", &fence.type);
	create_param(k_param_fence, "FENCE_ACTION", "Fence Action", &fence.action);
	create_param(k_param_fence, "FENCE_ALT_MAX", "Fence Maximum Altitude", &fence.alt_max);
	create_param(k_param_fence, "FENCE_RADIUS", "Circular Fence Radius", &fence.radius);
	create_param(k_param_fence, "FENCE_MARGIN", "Fence Margin", &fence.margin);
	create_param(k_param_fence, "FENCE_TOTAL", "Fence polygon points total", &fence.total);
	create_param(k_param_fence, "FENCE_DEPTH_MAX", "Fence Maximum Depth", &fence.depth_max);
#endif

	create_param(k_param_motors, "MOT1_DIRECTION", "Motor normal or reverse", &mot.direction[0]);
	create_param(k_param_motors, "MOT2_DIRECTION", "Motor normal or reverse", &mot.direction[1]);
	create_param(k_param_motors, "MOT3_DIRECTION", "Motor normal or reverse", &mot.direction[2]);
	create_param(k_param_motors, "MOT4_DIRECTION", "Motor normal or reverse", &mot.direction[3]);
	create_param(k_param_motors, "MOT5_DIRECTION", "Motor normal or reverse", &mot.direction[4]);
	create_param(k_param_motors, "MOT6_DIRECTION", "Motor normal or reverse", &mot.direction[5]);
	create_param(k_param_motors, "MOT_YAW_HEADROOM", "Matrix Yaw Min", &mot.yaw_headroom);
	create_param(k_param_motors, "MOT_THST_EXPO", "Thurust Curve Expo", &mot.thst_expo);
	create_param(k_param_motors, "MOT_SPIN_MAX", "Motor Spin Maximum", &mot.spin_max);
	create_param(k_param_motors, "MOT_BAT_VOLT_MAX", "Battery voltage compensation maximum voltage", &mot.bat_volt_max);
	create_param(k_param_motors, "MOT_BAT_VOLT_MIN", "Battery voltage compensation minimum voltage", &mot.bat_volt_min);
	create_param(k_param_motors, "MOT_BAT_CURR_MAX", "Motor Current Max", &mot.bat_curr_max);
	create_param(k_param_motors, "MOT_PWM_TYPE", "Output PWM type", &mot.pwm_type);
	create_param(k_param_motors, "MOT_PWM_MIN", "PWM output min", &mot.pwm_min);
	create_param(k_param_motors, "MOT_PWM_MAX", "PWM output max", &mot.pwm_max);
	create_param(k_param_motors, "MOT_SPIN_ARM", "Motor Spin armed", &mot.spin_arm);
	create_param(k_param_motors, "MOT_SPIN_MIN", "Motor Spin minimum", &mot.spin_min);
	create_param(k_param_motors, "MOT_BAT_CURR_TC", "Motor Current Max Time Constant", &mot.bat_curr_tc);
	create_param(k_param_motors, "MOT_THUST_HOVER", "Thrust Hover Value", &mot.thst_hover);
	create_param(k_param_motors, "MOT_HOVER_LEARN", "Hover Value Learning", &mot.hover_learn);
	create_param(k_param_motors, "MOT_SAFE_DISARM", "Motor PWM output disabled when disarmed", &mot.safe_disarm);
	
	create_param(k_param_NavEKF, "EKF_ENABLE", "Enable EKF1", &ekf.enable);
	create_param(k_param_NavEKF, "EKF_VELNE_NOISE", "GPS horizontal velocity measurement noise scaler", &ekf.velne_noise);
	create_param(k_param_NavEKF, "EKF_VELD_NOISE", "GPS vertical velocity measurement noise scaler", &ekf.veld_noise);
	create_param(k_param_NavEKF, "EKF_POSNE_NOISE", "GPS horizontal position measurement noise (m)", &ekf.posne_noise);
	create_param(k_param_NavEKF, "EKF_ALT_NOISE", "Altitude measurement noise (m)", &ekf.alt_noise);
	create_param(k_param_NavEKF, "EKF_MAG_NOISE", "Magnetometer measurement noise (Gauss)", &ekf.mag_noise);
	create_param(k_param_NavEKF, "EKF_EAS_NOISE", "Equivalent airspeed measurement noise (m/s)", &ekf.eas_noise);
	create_param(k_param_NavEKF, "EKF_WIND_PNOISE", "Wind velocity process noise (m/s^2)", &ekf.wind_pnoise);
	create_param(k_param_NavEKF, "EKF_WIND_PSCALE", "Height rate to wind process noise scaler", &ekf.wind_pscale);
	create_param(k_param_NavEKF, "EKF_GYRO_PNOISE", "Rate gyro noise (rad/s)", &ekf.gyro_pnoise);
	create_param(k_param_NavEKF, "EKF_ACC_PNOISE", "Accelerometer noise (m/s^2)", &ekf.acc_pnoise);
	create_param(k_param_NavEKF, "EKF_GBIAS_PNOISE", "Rate gyro bias process noise (rad/s)", &ekf.gbias_pnoise);
	create_param(k_param_NavEKF, "EKF_ABIAS_PNOISE", "Accelerometer bias process noise (m/s^2)", &ekf.abias_pnoise);
	create_param(k_param_NavEKF, "EKF_MAGE_PNOISE", "Body magnetic field process noise (gauss/s)", &ekf.mage_pnoise);
	create_param(k_param_NavEKF, "EKF_VEL_DELAY", "GPS velocity measurement delay (msec)", &ekf.vel_delay);
	create_param(k_param_NavEKF, "EKF_POS_DELAY", "GPS position measurement delay (msec)", &ekf.pos_delay);
	create_param(k_param_NavEKF, "EKF_GPS_TYPE", "GPS mode control", &ekf.gps_type);
	create_param(k_param_NavEKF, "EKF_VEL_GATE", "GPS velocity measurement gate size", &ekf.vel_gate);
	create_param(k_param_NavEKF, "EKF_POS_GATE", "GPS position measurement gate size", &ekf.pos_gate);
	create_param(k_param_NavEKF, "EKF_HGT_GATE", "Height measurement gate size", &ekf.hgt_gate);
	create_param(k_param_NavEKF, "EKF_MAG_GATE", "Magnetometer measurement gate size", &ekf.mag_gate);
	create_param(k_param_NavEKF, "EKF_EAS_GATE", "Airspeed measurement gate size", &ekf.eas_gate);
	create_param(k_param_NavEKF, "EKF_MAG_CAL", "Magnetometer calibration mode", &ekf.mag_cal);
	create_param(k_param_NavEKF, "EKF_GLITCH_ACCEL", "GPS glitch accel gate size (cm/s^2)", &ekf.glitch_accel);
	create_param(k_param_NavEKF, "EKF_GLITCH_RAD", "GPS glitch radius gate size (m)", &ekf.glitch_rad);
	create_param(k_param_NavEKF, "EKF_GND_GRADIENT", "Terrain Gradient % RMS", &ekf.gnd_gradient);
	create_param(k_param_NavEKF, "EKF_FLOW_NOISE", "Optical flow measurement noise (rad/s)", &ekf.flow_noise);
	create_param(k_param_NavEKF, "EKF_FLOW_GATE", "Optical flow measurement gate size", &ekf.flow_gate);
	create_param(k_param_NavEKF, "EKF_FLOW_DELAY", "Optical flow measurement delay (msec)", &ekf.flow_delay);
	create_param(k_param_NavEKF, "EKF_RNG_GATE", "Range finder measurement gate size", &ekf.rng_gate);
	create_param(k_param_NavEKF, "EKF_MAX_FLOW", "Maximum valid optical flow rate", &ekf.max_flow);
	create_param(k_param_NavEKF, "EKF_FALLBACK", "Fallback strictness", &ekf.fallback);
	create_param(k_param_NavEKF, "EKF_ALT_SOURCE", "Primary height source", &ekf.alt_source);
	create_param(k_param_NavEKF, "EKF_GPS_CHECK", "GPS preflight check", &ekf.gps_check);


	create_param(k_param_NavEKF2, "EK2_ENABLE", "Enable EKF2", &ek2.enable);
	create_param(k_param_NavEKF2, "EK2_GPS_TYPE", "GPS mode control", &ek2.gps_type);
	create_param(k_param_NavEKF2, "EK2_VELNE_M_NSE", "GPS horizontal velocity measurement noise (m/s)", &ek2.velne_m_nse);
	create_param(k_param_NavEKF2, "EK2_VELD_M_NSE", "GPS vertical velocity measurement noise (m/s)", &ek2.veld_m_nse);
	create_param(k_param_NavEKF2, "EK2_VEL_I_GATE", "GPS velocity innovation gate size", &ek2.vel_i_gate);
	create_param(k_param_NavEKF2, "EK2_POSNE_M_NSE", "GPS horizontal position measurement noise (m)", &ek2.posne_m_nse);
	create_param(k_param_NavEKF2, "EK2_POS_I_GATE", "GPS position measurement gate size", &ek2.pos_i_gate);
	create_param(k_param_NavEKF2, "EK2_GLITCH_RAD", "GPS glitch radius gate size (m)", &ek2.glitch_rad);
	create_param(k_param_NavEKF2, "EK2_GPS_DELAY", "GPS measurement delay (msec)", &ek2.gps_delay);
	create_param(k_param_NavEKF2, "EK2_ALT_SOURCE", "Primary height source", &ek2.alt_source);
	create_param(k_param_NavEKF2, "EK2_ALT_M_NSE", "Altitude measurement noise (m)", &ek2.alt_m_nse);
	create_param(k_param_NavEKF2, "EK2_HGT_I_GATE", "Height measurement gate size", &ek2.hgt_i_gate);
	create_param(k_param_NavEKF2, "EK2_HGT_DELAY", "Height measurement delay (msec)", &ek2.hgt_delay);
	create_param(k_param_NavEKF2, "EK2_MAG_M_NSE", "Magnetometer measurement noise (Gauss)", &ek2.mag_m_nse);
	create_param(k_param_NavEKF2, "EK2_MAG_CAL", "Magnetometer calibration mode", &ek2.mag_cal);
	create_param(k_param_NavEKF2, "EK2_MAG_I_GATE", "Magnetometer measurement gate size", &ek2.mag_i_gate);
	create_param(k_param_NavEKF2, "EK2_EAS_M_NSE", "Equivalent airspeed measurement noise", &ek2.eas_m_nse);
	create_param(k_param_NavEKF2, "EK2_EAS_I_GATE", "Equivalent airspeed measurment gate size", &ek2.eas_i_gate);
	create_param(k_param_NavEKF2, "EK2_RNG_M_NSE", "Range finder measurement noise", &ek2.rng_m_nse);
	create_param(k_param_NavEKF2, "EK2_RNG_I_GATE", "Range finder measurement gate size", &ek2.rng_i_gate);
	create_param(k_param_NavEKF2, "EK2_MAX_FLOW", "Maximum valid optical flow rate", &ek2.max_flow);
	create_param(k_param_NavEKF2, "EK2_FLOW_M_NSE", "Optical flow measurement noise (rad/s)", &ek2.flow_m_nse);
	create_param(k_param_NavEKF2, "EK2_FLOW_I_GATE", "Optical flow measurement gate size", &ek2.flow_i_gate);
	create_param(k_param_NavEKF2, "EK2_FLOW_DELAY", "Optical flow measurement delay (msec)", &ek2.flow_delay);
	create_param(k_param_NavEKF2, "EK2_GYRO_P_NSE", "Rate gyro noise (rad/s)", &ek2.gyro_p_nse);
	create_param(k_param_NavEKF2, "EK2_ACC_P_NSE", "Accelerometer noise (m/s^2)", &ek2.acc_p_nse);
	create_param(k_param_NavEKF2, "EK2_GBIAS_P_NSE", "Rate gyro bias stability (rad/s^2)", &ek2.gbias_p_nse);
	create_param(k_param_NavEKF2, "EK2_GSCL_P_NSE", "Rate gyro scale factor stability (1/s)", &ek2.gscl_p_nse);
	create_param(k_param_NavEKF2, "EK2_ABIAS_P_NSE", "Accelerometer bias stability (m/s^3)", &ek2.abias_p_nse);
	create_param(k_param_NavEKF2, "EK2_WIND_P_NSE", "Wind velocity process noise (m/s^2)", &ek2.wind_p_nse);
	create_param(k_param_NavEKF2, "EK2_WIND_PSCALE", "Height rate to wind process noise scaler", &ek2.wind_pscale);
	create_param(k_param_NavEKF2, "EK2_GPS_CHECK", "GPS preflight check", &ek2.gps_check);
	create_param(k_param_NavEKF2, "EK2_IMU_MASK", "Bitmask of active IMUs", &ek2.imu_mask);
	create_param(k_param_NavEKF2, "EK2_CHECK_SCALE", "GPS accuracy check scalere (%)", &ek2.check_scale);
	create_param(k_param_NavEKF2, "EK2_NOAID_M_NSE", "Non-GPS operation position uncertainty (m)", &ek2.noaid_m_nse);
	create_param(k_param_NavEKF2, "EK2_LOG_MASK", "EKF sensor logging IMU mask", &ek2.log_mask);
	create_param(k_param_NavEKF2, "EK2_YAW_M_NSE", "Yaw measurement noise (rad)", &ek2.yaw_m_nse);
	create_param(k_param_NavEKF2, "EK2_YAW_I_GATE", "Yaw measurement gate size", &ek2.yaw_i_gate);
	create_param(k_param_NavEKF2, "EK2_TAU_OUTPUT", "Output complementary filter time constant (centi-sec)", &ek2.tau_output);
	create_param(k_param_NavEKF2, "EK2_MAGE_P_NSE", "Earth magnetic field process noise (gauss/s)", &ek2.mage_p_nse);
	create_param(k_param_NavEKF2, "EK2_MAGB_P_NSE", "Body magnetic field process noise (gauss/s)", &ek2.magb_p_nse);
	create_param(k_param_NavEKF2, "EK2_RNG_USE_HGT", "Range finder switch height percentage", &ek2.rng_use_hgt);
	create_param(k_param_NavEKF2, "EK2_TERR_GRAD", "Maximum terrain gradient", &ek2.terr_grad);
	create_param(k_param_NavEKF2, "EK2_BCN_M_NSE", "Range beacon measurement noise (m)", &ek2.bcn_m_nse);
	create_param(k_param_NavEKF2, "EK2_BCN_I_GTE", "Range beacon measurement gate size", &ek2.bcn_m_nse);
	create_param(k_param_NavEKF2, "EK2_BCN_DELAY", "Range beacon measurement delay (msec)", &ek2.bcn_delay);

	create_param(k_param_mission, "MIS_TOTAL", "total mission commands", &mis.total);
	create_param(k_param_mission, "MIS_RESTART", "Mission Restart when entering Auto mode", &mis.restart);

	create_param(k_param_mission, "NTF_LED_BRIGHT", "LED Brightness", &ntf.led_brright);
	create_param(k_param_mission, "NTF_BUZZ_ENABLE", "Buzz enable", &ntf.buzz_enable);
	create_param(k_param_mission, "NTF_LED_OVERRIDE", "Setup for Mavlink LED override", &ntf.led_override);

	// initializaing hash table
	m_htbl.resize(SIZE_HTBL, -1);
	for (int i = 0; i < m_ptbl.size(); i++){
		int key = hash(m_ptbl[i].str);
		while (m_htbl[key] > 0){
			key++;
			if (key == SIZE_HTBL)
				key = 0;
		}

		m_htbl[key] = i;
		m_ptbl[i].key = key;
		m_ptbl[i].sync = false;
	}

	register_fpar("max_retry_load_param", &max_retry_load_param, "Maximum retry counts loading parameters.");
	register_fpar("sysid", &m_sys_id, "System id in mavlink protocol (default 255)");
	register_fpar("port", &m_port, "UDP port recieving mavlink packets.");
}

f_aws3_com::~f_aws3_com()
{
}

bool f_aws3_com::init_run()
{
  m_sock = socket(AF_INET, SOCK_DGRAM, 0);
  
	m_sock_addr_rcv.sin_family = AF_INET;
	m_sock_addr_rcv.sin_port = htons(m_port);
	set_sockaddr_addr(m_sock_addr_rcv);
	if (::bind(m_sock, (sockaddr*)&m_sock_addr_rcv, sizeof(m_sock_addr_rcv)) == SOCKET_ERROR){
		cerr << "Socket error" << endl;
		return false;
	}

	m_state = INIT;
	m_bcon = false;
	num_retry_load_param = 0;

	return true;
}

void f_aws3_com::destroy_run()
{
	closesocket(m_sock);
}

bool f_aws3_com::proc()
{
	fd_set fr, fw, fe;
	timeval tv;
	int res;
	if (m_bcon){
		/*
		FD_ZERO(&fw);
		FD_ZERO(&fe);
		FD_SET(m_sock, &fw);
		FD_SET(m_sock, &fe);
		tv.tv_sec = 0;
		tv.tv_usec = 1000;
		*/
		mavlink_message_t msg;
		mavlink_status_t status;
		uint16_t len;
		/*Send Heartbeat */
		mavlink_msg_heartbeat_pack(255, 1, &msg, MAV_TYPE_SURFACE_BOAT,
			MAV_AUTOPILOT_GENERIC, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);

		len = mavlink_msg_to_send_buffer(m_buf, &msg);

		res = sendto(m_sock, (char*)m_buf, len, 0, (struct sockaddr*)&m_sock_addr_snd, sizeof(struct sockaddr_in));

		/* Send controller output */
		uint16_t mask = 0x0001, btns = 0x0000;
		for (int i = 0; i < 16; i++, mask <<= 1)
			btns |= (m_jbtns[i] ? mask : 0);

		// saturation -1000 to 1000
		m_jx = (int16_t)max(min((int)m_jx, 1000), -1000);
		m_jy = (int16_t)max(min((int)m_jy, 1000), -1000);
		m_jz = (int16_t)max(min((int)m_jz, 1000), -1000);
		m_jr = (int16_t)max(min((int)m_jr, 1000), -1000);

		mavlink_msg_manual_control_pack(255, 1, &msg, 1,
			(int16_t)m_jx, (int16_t)m_jy, (int16_t)m_jz, (int16_t)m_jr, btns);

		len = mavlink_msg_to_send_buffer(m_buf, &msg);

		res = sendto(m_sock, (char*)m_buf, len, 0, (struct sockaddr*)&m_sock_addr_snd, sizeof(struct sockaddr_in));

		if (m_state == INIT){
			if (!load_parameters())
				return false;
		}
	}

	/* Send Status */
	/*
	mavlink_msg_sys_status_pack(1, 200, &msg, 0, 0, 0, 500, 11000, -1, -1, 0, 0, 0, 0, 0, 0);

	len = mavlink_msg_to_send_buffer(buf, &msg);

	bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof (struct sockaddr_in));
	*/


	/* Send Local Position */
	/*
	mavlink_msg_local_position_ned_pack(1, 200, &msg, microsSinceEpoch(),

	position[0], position[1], position[2],

	position[3], position[4], position[5]);

	len = mavlink_msg_to_send_buffer(buf, &msg);

	bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
	*/


	/* Send attitude */
	/*
	mavlink_msg_attitude_pack(1, 200, &msg, microsSinceEpoch(), 1.2, 1.7, 3.14, 0.01, 0.02, 0.03);

	len = mavlink_msg_to_send_buffer(buf, &msg);

	bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
	*/

	FD_ZERO(&fr);
	FD_ZERO(&fe);
	FD_SET(m_sock, &fr);
	FD_SET(m_sock, &fe);
	tv.tv_sec = 0;
	tv.tv_usec = 1000;

	memset(m_buf, 0, 2048);
	res = select((int)m_sock + 1, &fr, NULL, &fe, &tv);
	if (FD_ISSET(m_sock, &fr)){
		res = recvfrom(m_sock, (char*)m_buf, 1024, 0, (struct sockaddr *)&m_sock_addr_snd, &m_sz);
		if (res > 0)
		{
			// Something received - print out all bytes and parse packet
			mavlink_message_t msg;
			mavlink_status_t status;

			printf("Bytes Received: %d\nDatagram: ", (int)res);
			uint8_t temp;

			for (int i = 0; i < res; ++i)
			{
				temp = m_buf[i];
				printf("%02x ", (unsigned char)temp);
				if (mavlink_parse_char(MAVLINK_COMM_0, m_buf[i], &msg, &status))
				{
					// Packet received
					printf("\nReceived packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
					switch (msg.msgid){
					case MAVLINK_MSG_ID_HEARTBEAT:
						mavlink_msg_heartbeat_decode(&msg, &m_heartbeat);
						break;
					case MAVLINK_MSG_ID_RAW_IMU:
						mavlink_msg_raw_imu_decode(&msg, &m_raw_imu);
						break;
					case MAVLINK_MSG_ID_SCALED_IMU2:
						mavlink_msg_scaled_imu2_decode(&msg, &m_scaled_imu2);
						break;
					case MAVLINK_MSG_ID_SCALED_PRESSURE:
						mavlink_msg_scaled_pressure_decode(&msg, &m_scaled_pressure);
						break;
					case MAVLINK_MSG_ID_SCALED_PRESSURE2:
						mavlink_msg_scaled_pressure2_decode(&msg, &m_scaled_pressure2);
						break;
					case MAVLINK_MSG_ID_SYS_STATUS:
						mavlink_msg_sys_status_decode(&msg, &m_sys_status);
						break;
					case MAVLINK_MSG_ID_POWER_STATUS:
						mavlink_msg_power_status_decode(&msg, &m_power_status);
						break;
					case MAVLINK_MSG_ID_MISSION_CURRENT:
						mavlink_msg_mission_current_decode(&msg, &m_mission_current);
						break;
					case MAVLINK_MSG_ID_SYSTEM_TIME:
						mavlink_msg_system_time_decode(&msg, &m_system_time);
						break;
					case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:
						mavlink_msg_nav_controller_output_decode(&msg, &m_nav_controller_output);
						break;
					case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
						mavlink_msg_global_position_int_decode(&msg, &m_global_position_int);
						break;
					case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:
						mavlink_msg_servo_output_raw_decode(&msg, &m_servo_output_raw);
						break;
					case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
						mavlink_msg_rc_channels_raw_decode(&msg, &m_rc_channels_raw);
						break;
					case MAVLINK_MSG_ID_ATTITUDE:
						mavlink_msg_attitude_decode(&msg, &m_attitude);
						break;
					//case MAVLINK_MSG_ID_RALLY_LAND_FETCH_POINT: //Message ID 178 is not appeared in ardupilot mega...
					case MAVLINK_MSG_ID_VFR_HUD:
						mavlink_msg_vfr_hud_decode(&msg, &m_vfr_hud);
						break;
					case MAVLINK_MSG_ID_HWSTATUS:
						mavlink_msg_hwstatus_decode(&msg, &m_hwstatus);
						break;
					case MAVLINK_MSG_ID_MOUNT_STATUS:
						mavlink_msg_mount_status_decode(&msg, &m_mount_status);
						break;
					case MAVLINK_MSG_ID_EKF_STATUS_REPORT:
						mavlink_msg_ekf_status_report_decode(&msg, &m_ekf_status_report);
						break;
					case MAVLINK_MSG_ID_VIBRATION:
						mavlink_msg_vibration_decode(&msg, &m_vibration);
						break;
					case MAVLINK_MSG_ID_SENSOR_OFFSETS:
						mavlink_msg_sensor_offsets_decode(&msg, &m_sensor_offsets);
						break;
					case MAVLINK_MSG_ID_RANGEFINDER:
						mavlink_msg_rangefinder_decode(&msg, &m_rangefinder);
						break;
					case MAVLINK_MSG_ID_RPM:
						mavlink_msg_rpm_decode(&msg, &m_rpm);
						break;
					case MAVLINK_MSG_ID_CAMERA_FEEDBACK:
						mavlink_msg_camera_feedback_decode(&msg, &m_camera_feedback);
						break;
					case MAVLINK_MSG_ID_LIMITS_STATUS:
						mavlink_msg_limits_status_decode(&msg, &m_limits_status);
						break;
					case MAVLINK_MSG_ID_SIMSTATE:
						mavlink_msg_simstate_decode(&msg, &m_simstate);
						break;
					case MAVLINK_MSG_ID_MEMINFO:
						mavlink_msg_meminfo_decode(&msg, &m_meminfo);
						break;
					case MAVLINK_MSG_ID_BATTERY2:
						mavlink_msg_battery2_decode(&msg, &m_battery2);
						break;
					case MAVLINK_MSG_ID_GIMBAL_REPORT:
						mavlink_msg_gimbal_report_decode(&msg, &m_gimbal_report);
						break;
					case MAVLINK_MSG_ID_PID_TUNING:
						mavlink_msg_pid_tuning_decode(&msg, &m_pid_tuning);
						break;
					case MAVLINK_MSG_ID_MAG_CAL_PROGRESS:
						mavlink_msg_mag_cal_progress_decode(&msg, &m_mag_cal_progress);
						break;
					case MAVLINK_MSG_ID_MAG_CAL_REPORT:
						mavlink_msg_mag_cal_report_decode(&msg, &m_mag_cal_report);
						break;
					case MAVLINK_MSG_ID_AHRS:
						mavlink_msg_ahrs_decode(&msg, &m_ahrs);
						break;
					case MAVLINK_MSG_ID_AHRS2:
						mavlink_msg_ahrs2_decode(&msg, &m_ahrs2);
						break;
					case MAVLINK_MSG_ID_AHRS3:
						mavlink_msg_ahrs3_decode(&msg, &m_ahrs3);
						break;
					case MAVLINK_MSG_ID_STATUSTEXT:
						mavlink_msg_statustext_decode(&msg, &m_statustext);
						handle_statustext();
						break;
					case MAVLINK_MSG_ID_PARAM_VALUE:
						mavlink_msg_param_value_decode(&msg, &m_param_value);
						break;
					}
					
				}
			}
			printf("\n");
			m_bcon = true;
		}
		else{
			cerr << "Error.Reopeninng socket." << endl;
			closesocket(m_sock);
			return init_run();
		}
	}
	else if (FD_ISSET(m_sock, &fe)){
		cerr << "Failed to recieve packet." << endl;
	}

	if (m_brst){
		cout << "Reopening socket." << endl;
		closesocket(m_sock);
		return init_run();
		m_brst = m_bcon = false;
	}
	return true;
}

bool f_aws3_com::load_parameters()
{
	cout << "Loading parameters." << endl;

	fd_set fr, fw, fe;
	timeval tv;
	int res;

	mavlink_message_t msg;
	mavlink_status_t status;
	uint16_t len;

	// issue request list
	mavlink_msg_param_request_list_pack(m_sys_id, 1, &msg, 1, 1);

	len = mavlink_msg_to_send_buffer(m_buf, &msg);

	res = sendto(m_sock, (char*)m_buf, len, 0, (struct sockaddr*)&m_sock_addr_snd, sizeof(struct sockaddr_in));

	m_state = LOAD_PARAM;

	int count_to = 0;
	while (count_to < 50){
		FD_ZERO(&fr);
		FD_ZERO(&fe);
		FD_SET(m_sock, &fr);
		FD_SET(m_sock, &fe);
		tv.tv_sec = 0;
		tv.tv_usec = 1000;

		if (FD_ISSET(m_sock, &fr)){
			res = recvfrom(m_sock, (char*)m_buf, 1024, 0, (struct sockaddr *)&m_sock_addr_snd, &m_sz);
			if (res > 0)
			{
				// Something received - print out all bytes and parse packet
				mavlink_message_t msg;
				mavlink_status_t status;
				uint8_t temp;

				for (int i = 0; i < res; ++i)
				{
					temp = m_buf[i];
					if (mavlink_parse_char(MAVLINK_COMM_0, m_buf[i], &msg, &status))
					{
						switch (msg.msgid){
						case MAVLINK_MSG_ID_STATUSTEXT:
							mavlink_msg_statustext_decode(&msg, &m_statustext);
							handle_statustext();
							break;
						case MAVLINK_MSG_ID_PARAM_VALUE:
							mavlink_msg_param_value_decode(&msg, &m_param_value);
							handle_param_value();
							break;
						}
					}
				}
			}
			else{
				cerr << "Error.Reopeninng socket." << endl;
				closesocket(m_sock);
				return init_run();
			}
		}
		else if (FD_ISSET(m_sock, &fe)){
			cerr << "Failed to recieve packet." << endl;
		}
		count_to++;
	}

	// check paramters;
	bool complete = true;
	for (int i = 0; i < m_ptbl.size(); i++){
		if (!m_ptbl[i].is_sync()){
			cout << "Parameter " << m_ptbl[i].str << " is not synchronized." << endl;
			complete = false;
		}
	}

	if (complete){
		m_state = ACTIVE;
	}
	else{
		m_state = INIT;
		num_retry_load_param++;
		if (num_retry_load_param == max_retry_load_param)
		{
			cerr << "Failed to load parameters." << endl;
			return false;
		}
	}

	return true;
}

void f_aws3_com::handle_param_value()
{
	int key = hash(m_param_value.param_id);
	
	int iparam = seek_param(m_param_value.param_id);
	if (iparam < 0){
		cout << "Unknown parameter ";
		cout.write(m_param_value.param_id, 16);
		cout << "(" <<  (unsigned short) m_param_value.param_index << "/" 
			<< (unsigned short) m_param_value.param_count << ")" <<  endl;
	}
	else{
		m_ptbl[iparam].set(m_param_value.param_value);
	}
}

void f_aws3_com::handle_statustext()
{
	switch (m_statustext.severity)
	{
	case MAV_SEVERITY_EMERGENCY:
		cout << "MAV EMERGENCY:";
		break;
	case MAV_SEVERITY_ALERT:
		cout << "MAV ALERT:";
		break;
	case MAV_SEVERITY_CRITICAL:
		cout << "MAV CRITICAL:";
		break;
	case MAV_SEVERITY_ERROR:
		cout << "MAV ERROR:";
		break;
	case MAV_SEVERITY_WARNING:
		cout << "MAV WORNING:";
		break;
	case MAV_SEVERITY_NOTICE:
		cout << "MAV NOTICE:";
		break;
	case MAV_SEVERITY_INFO:
		cout << "MAV INFO:";
		break;
	case MAV_SEVERITY_DEBUG:
		cout << "MAV_DEBUG:";
		break;
	}
	cout.write(m_statustext.text, 50);
	cout << endl;
}

