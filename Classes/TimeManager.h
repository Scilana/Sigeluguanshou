#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__

class TimeManager {
 public:
  static TimeManager* getInstance();
  static void destroyInstance();

  void update(float dt);

  int getHour() const;
  int getMinute() const;
  int getDay() const;
  float getDayProgress() const;

  void startNewDay();
  void advanceToNextDay();
  void setDayCount(int day);
  int getLastFarmUpdateDay() const { return lastFarmUpdateDay_; }
  void setLastFarmUpdateDay(int day) { lastFarmUpdateDay_ = day; }

  void skipToNextMorning();

  bool isMidnight() const;

 private:
  TimeManager();
  ~TimeManager();

  static TimeManager* instance_;

  float dayTimer_;
  int dayCount_;
  int lastFarmUpdateDay_;

  const float SECONDS_PER_DAY = 300.0f;
  const float START_HOUR = 6.0f;
};

#endif
