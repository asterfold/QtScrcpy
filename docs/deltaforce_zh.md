# 三角洲行动键位配置

来源：用户提供的 `黄叔的三角洲默认键位.json`。

- `keymap/deltaforce-original.json`：源文件逐字节副本，供回退／对照。
- `keymap/deltaforce.json`：在原配置上启用一次手势内保持稳定的随机落点。

保留全部 20 个映射节点：1 个 WASD 摇杆、17 个键盘按钮、2 个鼠标按钮。
保留鼠标视角配置和 Alt 小眼睛配置，包括所有原坐标、方向行程和灵敏度。
唯一的原键名修正：`Key_Esc` → `Key_Escape`。真实解析测试发现原名不被 Qt 识别，
会跳过该节点；新版使用正确枚举名。原文件副本不改动，因此仍保留这个问题。
多数原按钮注释是“新键位”，没有依据将它们重新标成具体游戏功能。
这份第三方配置不代表游戏所有菜单、载具、角色技能或模式都已覆盖。

## 新增参数

WASD 节点增加 `floatingOriginRange: {"x": 0.002, "y": 0.002}`。
19 个普通点击节点分别增加 `touchRange: {"x": 0.002, "y": 0.002}`。
范围是原坐标左右／上下各屏幕宽／高的 0.2%，不是整屏随机。
以 2400×1080 为例，对应水平 ±4.8 像素、垂直 ±2.16 像素（发送时取整）。
这是小幅初始配置，未经手机按钮命中区域校准，不承诺完全无手感影响。

方向键第一次按下时取中心，W+D 等组合仍使用同一中心，全部松开后才结束手势。
普通按键或鼠标按钮每次按下选点，按住和松开使用同一点。
鼠标转视角增加 `mouseMoveMap.startPosRange`，Alt 小眼睛增加 `smallEyes.touchRange`，均为 x/y 0.002。
每段触控开始时只取一次随机起点，后续按原鼠标相对位移和灵敏度拖动，不持续抖动。
范围仅在原起点附近；没有整个屏幕的按钮区域数据，因此不会自动避开其他按钮。
原脚本上下行程不同（上 0.27、下 0.20），这里原样保留；相反方向同时按下不一定归零。

## 使用与回退

1. 使用本项目编译的修改版 QtScrcpy；下载文件夹中的 Windows exe 不含我们新增的功能。
2. 刷新脚本，选择 `deltaforce.json` 并应用；按反引号键切换普通／映射模式。
3. 摇杆必须是以落指点为中心的浮动模式。起点范围必须位于摇杆激活区内。
4. 先核对手机上的按键布局是否与这份配置一致，尤其是屏幕比例、按钮大小和边缘按钮。
5. 回退时应用 `deltaforce-original.json`，恢复原脚本行为（含原有 Esc 键名问题）。若只想关随机而保留稳定摇杆模式，
   将新配置所有 `floatingOriginRange`／`touchRange`／`startPosRange` 的 x、y 设为 0，再应用脚本。

macOS 本机构建产物为 `output/arm64/Debug/QtScrcpy.app`。
本次已将两份三角洲配置复制到其 `Contents/MacOS/keymap` 默认脚本目录。
完整打包脚本也会复制仓库 `keymap` 目录。原始 Downloads 文件不作修改。

## 验证边界

已检查原参数保留、所有节点被真实解析器接受，以及真实输入消息中的坐标范围、
方向相对位移、长按不重新取点、W+D 切换和释放、19 个按钮的按下／松开坐标一致。
补充验证视角与 Alt 起点范围、拖动相对位移、切换映射关闭后不产生延迟视角按下。
尚未连接手机验证实际布局、操作手感、窗口失焦或设备断线；没有验证防封效果。
可视化 HTML 编辑器未修改，不保证它导出时保留新增字段。

在仓库根目录运行：

```sh
cmake -S QtScrcpy/QtScrcpyCore/tests -B build-touch-tests -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5 -DCMAKE_BUILD_TYPE=Debug -DTOUCH_PROFILE="$PWD/keymap/deltaforce.json"
cmake --build build-touch-tests --parallel 6
ctest --test-dir build-touch-tests --output-on-failure
```
