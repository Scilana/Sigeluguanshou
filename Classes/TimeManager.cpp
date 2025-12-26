#include "TimeManager.h"
#include <cmath>

TimeManager* TimeManager::instance_ = nullptr;

TimeManager* TimeManager::getInstance()
{
    if (!instance_)
    {
        instance_ = new TimeManager();
    }
    return instance_;
}

void TimeManager::destroyInstance()
{
    if (instance_)
    {
        delete instance_;
        instance_ = nullptr;
    }
}

TimeManager::TimeManager()
    : dayTimer_(0.0f)
    , dayCount_(1)
    , lastFarmUpdateDay_(0)
{
    startNewDay();
}

TimeManager::~TimeManager()
{
}

void TimeManager::startNewDay()
{
    // Start at 6:00 AM
    // 24 hours = SECONDS_PER_DAY
    // 6 hours = SECONDS_PER_DAY * (6 / 24) = 300 * 0.25 = 75.0f
    dayTimer_ = SECONDS_PER_DAY * (START_HOUR / 24.0f);
}

void TimeManager::advanceToNextDay()
{
    dayCount_++;
    startNewDay();
}

void TimeManager::skipToNextMorning()
{
    // Jump time to just before 6:00 AM the next day (e.g. 5:59 AM next day)
    // 6:00 AM relative to current day start is:
    // Midnight (SECONDS_PER_DAY) + 6h (SECONDS_PER_DAY * 6/24)
    float nextMorning = SECONDS_PER_DAY + (SECONDS_PER_DAY * (START_HOUR / 24.0f));
    
    // Set slightly before to ensure update loop catches the trigger point
    dayTimer_ = nextMorning - 0.2f; 
}


void TimeManager::update(float dt)
{
    dayTimer_ += dt;
}

int TimeManager::getHour() const
{
    // timer / total * 24
    float totalHours = (dayTimer_ / SECONDS_PER_DAY) * 24.0f;
    return static_cast<int>(totalHours) % 24;
}

int TimeManager::getMinute() const
{
    float totalHours = (dayTimer_ / SECONDS_PER_DAY) * 24.0f;
    float minutes = (totalHours - static_cast<int>(totalHours)) * 60.0f;
    return static_cast<int>(minutes) % 60;
}

int TimeManager::getDay() const
{
    return dayCount_;
}

float TimeManager::getDayProgress() const
{
    return dayTimer_ / SECONDS_PER_DAY;
}

void TimeManager::setDayCount(int day)
{
    dayCount_ = day;
}

bool TimeManager::isMidnight() const
{
    // Midnight is when timer reaches SECONDS_PER_DAY (24:00 which is 0:00 next day contextually for sleep)
    // However, usually games allow slightly past midnight.
    // The requirement says "restore 12:00 AM forced sleep".
    // 12 AM is 0:00 (Start of day) or 24:00 (End of day).
    // Our timer goes from 6:00 AM -> 24:00/0:00 AM.
    // So if timer >= SECONDS_PER_DAY, it is midnight.
    return dayTimer_ >= SECONDS_PER_DAY;
}
