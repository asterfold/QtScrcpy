# 三角洲行动：Samsung S26 键位

正式配置只有 `keymap/deltaforce.json`，整合最新截图校准。旧配置保留在 Git 历史中。
配置共 23 个节点：1 个 WASD 摇杆和 22 个点击按钮。必须配合主仓库锁定的修改版 Core。

## 主要按键

| 按键 | 映射 |
| --- | --- |
| W/A/S/D | 移动；W 上拉行程 0.08 |
| W + Shift | 上拉行程扩展为 0.27；Shift 单独不点击疾跑按钮 |
| 1 / 2 / 3 | 武器栏；3 对应匕首图标下方的最右侧武器格 |
| Q / E | 左 / 右探头 |
| F | 枪械框下方问号/钥匙交互长条；拾取、开门、搜索需分别实测 |
| 鼠标下侧键（暂按后退键 BackButton） | 枪械框左侧纸张图标；搜索功能需实测，若实际下侧键上报为前进键则改为 ForwardButton |
| Tab | 背包 |
| H | 左侧双人图标（背人） |
| CapsLock | 静步图标 |
| Space / C / Z | 跳跃 / 蹲下 / 趴下 |
| R | 换弹 |
| 鼠标左 / 右键 | 开火 / 开镜 |
| V / G | 战术道具 1 / 2（技能，具体效果以角色为准） |
| X | 特殊装备 |
| Escape | 设置 |
| Alt | 自由观察 |

1、2、M、5 沿用原配置坐标。按反引号键切换普通鼠标与游戏映射模式。
普通模式下菜单鼠标点击不使用这些按键的随机范围。

## 随机范围和移动

坐标与范围按屏幕宽高归一化，范围为中心两侧的半范围。

- 摇杆中心为 (0.16, 0.75)，起点水平/垂直各随机 ±0.04。
- F 中心为 (0.697947, 0.347503)，仅水平随机 ±0.04，上下固定。
- Tab、H、CapsLock、Escape 固定点击；3 和鼠标下侧键水平/垂直各 ±0.002，其他一般按钮同样是小范围随机。
- 视角和 Alt 起点水平/垂直各 ±0.002，相对鼠标位移和灵敏度沿用原配置。

每段触控只随机一次，长按不持续漂移。W 与 Shift 的按下顺序均可；松开 Shift 恢复普通行程，
所有方向键松开即释放触点。若只松开 W 但仍按其他方向，则继续对应移动。
手机内的自动疾跑/冲刺锁定仍可能导致角色继续跑，需要在手机游戏设置中核对。
摇杆须使用浮动模式；随机范围不会自动识别按钮边界，改变布局后需重新校准。

## 使用

刷新脚本，选择 `deltaforce.json`，应用后在投屏窗口按反引号开启映射。
先在训练场验证命中区域和长按释放。将范围设为零可关闭随机落点，同时保留稳定触控逻辑。

## 验证状态

- Windows：MSVC 2022 + Qt 5.15.2，RelWithDebInfo 编译成功；已打包 Qt、ADB、FFmpeg 等依赖。
- 两项自动化测试通过，实际配置测试覆盖 23 个节点、Shift 行程切换、触点保持、释放与长按不漂移。
- 三星手机 USB 连接及投屏已运行；按键配置依据用户截图调整，不能等同于所有游戏操作实测通过。
- F 的拾取、开门、搜索及所有布局边界仍需手机校准。没有验证防封效果，不承诺防封。
- 可视化配置编辑器未适配新增字段，保存时需核对是否保留。

Windows PowerShell，在仓库根目录（将 Qt 路径替换为本机安装路径）：

```powershell
cmake -S QtScrcpy/QtScrcpyCore/tests -B build-touch-tests -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64 "-DTOUCH_PROFILE=$((Resolve-Path keymap/deltaforce.json).Path)" -DTOUCH_PROFILE_EXPECTED_NODES=23
cmake --build build-touch-tests --config RelWithDebInfo --parallel
ctest --test-dir build-touch-tests -C RelWithDebInfo --output-on-failure
```

运行前将 Qt 的 bin 目录加入 PATH。多配置生成器必须用 `-C` 指定实际构建配置。
