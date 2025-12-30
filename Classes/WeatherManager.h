#ifndef __WEATHER_MANAGER_H__
#define __WEATHER_MANAGER_H__

#include "MarketState.h"
#include "cocos2d.h"

class WeatherManager : public cocos2d::Node {
 public:
  static WeatherManager* create();
  void updateWeather(MarketState::Weather weather);
  void removeWeatherEffect();

 private:
  WeatherManager();
  ~WeatherManager();

  cocos2d::ParticleSystem* currentWeatherSystem_;
  cocos2d::LayerColor* backgroundMask_;
};

#endif
