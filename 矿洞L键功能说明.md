# 矿洞系统 - L 键直接进入功能

## ✅ 已完成的功能

### 1. L 键直接进入矿洞
在主世界（GameScene）中，按 **L** 键可以直接进入矿洞场景，无需靠近电梯。

### 2. 矿洞地图加载
- **地图路径：** `Resources/map/Mines/1.tmx`
- **支持楼层：** 当前支持第 1 层，可扩展到多层

### 3. 四个图层显示
矿洞地图包含四个图层，从下到上的渲染顺序为：

```
1. Back 层     (Z-order: -100)  - 最底层背景
2. Buildings 层 (Z-order: -50)   - 墙壁/碰撞层
3. 玩家        (Z-order: 10)    - 玩家角色
4. mine1 层    (Z-order: 15)    - 矿石贴图层（显示在玩家上方）
5. Front 层    (Z-order: 20)    - 装饰层（显示在最上层）
```

**重要说明：**
- **mine1 层**（矿石贴图）会被提升到场景层级，Z-order 设为 15，显示在玩家上方但在 Front 层下方
- **Front 层**会被提升到场景层级，Z-order 设为 20，确保显示在最上层
- **Back、Buildings 层**保留在 TMX 地图中，设置了负 Z-order 确保在玩家下方
- 这样可以实现正确的遮挡关系：地面 → 墙壁 → 玩家 → 矿石 → 装饰

### 4. 玩家生成位置
- **生成位置：** 地图中心
- **查找策略：** 从中心点开始螺旋搜索，找到第一个可行走的位置
- **搜索范围：** 半径 320 像素，每 16 像素增加半径，每个半径检查 8 个方向
- **兜底方案：** 如果找不到，使用随机可行走位置

**日志输出示例：**
```
Map size: (560.00, 560.00) pixels
Map center: (280.00, 280.00)
✓ Center position is walkable
✓ Player positioned at (280.00, 280.00)
```

或者：
```
Center position not walkable, searching nearby...
✓ Found walkable position at (296.00, 280.00), radius: 16
✓ Player positioned at (296.00, 280.00)
```

## 🎮 控制说明

### 主世界（GameScene）
- **L 键** - 直接进入矿洞（新功能）
- **M 键** - 需要靠近电梯才能进入矿洞（原有功能）
- **B 键** - 打开/关闭背包
- **ESC 键** - 返回主菜单

### 矿洞场景（MineScene）
- **WASD** - 移动
- **J 键** - 攻击/挖矿
- **空格** - 打开宝箱
- **Enter** - 如果在楼梯上，前往下一层
- **M 键** - 返回主世界
- **ESC** - 返回主世界

## 📁 相关文件

### 修改的文件
1. **MineScene.cpp** - 修改了地图加载逻辑
   - 改用 `map/Mines/%d.tmx` 格式
   - 处理四个图层的 Z-order
   - Front 层提升到场景层级

2. **GameScene.cpp** - 添加了 L 键处理
   - 在 `onKeyPressed()` 方法中添加 `KEY_L` 案例
   - 直接调用 `enterMine()` 进入矿洞

### 地图文件
- **Resources/map/Mines/1.tmx** - 第一层矿洞地图
- **Resources/map/Mines/mine.png** - 地砖集图片

## 🔧 技术实现

### Z-order 层级管理
为了确保所有图层正确显示，使用了以下策略：

1. **mine1、Back、Buildings 层保留在 TMX 中**
   - 设置负 Z-order (-100, -50, -25)
   - 确保它们渲染在玩家下方

2. **Front 层提升到场景层级**
   - 从 TMX 中移除
   - 计算绝对位置
   - 添加到场景，Z-order = 20
   - 确保显示在玩家 (Z=10) 上方

### 位置计算
```cpp
// Front 层在场景中的绝对位置 =
// mineLayer 位置 + tmxMap 位置 + frontLayer 位置
Vec2 frontPosInScene = mineLayerPosInScene + mapPosInMineLayer + layerPosInMap;
```

## 🧪 测试步骤

1. **启动游戏**
   ```bash
   build\bin\Sigeluguanshou\Debug\Sigeluguanshou.exe
   ```

2. **进入矿洞**
   - 在主世界按 **L** 键

3. **检查显示**
   - 确认四个图层都能看到：
     - mine1 层（最底层基础）
     - Back 层（背景地面）
     - Buildings 层（墙壁/障碍物）
     - Front 层（装饰，应显示在玩家上方）

4. **测试交互**
   - WASD 移动玩家
   - 检查碰撞检测（应该被 Buildings 层阻挡）
   - 走到装饰物"后面"，检查是否被 Front 层遮挡

## 📊 控制台日志

启动矿洞时，应该看到以下日志：

```
========================================
Initializing Mine Scene (Floor 1)
========================================
Initializing mine map...
Mine layer added to scene (Floor 1)
TMX Map position: (0.00, 0.00), Z-order: 0
✓ mine1 layer found - keeping in tmxMap
✓ Back layer found - keeping in tmxMap
✓ Buildings layer found - keeping in tmxMap
✓ Front layer found!
  - Position: (0.00, 0.00)
  - Visible: YES
  - Layer Size: (35, 35)
✓ Front layer moved to scene with Z-order 20
...
Mine Scene initialized successfully!
```

## 🎯 预期效果

修复完成后，您应该看到：

1. **四个图层全部显示**
   - mine1：最底层基础
   - Back：地面纹理
   - Buildings：墙壁和障碍物
   - Front：装饰效果

2. **正确的遮挡关系**
   - 玩家显示在地面和墙壁上方
   - 装饰物（Front 层）显示在玩家上方
   - 当玩家走到某些装饰后面时，会被部分遮挡

3. **流畅的交互**
   - 玩家可以正常移动
   - 被墙壁（Buildings 层）正确阻挡
   - 碰撞检测正常工作

## 💡 后续扩展

如果需要添加更多楼层：

1. 在 `Resources/map/Mines/` 文件夹中添加 `2.tmx`、`3.tmx` 等文件
2. 确保每个文件都包含四个图层：mine1、Back、Buildings、Front
3. 矿洞系统会自动根据楼层号加载对应的地图

## ⚠️ 注意事项

1. **图层命名必须准确**
   - 确保 TMX 文件中的图层名称为：`mine1`、`Back`、`Buildings`、`Front`
   - 大小写敏感

2. **图块集路径**
   - 确保 TMX 文件中的 tileset 引用正确：`source="mine.png"`
   - 确保 `Resources/map/Mines/mine.png` 文件存在

3. **性能考虑**
   - Front 层被提升到场景层级，会略微增加渲染复杂度
   - 但这是实现正确遮挡关系的必要代价

---

✨ 功能已完成！按 **L** 键即可直接进入矿洞场景。
