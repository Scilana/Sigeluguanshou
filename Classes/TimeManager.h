#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__

class TimeManager
{
public:
    static TimeManager* getInstance();
    static void destroyInstance();

    void update(float dt);
    
    // Time getters
    int getHour() const;
    int getMinute() const;
    int getDay() const;
    float getDayProgress() const; // 0.0 to 1.0
    
    // Life cycle
    void startNewDay(); // Resets time to 6 AM
    void advanceToNextDay(); // Increments day count and starts new day
    void setDayCount(int day);
    int getLastFarmUpdateDay() const { return lastFarmUpdateDay_; }
    void setLastFarmUpdateDay(int day) { lastFarmUpdateDay_ = day; }
    
    // Cheat helper
    void skipToNextMorning();

    
    // State checks
    bool isMidnight() const;

private:
    TimeManager();
    ~TimeManager();
    
    static TimeManager* instance_;

    float dayTimer_;      // Current seconds passed in day
    int dayCount_;        // Current day number
    int lastFarmUpdateDay_; // Track when farm was last updated

    
    // Constants
    const float SECONDS_PER_DAY = 300.0f; // 5 minutes real time = 1 day
    const float START_HOUR = 6.0f;        // 6:00 AM
};

#endif // __TIME_MANAGER_H__
