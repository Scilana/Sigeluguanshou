#include "MarketState.h"
#include <algorithm>

namespace {
const int kSunnyInfluence = 0;
const int kLightRainInfluence = 10;
const int kHeavyRainInfluence = 30;
const int kSnowyInfluence = 50;
}

void MarketState::init()
{
    buildDefaultGoods();
    updatePrices(1);
}

void MarketState::buildDefaultGoods()
{
    buyGoods_.clear();
    sellGoods_.clear();

    // Seeds (buy)
    buyGoods_.push_back({ItemType::SeedTurnip, 20, 20});
    buyGoods_.push_back({ItemType::SeedPotato, 30, 30});
    buyGoods_.push_back({ItemType::SeedCorn, 40, 40});
    buyGoods_.push_back({ItemType::SeedTomato, 35, 35});
    buyGoods_.push_back({ItemType::SeedPumpkin, 60, 60});
    buyGoods_.push_back({ItemType::SeedBlueberry, 45, 45});

    // Crops (sell)
    sellGoods_.push_back({ItemType::Turnip, 60, 60});
    sellGoods_.push_back({ItemType::Potato, 80, 80});
    sellGoods_.push_back({ItemType::Corn, 120, 120});
    sellGoods_.push_back({ItemType::Tomato, 90, 90});
    sellGoods_.push_back({ItemType::Pumpkin, 180, 180});
    sellGoods_.push_back({ItemType::Blueberry, 110, 110});

    // Fish
    sellGoods_.push_back({ItemType::Fish, 50, 50});
    sellGoods_.push_back({ItemType::ITEM_Anchovy, 30, 30});
    sellGoods_.push_back({ItemType::ITEM_Carp, 30, 30});
    sellGoods_.push_back({ItemType::ITEM_Eel, 85, 85});
    sellGoods_.push_back({ItemType::ITEM_Flounder, 50, 50});
    sellGoods_.push_back({ItemType::ITEM_Largemouth_Bass, 100, 100});
    sellGoods_.push_back({ItemType::ITEM_Pufferfish, 200, 200});
    sellGoods_.push_back({ItemType::ITEM_Rainbow_Trout, 65, 65});
    sellGoods_.push_back({ItemType::ITEM_Sturgeon, 200, 200});
    sellGoods_.push_back({ItemType::ITEM_Tilapia, 75, 75});

    // Ores (Fixed Prices)
    sellGoods_.push_back({ItemType::CopperOre, 20, 20});
    sellGoods_.push_back({ItemType::IronOre, 50, 50});
    sellGoods_.push_back({ItemType::SilverOre, 100, 100});
    sellGoods_.push_back({ItemType::GoldOre, 250, 250});
    sellGoods_.push_back({ItemType::DiamondOre, 500, 500});
}

void MarketState::updatePrices(int dayCount)
{
    if (dayCount <= 0) {
        dayCount = 1;
    }

    updateContext(dayCount);

    for (auto& good : buyGoods_) {
        adjustGoodPrice(good);
    }
    for (auto& good : sellGoods_) {
        adjustGoodPrice(good);
    }
}

void MarketState::updateContext(int dayCount)
{
    dayCount_ = dayCount;

    int seasonIndex = ((dayCount_ - 1) / 7) % 4;
    switch (seasonIndex) {
    case 0: season_ = Season::Spring; break;
    case 1: season_ = Season::Summer; break;
    case 2: season_ = Season::Fall; break;
    default: season_ = Season::Winter; break;
    }

    int weatherIndex = (dayCount_ * 3) % 4;
    switch (weatherIndex) {
    case 0: weather_ = Weather::Sunny; break;
    case 1: weather_ = Weather::LightRain; break;
    case 2: weather_ = Weather::HeavyRain; break;
    default: weather_ = Weather::Snowy; break;
    }
}

void MarketState::adjustGoodPrice(MarketGood& good) const
{
    int price = good.basePrice;

    // Fixed price items (Ores) - do not fluctuate
    if (good.itemType == ItemType::CopperOre ||
        good.itemType == ItemType::IronOre ||
        good.itemType == ItemType::SilverOre ||
        good.itemType == ItemType::GoldOre ||
        good.itemType == ItemType::DiamondOre)
    {
        good.currentPrice = price;
        return;
    }

    switch (season_) {
    case Season::Spring:
        price = good.basePrice;
        break;
    case Season::Summer:
        price = good.basePrice / 2;
        break;
    case Season::Fall:
        price = good.basePrice * 3 / 2;
        break;
    case Season::Winter:
        price = good.basePrice * 2;
        break;
    }

    switch (weather_) {
    case Weather::Sunny:
        price += kSunnyInfluence;
        break;
    case Weather::LightRain:
        price += kLightRainInfluence;
        break;
    case Weather::HeavyRain:
        price += kHeavyRainInfluence;
        break;
    case Weather::Snowy:
        price += kSnowyInfluence;
        break;
    }

    good.currentPrice = std::max(1, price);
}

int MarketState::getBuyPrice(ItemType itemType) const
{
    for (const auto& good : buyGoods_) {
        if (good.itemType == itemType) {
            return good.currentPrice;
        }
    }
    return -1;
}

int MarketState::getSellPrice(ItemType itemType) const
{
    for (const auto& good : sellGoods_) {
        if (good.itemType == itemType) {
            return good.currentPrice;
        }
    }
    return -1;
}

std::string MarketState::getSeasonName() const
{
    switch (season_) {
    case Season::Spring: return "Spring";
    case Season::Summer: return "Summer";
    case Season::Fall: return "Fall";
    case Season::Winter: return "Winter";
    default: return "Unknown";
    }
}

std::string MarketState::getWeatherName() const
{
    return getWeatherName(weather_);
}

MarketState::Weather MarketState::predictWeather(int dayCount)
{
    if (dayCount <= 0) dayCount = 1;
    int weatherIndex = (dayCount * 3) % 4;
    switch (weatherIndex) {
    case 0: return Weather::Sunny;
    case 1: return Weather::LightRain;
    case 2: return Weather::HeavyRain;
    default: return Weather::Snowy;
    }
}

std::string MarketState::getWeatherName(Weather weather)
{
    switch (weather) {
    case Weather::Sunny: return "Sunny";
    case Weather::LightRain: return "Light Rain";
    case Weather::HeavyRain: return "Heavy Rain";
    case Weather::Snowy: return "Snowy";
    default: return "Unknown";
    }
}
