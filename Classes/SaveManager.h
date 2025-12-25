#ifndef __SAVE_MANAGER_H__
#define __SAVE_MANAGER_H__

#include "cocos2d.h"
#include "InventoryManager.h"
#include "json/document.h"
#include <string>
#include <vector>

/**
 * @brief 存档管理器
 *
 * 职责：
 * - 保存和加载游戏存档
 * - 管理存档文件
 * - 序列化和反序列化游戏数据
 */
class SaveManager
{
public:
    /**
     * @brief 游戏存档数据结构
     */
    struct SaveData
    {
        // 玩家数据
        cocos2d::Vec2 playerPosition;

        // 背包数据
        struct InventoryData
        {
            struct ItemSlotData
            {
                int type;  // ItemType 枚举值
                int count;
            };
            std::vector<ItemSlotData> slots;
            int money;
        } inventory;

        // 游戏时间数据
        int dayCount;

        // 农作物数据
        struct FarmTileData
        {
            int x;
            int y;
            bool tilled;
            bool watered;
            bool hasCrop;
            int cropId;
            int stage;
            int progressDays;
        };
        std::vector<FarmTileData> farmTiles;

        // 储物箱数据
        struct StorageChestData
        {
            int x;
            int y;
            struct SlotData
            {
                int type;
                int count;
            };
            std::vector<SlotData> slots;
        };
        std::vector<StorageChestData> storageChests;
    };

    /**
     * @brief 获取单例实例
     */
    static SaveManager* getInstance();

    /**
     * @brief 保存游戏
     * @param data 游戏数据
     * @return 是否保存成功
     */
    bool saveGame(const SaveData& data);

    /**
     * @brief 加载游戏
     * @param data 输出参数，加载的游戏数据
     * @return 是否加载成功
     */
    bool loadGame(SaveData& data);

    /**
     * @brief 检查是否存在存档
     * @return 是否存在存档文件
     */
    bool hasSaveFile() const;

    /**
     * @brief 删除存档文件
     */
    void deleteSaveFile();

private:
    SaveManager();
    ~SaveManager();

    static SaveManager* instance_;

    /**
     * @brief 获取存档文件路径
     */
    std::string getSaveFilePath() const;

    /**
     * @brief 序列化存档数据为 JSON
     */
    rapidjson::Document serializeToJson(const SaveData& data);

    /**
     * @brief 从 JSON 反序列化存档数据
     */
    bool deserializeFromJson(const rapidjson::Document& doc, SaveData& data);
};

#endif // __SAVE_MANAGER_H__
