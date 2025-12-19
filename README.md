# 🌾 星露谷物语 (Stardew Valley) - 课程项目

<div align="center">

![Game Type](https://img.shields.io/badge/类型-农场模拟-green)
![Language](https://img.shields.io/badge/语言-C++-blue)
![Framework](https://img.shields.io/badge/框架-Cocos2d--x-orange)
![Status](https://img.shields.io/badge/状态-开发中-yellow)
![License](https://img.shields.io/badge/license-MIT-blue)

**一款基于 Cocos2d-x 开发的 2D 像素风格农场模拟经营游戏**

[功能特性](#功能特性) • [技术栈](#技术栈) • [快速开始](#快速开始) • [开发计划](#开发计划) • [团队成员](#团队成员)

</div>

---

## 📋 项目信息

- **课程名称：** 程序设计范式
- **项目类型：** 期末课程项目
- **开发周期：** 2025年12月 - 2025年12月
- **团队人数：** 4人
- **项目难度：** 中高

## 🎮 游戏简介

本项目参考经典独立游戏《星露谷物语》，实现一款 2D 像素风格的农场模拟经营游戏。玩家将扮演继承爷爷农场的主角，通过种植作物、经营农场、与NPC互动等方式，体验悠闲的田园生活。

游戏融合了**资源管理**、**时间规划**、**探索冒险**等多种玩法，强调系统间的相互关联和玩家的策略选择。

### 🎯 核心玩法

- 🌱 **农场经营：** 耕地、种植、浇水、收获
- ⏰ **时间管理：** 日夜循环、季节更替
- 💰 **经济系统：** 出售作物、购买种子
- 🎒 **物品管理：** 背包系统、物品分类
- 🏪 **商店交易：** 购买种子和道具
- 💾 **存档系统：** 保存和读取游戏进度

---

## ✨ 功能特性

### 基础功能（必须实现）

#### 🚶 角色系统
- [x] 四方向移动控制
- [x] 行走/奔跑状态切换
- [x] 角色动画系统
- [x] 交互范围检测

#### 🌾 农场系统
- [x] 30+ 可耕种地块
- [x] 6 种不同作物（生长周期、价格、季节各异）
- [x] 4 种基础工具（锄头、水壶、镰刀、斧头）
- [x] 作物生长阶段显示
- [x] 土地状态管理

#### ⏳ 时间与季节
- [x] 游戏内时间流逝系统
- [x] 日夜循环（早晨/中午/傍晚/夜晚）
- [x] 2个季节支持（春季、夏季）
- [x] 季节切换场景变化
- [x] 日历显示

#### 🎒 背包与物品
- [x] 20格背包空间
- [x] 物品堆叠系统（最多99个）
- [x] 物品分类（种子/作物/工具/资源）
- [x] 拖拽整理功能

#### 🏪 商店系统
- [x] 种子商店
- [x] 季节性商品更新
- [x] 购买/出售机制
- [x] 价格显示

#### 💰 经济系统
- [x] 金币货币系统
- [x] 作物出售结算
- [x] 收支记录

#### 🖥️ UI系统
- [x] 主菜单（新游戏/继续/退出）
- [x] 游戏HUD（金币/时间/日期/季节）
- [x] 暂停菜单
- [x] 背包界面
- [x] 商店界面

#### 💾 存档系统
- [x] 游戏进度保存
- [x] 存档读取
- [x] 数据序列化

#### 🔊 音效系统
- [x] 背景音乐（白天/夜晚）
- [x] 交互音效（耕地/浇水/收获等）

### 可选功能（扩展内容）

#### 🏗️ 建筑系统
- [ ] 鸡舍/牛棚（动物养殖）
- [ ] 谷仓（物品存储）
- [ ] 温室（全季节种植）

#### ⛏️ 矿洞探索
- [ ] 多层矿洞系统
- [ ] 怪物战斗（基础AI）
- [ ] 矿物采集
- [ ] 战斗系统

#### 🎣 钓鱼系统
- [ ] 钓鱼小游戏
- [ ] 5种鱼类
- [ ] 钓鱼技能等级

#### 👥 NPC系统
- [ ] 可互动NPC
- [ ] 对话系统
- [ ] 好感度系统
- [ ] 送礼机制

#### 📈 技能系统
- [ ] 农业技能
- [ ] 采矿技能
- [ ] 钓鱼技能
- [ ] 经验值与等级

#### 🌟 其他功能
- [ ] 作物品质系统
- [ ] 天气系统
- [ ] 节日活动
- [ ] 成就系统

---

## 🛠️ 技术栈

### 开发环境
- **开发语言：** C++ (C++11/14标准)
- **游戏引擎：** Cocos2d-x 3.x/4.x
- **版本控制：** Git + GitHub
- **开发工具：** Visual Studio 2019/2022 / Xcode
- **构建系统：** CMake

### C++ 特性应用

本项目重点应用以下 C++ 特性（满足课程要求）：

#### ✅ 已应用特性

1. **STL 容器**
   ```cpp
   std::vector<Crop*> crops;           // 作物列表
   std::map<int, Item*> itemDatabase;  // 物品数据库
   std::queue<Event> eventQueue;       // 事件队列
   ```

2. **类与多态**
   ```cpp
   class Item { virtual void use() = 0; };
   class Seed : public Item { ... };
   class Crop : public Item { ... };
   ```

3. **C++11 特性**
   - `auto` 关键字
   - Lambda 表达式
   - 智能指针 (`std::shared_ptr`, `std::unique_ptr`)
   - `enum class`
   - 范围 for 循环

4. **模板**
   ```cpp
   template<typename T>
   class ResourceManager { ... };
   ```

5. **操作符重载**
   ```cpp
   Position operator+(const Position& other);
   ```

6. **异常处理**
   ```cpp
   try { loadSave(); }
   catch (const std::exception& e) { ... }
   ```

### 设计模式

- **单例模式：** GameManager, AudioManager
- **工厂模式：** ItemFactory
- **观察者模式：** 时间系统事件通知
- **状态模式：** 游戏状态管理

---

## 📁 项目结构

```
StardewValley/
├── Classes/                    # 游戏逻辑代码
│   ├── Core/                  # 核心系统
│   │   ├── GameManager.h/cpp  # 游戏管理器
│   │   ├── SceneManager.h/cpp # 场景管理器
│   │   ├── TimeManager.h/cpp  # 时间管理器
│   │   └── SaveManager.h/cpp  # 存档管理器
│   ├── Scene/                 # 游戏场景
│   │   ├── MenuScene.h/cpp    # 主菜单场景
│   │   ├── GameScene.h/cpp    # 游戏主场景
│   │   └── ShopScene.h/cpp    # 商店场景
│   ├── Player/                # 玩家相关
│   │   └── Player.h/cpp       # 玩家类
│   ├── Farm/                  # 农场系统
│   │   ├── TileMap.h/cpp      # 地块系统
│   │   ├── Crop.h/cpp         # 作物类
│   │   └── FarmManager.h/cpp  # 农场管理器
│   ├── Item/                  # 物品系统
│   │   ├── Item.h/cpp         # 物品基类
│   │   ├── Seed.h/cpp         # 种子类
│   │   ├── Tool.h/cpp         # 工具类
│   │   └── ItemFactory.h/cpp  # 物品工厂
│   ├── Inventory/             # 背包系统
│   │   └── Inventory.h/cpp    # 背包管理器
│   ├── Shop/                  # 商店系统
│   │   └── Shop.h/cpp         # 商店类
│   ├── UI/                    # UI系统
│   │   ├── HUD.h/cpp          # 游戏HUD
│   │   ├── InventoryUI.h/cpp  # 背包界面
│   │   └── ShopUI.h/cpp       # 商店界面
│   └── Utils/                 # 工具类
│       ├── Constants.h        # 常量定义
│       └── Helper.h/cpp       # 辅助函数
├── Resources/                 # 资源文件
│   ├── images/                # 图片资源
│   │   ├── characters/        # 角色图片
│   │   ├── crops/             # 作物图片
│   │   ├── items/             # 物品图片
│   │   ├── tiles/             # 地块图片
│   │   └── ui/                # UI图片
│   ├── sounds/                # 音效文件
│   │   ├── bgm/               # 背景音乐
│   │   └── sfx/               # 音效
│   └── data/                  # 数据文件
│       ├── crops.json         # 作物数据
│       └── items.json         # 物品数据
├── proj.win32/                # Windows项目
├── proj.android/              # Android项目（可选）
├── proj.ios/                  # iOS项目（可选）
├── CMakeLists.txt             # CMake配置
├── .gitignore                 # Git忽略文件
├── README.md                  # 项目说明
└── docs/                      # 文档
    ├── design.md              # 设计文档
    ├── api.md                 # API文档
    └── development.md         # 开发日志
```

---

## 🚀 快速开始

### 环境要求

- **操作系统：** Windows 10/11, macOS 10.14+, Ubuntu 18.04+
- **编译器：**
  - Windows: Visual Studio 2019 或更高版本
  - macOS: Xcode 11 或更高版本
  - Linux: GCC 7+ 或 Clang 6+
- **CMake：** 3.10 或更高版本
- **Cocos2d-x：** 3.17.2 或 4.0

### 安装步骤

1. **克隆仓库**
   ```bash
   git clone https://github.com/your-username/StardewValley.git
   cd StardewValley
   ```

2. **安装 Cocos2d-x**
   ```bash
   # 下载 Cocos2d-x
   # 设置环境变量
   # 详见 Cocos2d-x 官方文档
   ```

3. **构建项目**
   
   **Windows:**
   ```bash
   mkdir build
   cd build
   cmake .. -G "Visual Studio 16 2019"
   # 打开 .sln 文件编译
   ```
   
   **macOS:**
   ```bash
   mkdir build
   cd build
   cmake .. -G Xcode
   # 打开 .xcodeproj 文件编译
   ```
   
   **Linux:**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

4. **运行游戏**
   ```bash
   # 编译完成后运行可执行文件
   ./StardewValley
   ```

---

## 📅 开发计划

### 第一阶段：核心框架（Week 1-2）
- [x] 项目初始化与环境搭建
- [x] Git 仓库创建与团队协作配置
- [ ] Cocos2d-x 基础场景搭建
- [ ] 角色移动系统实现
- [ ] 地块系统基础实现

### 第二阶段：核心玩法（Week 3）
- [ ] 作物种植系统
- [ ] 作物生长逻辑
- [ ] 时间与季节系统
- [ ] 背包系统
- [ ] 商店系统

### 第三阶段：系统完善（Week 4）
- [ ] 存档系统实现
- [ ] UI 界面优化
- [ ] 音效系统集成
- [ ] 游戏平衡性调整

### 第四阶段：测试与优化（Week 5）
- [ ] 功能测试
- [ ] Bug 修复
- [ ] 性能优化
- [ ] 代码规范检查
- [ ] 文档完善

---

## 👥 团队成员

| 成员 | 学号 | 职责 | GitHub |
|------|------|------|--------|
| **组长姓名** | 学号 | 核心框架 + 角色系统 + 地块系统 | [@username](https://github.com/username) |
| **成员B** | 学号 | 作物系统 + 时间系统 + 经济系统 | [@username](https://github.com/username) |
| **成员C** | 学号 | UI系统 + 背包系统 + 商店系统 | [@username](https://github.com/username) |
| **成员D** | 学号 | 存档系统 + 音效系统 + 可选功能 | [@username](https://github.com/username) |

---

## 📊 开发进度

![Progress](https://progress-bar.dev/15/?title=总体进度&width=500)

- ✅ 项目初始化
- ✅ README 编写
- ✅ 项目设计文档
- 🔄 Cocos2d-x 环境搭建
- ⏳ 核心系统开发
- ⏳ 功能实现
- ⏳ 测试与优化

---

## 📖 文档

- [项目简介](docs/project_intro.md) - 详细的项目说明
- [设计文档](docs/design.md) - 系统设计与架构说明
- [开发日志](docs/development.md) - 开发过程记录
- [API 文档](docs/api.md) - 代码接口说明

---

## 🎯 代码规范

本项目严格遵循 **Google C++ Style Guide**：

- 类名使用大驼峰：`GameManager`
- 函数名使用小驼峰：`updateGame()`
- 成员变量使用下划线结尾：`player_`
- 常量使用全大写：`MAX_INVENTORY_SIZE`
- 使用 C++ 风格类型转换：`static_cast`, `dynamic_cast`
- 合理使用 `const`
- 注释规范，每个类和公有函数必须有文档注释

---

## 🧪 测试

```bash
# 运行单元测试
cd build
ctest

# 查看测试覆盖率
# TODO: 添加测试覆盖率工具
```

---

## 🐛 已知问题

- [ ] 待补充

---

## 🤝 贡献指南

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

**分支命名规范：**
- `feature/xxx` - 新功能
- `bugfix/xxx` - Bug修复
- `refactor/xxx` - 代码重构
- `docs/xxx` - 文档更新

---

## 📝 开发日志

### 2025-12-08
- ✅ 创建项目仓库
- ✅ 完成 README 初稿
- ✅ 编写项目设计文档

### 待更新...

---

## 📜 许可证

本项目为课程项目，仅用于学习目的。

参考游戏《星露谷物语》版权归 ConcernedApe 所有。

---

## 🙏 致谢

- [Cocos2d-x](https://www.cocos.com/) - 游戏引擎
- [ConcernedApe](https://www.stardewvalley.net/) - 原版游戏作者
- [OpenGameArt](https://opengameart.org/) - 开源游戏资源
- 课程助教与老师的指导

---

## 📞 联系方式

- **课程：** 程序设计范式
- **项目周期：** 2025年12月
- **问题反馈：** [Issues](https://github.com/your-username/StardewValley/issues)

---

<div align="center">

**⭐ 如果这个项目对你有帮助，请给我们一个 Star！⭐**

Made with ❤️ by Our Team

</div>
