#include "TimeManager.h"

#include <cmath>

TimeManager* TimeManager::instance_ = nullptr;

TimeManager* TimeManager::getInstance() {
  if (!instance_) {
    instance_ = new TimeManager();
  }
  return instance_;
}

void TimeManager::destroyInstance() {
  if (instance_) {
    delete instance_;
    instance_ = nullptr;
  }
}

TimeManager::TimeManager()
    : dayTimer_(0.0f), dayCount_(1), lastFarmUpdateDay_(0) {
  startNewDay();
}

TimeManager::~TimeManager() {}

void TimeManager::startNewDay() {
  dayTimer_ = SECONDS_PER_DAY * (START_HOUR / 24.0f);
}

void TimeManager::advanceToNextDay() {
  dayCount_++;
  startNewDay();
}

void TimeManager::skipToNextMorning() {
  float nextMorning =
      SECONDS_PER_DAY + (SECONDS_PER_DAY * (START_HOUR / 24.0f));

  dayTimer_ = nextMorning - 0.2f;
}

void TimeManager::update(float dt) { dayTimer_ += dt; }

int TimeManager::getHour() const {
  float totalHours = (dayTimer_ / SECONDS_PER_DAY) * 24.0f;
  return static_cast<int>(totalHours) % 24;
}

int TimeManager::getMinute() const {
  float totalHours = (dayTimer_ / SECONDS_PER_DAY) * 24.0f;
  float minutes = (totalHours - static_cast<int>(totalHours)) * 60.0f;
  return static_cast<int>(minutes) % 60;
}

int TimeManager::getDay() const { return dayCount_; }

float TimeManager::getDayProgress() const {
  return dayTimer_ / SECONDS_PER_DAY;
}

void TimeManager::setDayCount(int day) { dayCount_ = day; }

bool TimeManager::isMidnight() const { return dayTimer_ >= SECONDS_PER_DAY; }
