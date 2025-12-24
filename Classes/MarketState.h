#ifndef __MARKET_STATE_H__
#define __MARKET_STATE_H__

#include "InventoryManager.h"
#include <string>
#include <vector>

class MarketState {
public:
    struct MarketGood {
        ItemType itemType;
        int basePrice;
        int currentPrice;
    };

    void init();
    void updatePrices(int dayCount);

    const std::vector<MarketGood>& getBuyGoods() const { return buyGoods_; }
    const std::vector<MarketGood>& getSellGoods() const { return sellGoods_; }

    int getBuyPrice(ItemType itemType) const;
    int getSellPrice(ItemType itemType) const;

    std::string getSeasonName() const;
    std::string getWeatherName() const;
    enum class Weather { Sunny, LightRain, HeavyRain, Snowy };
    Weather getWeather() const { return weather_; }

private:
    enum class Season { Spring, Summer, Fall, Winter };

    void buildDefaultGoods();
    void updateContext(int dayCount);
    void adjustGoodPrice(MarketGood& good) const;

    std::vector<MarketGood> buyGoods_;
    std::vector<MarketGood> sellGoods_;
    int dayCount_ = 0;
    Season season_ = Season::Spring;
    Weather weather_ = Weather::Sunny;
};

#endif // __MARKET_STATE_H__
