# 每次手势选择起点，长按保持稳定

本地扩展支持普通按键的区域落点，以及浮动摇杆的整体平移。原配置默认不启用。
这不是防封功能，没有验证反作弊效果。尚未在手机或游戏内测试手感。

## 浮动摇杆

在 `KMT_STEER_WHEEL` 节点添加：

```json
"floatingOriginRange": {"x": 0.01, "y": 0.01}
```

坐标按屏幕宽高归一化。x/y 是围绕 `centerPos` 的水平/垂直半范围：
例如宽 1000 像素时，x=0.01 表示左右各 10 像素。这只是配置示例，不是已验证的推荐值。

- 第一个方向键按下时，在范围内选择一次中心 A，发送落指，然后移到 A 加方向偏移。
- W 长按维持位置，不继续往上累加，不运行周期漂移。
- W+D、松开 W 继续按 D 等组合全过程使用同一个中心、同一个触点编号。
- 所有方向键松开时立即释放；下一次手势才重新选点。
- 上下同时按下时使用原有方向偏移求和规则；对称配置会回到本次中心。
- 新模式不使用上游摇杆拖动的随机轨迹/定时队列，直接发送目标位置。
- 设为 `{ "x": 0, "y": 0 }` 可使用稳定模式但固定起点，便于对照手感。
- 删除该字段恢复原有摇杆模式。重新应用脚本后生效。

必须使用游戏支持的浮动摇杆，且整个起点范围都落在允许激活区域内。
固定摇杆不能保证方向保持一致。程序只验证屏幕边界，不知道游戏按钮或摇杆的有效区域。
中心范围加上四方向完整行程必须在屏幕内；无效范围会拒绝该映射节点并输出警告。

## 普通键盘／鼠标按钮

在 `KMT_CLICK` 节点添加：

```json
"touchRange": {"x": 0.01, "y": 0.01}
```

每次按下时在 `pos` 附近选点，长按与松开共用同一点；下次按下重新选择。
键盘自动重复不会新建触点。范围必须完全位于实际按钮内部，避开相邻按钮。
目前仅支持 `switchMap: false` 的普通点击，不适用于双击、多点宏、拖拽、视角或切换鼠标模式按钮。
删除字段恢复原点击逻辑。

视角使用独立参数 `mouseMoveMap.startPosRange`，小眼睛使用 `mouseMoveMap.smallEyes.touchRange`。
均为归一化坐标的矩形半范围，每段触控仅取一次起点，拖动位移和灵敏度保持原配置。
缺省范围为零；视角起点范围必须位于 x/y 0.05–0.95 的拖动边界内。

关闭映射模式、替换脚本时，会释放新模式正在按住的摇杆和按钮。
窗口失焦、设备断线、手机系统兼容性尚需实机验证，不视为已通过。

## 示例与使用

三角洲配置已按用户提供的键位另行加入，见 [三角洲配置说明](deltaforce_zh.md)。
`keymap/deltaforce.json` 启用小幅范围落点，`keymap/deltaforce-original.json` 保留原文件用于回退。

`keymap/floating-origin-demo.json` 是独立触控测试示例，包含 WASD、F、鼠标左键。
其中坐标不是三角洲布局，没有提供未经确认的游戏键位。
选中脚本并应用，按反引号键切换映射模式。在触控测试界面核对坐标后再调整自己的布局。
原版发布程序不会理解新增参数，必须使用本次编译的修改版。

## 构建与回归

在 QtScrcpy 仓库根目录执行（本机 macOS / Apple Silicon / Qt 5）：

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5 -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 6
cmake -S QtScrcpy/QtScrcpyCore/tests -B build-touch-tests -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5 -DCMAKE_BUILD_TYPE=Debug
cmake --build build-touch-tests --parallel 6
ctest --test-dir build-touch-tests --output-on-failure
```

输出：`output/arm64/Debug/QtScrcpy.app`。这是本机构建产物，仍依赖本机 Qt，未打包为可分发安装包。

测试读取真实 Controller 产生的序列化触控消息，覆盖长按无后续事件、128 次起点范围检查、
W/D 组合及反向按键、快速按下松开、键盘与鼠标按钮保持同一点、模式切换/脚本替换释放、
无效配置拒绝及原配置默认关闭。测试不连接手机、不进入游戏。

## 仓库结构

输入实现位于上游的 `QtScrcpy/QtScrcpyCore` Git 子模块；文档和示例在主仓库。
本地两个仓库均使用 `minghao/stable-touch-origin` 分支，未提交、未推送。
远程保存时必须同时处理核心子模块的改动，单独推送主仓库无法包含未提交的核心修改。
