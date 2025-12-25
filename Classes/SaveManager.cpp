#include "SaveManager.h"
#include "json/writer.h"
#include "json/stringbuffer.h"
#include "json/prettywriter.h"
#include "platform/CCFileUtils.h"

USING_NS_CC;

SaveManager* SaveManager::instance_ = nullptr;

SaveManager::SaveManager()
{
}

SaveManager::~SaveManager()
{
}

SaveManager* SaveManager::getInstance()
{
    if (instance_ == nullptr)
    {
        instance_ = new SaveManager();
    }
    return instance_;
}

std::string SaveManager::getSaveFilePath() const
{
    // 将存档保存在可写目录
    std::string writablePath = FileUtils::getInstance()->getWritablePath();
    return writablePath + "savegame.json";
}

bool SaveManager::hasSaveFile() const
{
    std::string path = getSaveFilePath();
    return FileUtils::getInstance()->isFileExist(path);
}

void SaveManager::deleteSaveFile()
{
    std::string path = getSaveFilePath();
    if (FileUtils::getInstance()->isFileExist(path))
    {
        FileUtils::getInstance()->removeFile(path);
        CCLOG("Save file deleted: %s", path.c_str());
    }
}

rapidjson::Document SaveManager::serializeToJson(const SaveData& data)
{
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    // 保存玩家位置
    rapidjson::Value playerPos(rapidjson::kObjectType);
    playerPos.AddMember("x", data.playerPosition.x, allocator);
    playerPos.AddMember("y", data.playerPosition.y, allocator);
    doc.AddMember("playerPosition", playerPos, allocator);

    // 保存背包数据
    rapidjson::Value inventoryObj(rapidjson::kObjectType);

    // 保存物品槽位
    rapidjson::Value slotsArray(rapidjson::kArrayType);
    for (const auto& slot : data.inventory.slots)
    {
        rapidjson::Value slotObj(rapidjson::kObjectType);
        slotObj.AddMember("type", slot.type, allocator);
        slotObj.AddMember("count", slot.count, allocator);
        slotsArray.PushBack(slotObj, allocator);
    }
    inventoryObj.AddMember("slots", slotsArray, allocator);
    inventoryObj.AddMember("money", data.inventory.money, allocator);
    doc.AddMember("inventory", inventoryObj, allocator);

    // 保存游戏天数
    doc.AddMember("dayCount", data.dayCount, allocator);

    // 保存农作物数据
    rapidjson::Value farmTilesArray(rapidjson::kArrayType);
    for (const auto& tile : data.farmTiles)
    {
        // 只保存有状态的瓦片（耕地或有作物）
        if (tile.tilled || tile.hasCrop)
        {
            rapidjson::Value tileObj(rapidjson::kObjectType);
            tileObj.AddMember("x", tile.x, allocator);
            tileObj.AddMember("y", tile.y, allocator);
            tileObj.AddMember("tilled", tile.tilled, allocator);
            tileObj.AddMember("watered", tile.watered, allocator);
            tileObj.AddMember("hasCrop", tile.hasCrop, allocator);
            tileObj.AddMember("cropId", tile.cropId, allocator);
            tileObj.AddMember("stage", tile.stage, allocator);
            tileObj.AddMember("progressDays", tile.progressDays, allocator);
            farmTilesArray.PushBack(tileObj, allocator);
        }
    }
    doc.AddMember("farmTiles", farmTilesArray, allocator);

    // 保存储物箱数据
    rapidjson::Value storageChestsArray(rapidjson::kArrayType);
    for (const auto& chest : data.storageChests)
    {
        rapidjson::Value chestObj(rapidjson::kObjectType);
        chestObj.AddMember("x", chest.x, allocator);
        chestObj.AddMember("y", chest.y, allocator);
        
        rapidjson::Value slotsArray(rapidjson::kArrayType);
        for (const auto& slot : chest.slots)
        {
            rapidjson::Value slotObj(rapidjson::kObjectType);
            slotObj.AddMember("type", slot.type, allocator);
            slotObj.AddMember("count", slot.count, allocator);
            slotsArray.PushBack(slotObj, allocator);
        }
        chestObj.AddMember("slots", slotsArray, allocator);
        storageChestsArray.PushBack(chestObj, allocator);
    }
    doc.AddMember("storageChests", storageChestsArray, allocator);

    // 树木存档已禁用（避免崩溃）

    return doc;
}

