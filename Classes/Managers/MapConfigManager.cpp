/****************************************************************
 * Project Name:  Clash_of_Clans
 * File Name:     MapConfigManager.cpp
 * File Function: 地图配置管理器实现
 * Author:        赵崇治
 * Update Date:   2025/12/14
 * License:       MIT License
 ****************************************************************/
#include "MapConfigManager.h"
#include "ui/CocosGUI.h"
#include "json/writer.h"
#include "json/stringbuffer.h"
#include <exception>
#include <stdexcept>

USING_NS_CC;
using namespace ui;

MapConfigManager* MapConfigManager::create(const std::string& filename)
{
    MapConfigManager* ret = new (std::nothrow) MapConfigManager();
    if (ret && ret->init(filename)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool MapConfigManager::init(const std::string& filename)
{
    try {
        if (!Node::init()) return false;
        _filename = filename;

        // try load existing file (if present)
        loadFromFile();
        return true;
    }
    catch (const std::exception& e) {
        CCLOG("❌ MapConfigManager::init 异常: %s", e.what());
        return false;
    }
}

bool MapConfigManager::loadFromFile()
{
    try {
        // read file using FileUtils
        std::string fullPath = FileUtils::getInstance()->fullPathForFilename(_filename);
        if (!FileUtils::getInstance()->isFileExist(fullPath)) {
            CCLOG("MapConfigManager: config file not found: %s", _filename.c_str());
            return false;
        }

        std::string content = FileUtils::getInstance()->getStringFromFile(fullPath);
        rapidjson::Document doc;
        doc.Parse(content.c_str());
        if (doc.HasParseError() || !doc.IsObject()) {
            CCLOG("MapConfigManager: failed to parse config json");
            return false;
        }

        _configs.clear();
        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
            std::string mapName = it->name.GetString();
            const rapidjson::Value& obj = it->value;
            MapConfig cfg;
            if (obj.HasMember("scale") && obj["scale"].IsNumber()) cfg.scale = obj["scale"].GetFloat();
            if (obj.HasMember("tileSize") && obj["tileSize"].IsNumber()) cfg.tileSize = obj["tileSize"].GetFloat();
            if (obj.HasMember("startPixel") && obj["startPixel"].IsArray() && obj["startPixel"].Size() >= 2) {
                cfg.startPixel.x = obj["startPixel"][0].GetFloat();
                cfg.startPixel.y = obj["startPixel"][1].GetFloat();
            }
            _configs[mapName] = cfg;
        }

        CCLOG("MapConfigManager: loaded %zu configs", _configs.size());
        return true;
    }
    catch (const std::exception& e) {
        CCLOG("❌ MapConfigManager::loadFromFile 异常: %s", e.what());
        return false;
    }
}

bool MapConfigManager::saveToFile()
{
    try {
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

        for (auto& kv : _configs) {
            rapidjson::Value obj(rapidjson::kObjectType);
            obj.AddMember("scale", kv.second.scale, alloc);
            obj.AddMember("tileSize", kv.second.tileSize, alloc);
            rapidjson::Value arr(rapidjson::kArrayType);
            arr.PushBack(kv.second.startPixel.x, alloc);
            arr.PushBack(kv.second.startPixel.y, alloc);
            obj.AddMember("startPixel", arr, alloc);

            rapidjson::Value nameVal;
            nameVal.SetString(kv.first.c_str(), (rapidjson::SizeType)kv.first.length(), alloc);
            doc.AddMember(nameVal, obj, alloc);
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        std::string out = buffer.GetString();
        bool ok = FileUtils::getInstance()->writeStringToFile(out, _filename);
        CCLOG("MapConfigManager: saved %zu configs to %s -> %d", _configs.size(), _filename.c_str(), ok);
        return ok;
    }
    catch (const std::exception& e) {
        CCLOG("❌ MapConfigManager::saveToFile 异常: %s", e.what());
        return false;
    }
}

// ... (其余方法保持不变，它们主要是简单的 getter/setter)
bool MapConfigManager::hasConfig(const std::string& mapName) const
{
    return _configs.find(mapName) != _configs.end();
}

MapConfigManager::MapConfig MapConfigManager::getConfig(const std::string& mapName) const
{
    auto it = _configs.find(mapName);
    if (it != _configs.end()) return it->second;
    return MapConfig();
}

void MapConfigManager::setConfig(const std::string& mapName, const MapConfig& cfg)
{
    _configs[mapName] = cfg;
    if (_onConfigChanged) _onConfigChanged(mapName, cfg);
}

void MapConfigManager::setAvailableMaps(const std::vector<std::string>& maps)
{
    _availableMaps = maps;
}

void MapConfigManager::refreshUIForMap(const std::string& mapName)
{
    if (mapName.empty()) return;
    _currentEditingMap = mapName;
    MapConfig cfg = getConfig(mapName);
    if (_lblScale) _lblScale->setString(StringUtils::format("Scale: %.3f", cfg.scale));
    if (_lblStart) _lblStart->setString(StringUtils::format("Start: %.2f, %.2f", cfg.startPixel.x, cfg.startPixel.y));
    if (_lblTile) _lblTile->setString(StringUtils::format("Tile: %.2f", cfg.tileSize));
}

void MapConfigManager::createEditorUI()
{
    try {
        // 位置和尺寸可以根据需要调整
        Vec2 base = Vec2(300, 300);

        // Map list
        _mapListView = ListView::create();
        _mapListView->setContentSize(Size(180, 200));
        _mapListView->setPosition(base + Vec2(-200, 0));
        _mapListView->setBackGroundColor(Color3B(70, 70, 70));
        _mapListView->setBackGroundColorType(ui::Layout::BackGroundColorType::SOLID);
        _mapListView->setScrollBarEnabled(true);

        for (auto& m : _availableMaps) {
            auto item = Layout::create();
            item->setContentSize(Size(170, 36));
            auto lbl = Label::createWithSystemFont(m, "Arial", 14);
            lbl->setPosition(Vec2(85, 18));
            item->addChild(lbl);
            item->setTouchEnabled(true);
            item->addClickEventListener([this, m](Ref* r) {
                refreshUIForMap(m);
            });
            _mapListView->pushBackCustomItem(item);
        }

        this->addChild(_mapListView, 1000);

        // Labels
        _lblScale = Label::createWithSystemFont("Scale: ", "Arial", 16);
        _lblScale->setPosition(base + Vec2(0, 60));
        this->addChild(_lblScale, 1000);

        _lblStart = Label::createWithSystemFont("Start: ", "Arial", 16);
        _lblStart->setPosition(base + Vec2(0, 30));
        this->addChild(_lblStart, 1000);

        _lblTile = Label::createWithSystemFont("Tile: ", "Arial", 16);
        _lblTile->setPosition(base + Vec2(0, 0));
        this->addChild(_lblTile, 1000);

        // Buttons: increase/decrease scale, move start by +/-1, +/-10, change tile
        auto makeBtn = [this](const std::string& t, const Vec2& pos, const std::function<void()>& cb) {
            auto b = Button::create();
            b->setTitleText(t);
            b->setTitleFontSize(14);
            b->setContentSize(Size(60, 30));
            b->setPosition(pos);
            b->addClickEventListener([cb](Ref* r) { cb(); });
            this->addChild(b, 1000);
            return b;
        };

        // scale +/-
        makeBtn("Scale+", base + Vec2(80, 60), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.scale += 0.01f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });
        makeBtn("Scale-", base + Vec2(160, 60), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.scale -= 0.01f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });

        // start +/-1
        makeBtn("X+1", base + Vec2(80, 30), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.startPixel.x += 1.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });
        makeBtn("X-1", base + Vec2(160, 30), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.startPixel.x -= 1.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });

        // start +/-10
        makeBtn("X+10", base + Vec2(80, 0), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.startPixel.x += 10.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });
        makeBtn("X-10", base + Vec2(160, 0), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.startPixel.x -= 10.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });

        // move Y +/-1 and +/-10
        makeBtn("Y+1", base + Vec2(80, -30), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.startPixel.y += 1.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });
        makeBtn("Y-1", base + Vec2(160, -30), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.startPixel.y -= 1.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });
        makeBtn("Y+10", base + Vec2(80, -60), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.startPixel.y += 10.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });
        makeBtn("Y-10", base + Vec2(160, -60), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.startPixel.y -= 10.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });

        // tile +/-
        makeBtn("Tile+", base + Vec2(80, -90), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.tileSize += 1.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });
        makeBtn("Tile-", base + Vec2(160, -90), [this]() {
            if (_currentEditingMap.empty()) return;
            MapConfig c = getConfig(_currentEditingMap);
            c.tileSize -= 1.0f;
            setConfig(_currentEditingMap, c);
            refreshUIForMap(_currentEditingMap);
        });

        // Save
        makeBtn("Save", base + Vec2(80, -130), [this]() {
            saveToFile();
        });

        // Apply to runtime grid (call callback)
        makeBtn("Apply", base + Vec2(160, -130), [this]() {
            if (_currentEditingMap.empty()) return;
            if (_onConfigChanged) _onConfigChanged(_currentEditingMap, getConfig(_currentEditingMap));
        });
    }
    catch (const std::exception& e) {
        CCLOG("❌ MapConfigManager::createEditorUI 异常: %s", e.what());
    }
}
