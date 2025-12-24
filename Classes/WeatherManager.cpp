#include "WeatherManager.h"

USING_NS_CC;

namespace {
constexpr float kSunOffset = 100.0f;
constexpr float kSunnySpeed = 0.0f;
constexpr float kLightRainSpeed = 500.0f;
constexpr float kLightRainSpeedVar = 50.0f;
constexpr float kLightRainSize = 10.0f;
constexpr float kLightRainSizeEnd = 5.0f;
constexpr float kHeavyRainSpeed = 800.0f;
constexpr float kHeavyRainSpeedVar = 100.0f;
constexpr float kHeavyRainSize = 15.0f;
constexpr float kHeavyRainSizeEnd = 10.0f;
constexpr float kSnowySpeed = 100.0f;
constexpr float kSnowySpeedVar = 20.0f;
constexpr float kSnowySize = 10.0f;
constexpr float kSnowySizeEnd = 5.0f;
}

WeatherManager::WeatherManager()
    : currentWeatherSystem_(nullptr), backgroundMask_(nullptr) {
}

WeatherManager::~WeatherManager() {
    removeWeatherEffect();
}

WeatherManager* WeatherManager::create() {
    WeatherManager* weatherManager = new (std::nothrow) WeatherManager();
    if (weatherManager && weatherManager->init()) {
        weatherManager->autorelease();
        return weatherManager;
    }
    CC_SAFE_DELETE(weatherManager);
    return nullptr;
}

void WeatherManager::updateWeather(MarketState::Weather weather) {
    removeWeatherEffect();

    const auto visibleSize = Director::getInstance()->getVisibleSize();

    switch (weather) {
    case MarketState::Weather::Sunny: {
        currentWeatherSystem_ = ParticleSun::create();
        currentWeatherSystem_->setPosition(visibleSize.width - kSunOffset, visibleSize.height - kSunOffset);
        currentWeatherSystem_->setStartColor(Color4F(1.0f, 0.9f, 0.5f, 1.0f));
        currentWeatherSystem_->setEndColor(Color4F(1.0f, 0.8f, 0.3f, 0.8f));
        currentWeatherSystem_->setSpeed(kSunnySpeed);
        break;
    }
    case MarketState::Weather::LightRain: {
        currentWeatherSystem_ = ParticleRain::create();
        currentWeatherSystem_->setPosition(visibleSize.width / 2, visibleSize.height);
        currentWeatherSystem_->setStartColor(Color4F(0.4f, 0.4f, 1.0f, 1.0f));
        currentWeatherSystem_->setEndColor(Color4F(0.2f, 0.2f, 0.8f, 0.8f));
        currentWeatherSystem_->setSpeed(kLightRainSpeed);
        currentWeatherSystem_->setSpeedVar(kLightRainSpeedVar);
        currentWeatherSystem_->setStartSize(kLightRainSize);
        currentWeatherSystem_->setEndSize(kLightRainSizeEnd);

        backgroundMask_ = LayerColor::create(Color4B(0, 0, 0, 32), visibleSize.width, visibleSize.height);
        addChild(backgroundMask_, -1);
        break;
    }
    case MarketState::Weather::HeavyRain: {
        currentWeatherSystem_ = ParticleRain::create();
        currentWeatherSystem_->setPosition(visibleSize.width / 2, visibleSize.height);
        currentWeatherSystem_->setStartColor(Color4F(0.4f, 0.4f, 1.0f, 1.0f));
        currentWeatherSystem_->setEndColor(Color4F(0.2f, 0.2f, 0.8f, 0.8f));
        currentWeatherSystem_->setTotalParticles(600);
        currentWeatherSystem_->setSpeed(kHeavyRainSpeed);
        currentWeatherSystem_->setSpeedVar(kHeavyRainSpeedVar);
        currentWeatherSystem_->setStartSize(kHeavyRainSize);
        currentWeatherSystem_->setEndSize(kHeavyRainSizeEnd);

        backgroundMask_ = LayerColor::create(Color4B(0, 0, 0, 64), visibleSize.width, visibleSize.height);
        addChild(backgroundMask_, -1);
        break;
    }
    case MarketState::Weather::Snowy: {
        currentWeatherSystem_ = ParticleSnow::create();
        currentWeatherSystem_->setPosition(visibleSize.width / 2, visibleSize.height);
        currentWeatherSystem_->setStartColor(Color4F(1.0f, 1.0f, 1.0f, 1.0f));
        currentWeatherSystem_->setEndColor(Color4F(0.8f, 0.8f, 0.8f, 0.8f));
        currentWeatherSystem_->setSpeed(kSnowySpeed);
        currentWeatherSystem_->setSpeedVar(kSnowySpeedVar);
        currentWeatherSystem_->setStartSize(kSnowySize);
        currentWeatherSystem_->setEndSize(kSnowySizeEnd);
        break;
    }
    }

    if (currentWeatherSystem_) {
        addChild(currentWeatherSystem_);
    }
}

void WeatherManager::removeWeatherEffect() {
    if (currentWeatherSystem_) {
        removeChild(currentWeatherSystem_);
        currentWeatherSystem_ = nullptr;
    }

    if (backgroundMask_) {
        removeChild(backgroundMask_);
        backgroundMask_ = nullptr;
    }
}
