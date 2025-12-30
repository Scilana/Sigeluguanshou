
int FarmManager::getDayCount() const {
  return TimeManager::getInstance()->getDay();
}

void FarmManager::setDayCount(int dayCount) {
  TimeManager::getInstance()->setDayCount(dayCount);
}
