
float FarmManager::getDayProgress() const
{
    auto tm = TimeManager::getInstance();
    return tm ? tm->getDayProgress() : 0.0f;
}
