
int FarmManager::getDayCount() const
{
    // Delegate to TimeManager
    return TimeManager::getInstance()->getDay();
}

void FarmManager::setDayCount(int dayCount)
{
    // Delegate to TimeManager
    TimeManager::getInstance()->setDayCount(dayCount);
}
