#include <date/date.h>
#include <lvgl/lvgl.h>
#include "displayapp/screens/WatchFaceTerminal.h"
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"

using namespace Pinetime::Applications::Screens;

WatchFaceTerminal::WatchFaceTerminal(DisplayApp* app,
                                     Controllers::DateTime& dateTimeController,
                                     Controllers::Battery& batteryController,
                                     Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificatioManager,
                                     Controllers::Settings& settingsController,
                                     Controllers::HeartRateController& heartRateController,
                                     Controllers::MotionController& motionController)
  : Screen(app),
    currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificatioManager {notificatioManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {
  settingsController.SetClockFace(3);

  batteryIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(batteryIcon, Symbols::batteryFull);
  lv_obj_align(batteryIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, -5, 2);

  batteryPlug = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(batteryPlug, Symbols::plug);
  lv_obj_align(batteryPlug, batteryIcon, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  batteryValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(batteryValue, true);
  lv_obj_align(batteryValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -20);

  connectState = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(connectState, true);
  lv_label_set_text(connectState, "[STAT]#387b54 Disconnected#");
  lv_obj_align(connectState, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 40);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 10, 0);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_date, true);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -40);

  label_prompt_1 = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_prompt_1, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -80);
  lv_label_set_text(label_prompt_1, "user@watch:~ $ now");

  label_prompt_2 = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_prompt_2, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 60);
  lv_label_set_text(label_prompt_2, "user@watch:~ $");

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_time, true);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -60);

  backgroundLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_click(backgroundLabel, true);
  lv_label_set_long_mode(backgroundLabel, LV_LABEL_LONG_CROP);
  lv_obj_set_size(backgroundLabel, 240, 240);
  lv_obj_set_pos(backgroundLabel, 0, 0);
  lv_label_set_text(backgroundLabel, "");

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(heartbeatValue, true);
  lv_label_set_text(heartbeatValue, "[L_HR]#ee3311 0 bpm#");
  lv_obj_align(heartbeatValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 20);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(stepValue, true);
  lv_label_set_text(stepValue, "[STEP]#ee3377 0 steps#");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 0);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceTerminal::~WatchFaceTerminal() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceTerminal::Refresh() {
  powerPresent = batteryController.IsPowerPresent();
  if (powerPresent.IsUpdated()) {
    lv_label_set_text_static(batteryPlug, BatteryIcon::GetPlugIcon(powerPresent.Get()));
  }

  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated()) {
    auto batteryPercent = batteryPercentRemaining.Get();
    if (batteryPercent == 100) {
      lv_obj_set_style_local_text_color(batteryIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
    } else {
      lv_obj_set_style_local_text_color(batteryIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    }
    lv_label_set_text(batteryIcon, BatteryIcon::GetBatteryIcon(batteryPercent));
    lv_label_set_text_fmt(batteryValue, "[BATT]#387b54 %d%\%#", batteryPercent);
  }

  bleState = bleController.IsConnected();
  if (bleState.IsUpdated()) {
    if (bleState.Get()) {
      lv_label_set_text_static(connectState, "[STAT]#387b54 Connected#");
    } else {
      lv_label_set_text_static(connectState, "[STAT]#387b54 Disconnected#");
    }
  }

  notificationState = notificatioManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    if (notificationState.Get()) {
      lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(true));
    } else {
      lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(false));
    }
  }

  currentDateTime = dateTimeController.CurrentDateTime();

  if (currentDateTime.IsUpdated()) {
    auto newDateTime = currentDateTime.Get();

    auto dp = date::floor<date::days>(newDateTime);
    auto time = date::make_time(newDateTime - dp);
    auto yearMonthDay = date::year_month_day(dp);

    auto year = static_cast<int>(yearMonthDay.year());
    auto month = static_cast<Pinetime::Controllers::DateTime::Months>(static_cast<unsigned>(yearMonthDay.month()));
    auto day = static_cast<unsigned>(yearMonthDay.day());
    auto dayOfWeek = static_cast<Pinetime::Controllers::DateTime::Days>(date::weekday(yearMonthDay).iso_encoding());

    uint8_t hour = time.hours().count();
    uint8_t minute = time.minutes().count();
    uint8_t second = time.seconds().count();

    if (displayedHour != hour || displayedMinute != minute || displayedSecond != second) {
      displayedHour = hour;
      displayedMinute = minute;
      displayedSecond = second;

      if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
        char ampmChar[3] = "AM";
        if (hour == 0) {
          hour = 12;
        } else if (hour == 12) {
          ampmChar[0] = 'P';
        } else if (hour > 12) {
          hour = hour - 12;
          ampmChar[0] = 'P';
        }
        lv_label_set_text_fmt(label_time, "[TIME]#11cc55 %02d:%02d:%02d %s#", hour, minute, second, ampmChar);
      } else {
        lv_label_set_text_fmt(label_time, "[TIME]#11cc55 %02d:%02d:%02d", hour, minute, second);
      }
    }

    if ((year != currentYear) || (month != currentMonth) || (dayOfWeek != currentDayOfWeek) || (day != currentDay)) {
      lv_label_set_text_fmt(label_date, "[DATE]#007fff %04d.%02d.%02d#", short(year), char(month), char(day));

      currentYear = year;
      currentMonth = month;
      currentDayOfWeek = dayOfWeek;
      currentDay = day;
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "[L_HR]#ee3311 %d bpm#", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "[L_HR]#ee3311 ---#");
    }
  }

  stepCount = motionController.NbSteps();
  motionSensorOk = motionController.IsSensorOk();
  if (stepCount.IsUpdated() || motionSensorOk.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "[STEP]#ee3377 %lu steps#", stepCount.Get());
  }
}