bool SaveManager::deserializeFromJson(const rapidjson::Document& doc, SaveData& data)
{
    try
    {
        // 加载玩家位置
        if (doc.HasMember("playerPosition") && doc["playerPosition"].IsObject())
        {
            const auto& pos = doc["playerPosition"];
            data.playerPosition.x = pos["x"].GetFloat();
            data.playerPosition.y = pos["y"].GetFloat();
        }
        else
        {
            CCLOG("Warning: playerPosition not found in save file");
            data.playerPosition = Vec2::ZERO;
        }

        // 加载背包数据
        if (doc.HasMember("inventory") && doc["inventory"].IsObject())
        {
            const auto& inv = doc["inventory"];

            // 加载物品槽位
            data.inventory.slots.clear();
            if (inv.HasMember("slots") && inv["slots"].IsArray())
            {
                const auto& slotsArray = inv["slots"].GetArray();
                for (rapidjson::SizeType i = 0; i < slotsArray.Size(); i++)
                {
                    const auto& slotObj = slotsArray[i];
                    SaveData::InventoryData::ItemSlotData slot;
                    slot.type = slotObj["type"].GetInt();
                    slot.count = slotObj["count"].GetInt();
                    data.inventory.slots.push_back(slot);
                }
            }

            // 加载金币
            if (inv.HasMember("money"))
            {
                data.inventory.money = inv["money"].GetInt();
            }
        }

        // 加载游戏天数
        if (doc.HasMember("dayCount"))
        {
            data.dayCount = doc["dayCount"].GetInt();
        }
        else
        {
            data.dayCount = 1;
        }

        // 加载农作物数据
        data.farmTiles.clear();
        if (doc.HasMember("farmTiles") && doc["farmTiles"].IsArray())
        {
            const auto& tilesArray = doc["farmTiles"].GetArray();
            for (rapidjson::SizeType i = 0; i < tilesArray.Size(); i++)
            {
                const auto& tileObj = tilesArray[i];
                SaveData::FarmTileData tile;
                tile.x = tileObj["x"].GetInt();
                tile.y = tileObj["y"].GetInt();
                tile.tilled = tileObj["tilled"].GetBool();
                tile.watered = tileObj["watered"].GetBool();
                tile.hasCrop = tileObj["hasCrop"].GetBool();
                tile.cropId = tileObj["cropId"].GetInt();
                tile.stage = tileObj["stage"].GetInt();
                tile.progressDays = tileObj["progressDays"].GetInt();
                data.farmTiles.push_back(tile);
            }
        }

        // 加载储物箱数据
        data.storageChests.clear();
        if (doc.HasMember("storageChests") && doc["storageChests"].IsArray())
        {
            const auto& chestsArray = doc["storageChests"].GetArray();
            for (rapidjson::SizeType i = 0; i < chestsArray.Size(); i++)
            {
                const auto& chestObj = chestsArray[i];
                SaveData::StorageChestData chest;
                chest.x = chestObj["x"].GetInt();
                chest.y = chestObj["y"].GetInt();
                
                if (chestObj.HasMember("slots") && chestObj["slots"].IsArray())
                {
                    const auto& slotsArray = chestObj["slots"].GetArray();
                    for (rapidjson::SizeType j = 0; j < slotsArray.Size(); j++)
                    {
                        const auto& slotObj = slotsArray[j];
                        SaveData::StorageChestData::SlotData slot;
                        slot.type = slotObj["type"].GetInt();
                        slot.count = slotObj["count"].GetInt();
                        chest.slots.push_back(slot);
                    }
                }
                data.storageChests.push_back(chest);
            }
        }

        // 树木存档已禁用（避免崩溃）

        return true;
    }
    catch (...)
    {
        CCLOG("Error: Failed to deserialize save data");
        return false;
    }
}

bool SaveManager::saveGame(const SaveData& data)
{
    std::string path = getSaveFilePath();
    CCLOG("Saving game to: %s", path.c_str());

    // 序列化为 JSON
    rapidjson::Document doc = serializeToJson(data);

    // 转换为字符串
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string jsonStr = buffer.GetString();

    // 写入文件
    FILE* file = fopen(path.c_str(), "w");
    if (file)
    {
        fwrite(jsonStr.c_str(), 1, jsonStr.length(), file);
        fclose(file);
        CCLOG("Game saved successfully!");
        return true;
    }
    else
    {
        CCLOG("Error: Failed to open file for writing: %s", path.c_str());
        return false;
    }
}

bool SaveManager::loadGame(SaveData& data)
{
    std::string path = getSaveFilePath();
    CCLOG("Loading game from: %s", path.c_str());

    if (!FileUtils::getInstance()->isFileExist(path))
    {
        CCLOG("Error: Save file does not exist");
        return false;
    }

    // 读取文件内容
    std::string jsonStr = FileUtils::getInstance()->getStringFromFile(path);
    if (jsonStr.empty())
    {
        CCLOG("Error: Failed to read save file");
        return false;
    }

    // 解析 JSON
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());

    if (doc.HasParseError())
    {
        CCLOG("Error: Failed to parse JSON: %d", doc.GetParseError());
        return false;
    }

    // 反序列化
    if (!deserializeFromJson(doc, data))
    {
        CCLOG("Error: Failed to deserialize save data");
        return false;
    }

    CCLOG("Game loaded successfully!");
    return true;
}
