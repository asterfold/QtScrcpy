# 外接显示器投屏

在「启动配置」顶部的「投屏目标」选择手机屏幕或外接显示器，然后点击「启动服务」。连接后列表会自动读取，也可点击「刷新显示器」。选项会保存到 `CommonDisplayUniqueId`。

外接显示器使用 Android 的 `uniqueId` 识别，启动前重新查询当前逻辑 display ID。显示器断开时保留选择并提示重新连接，不自动改投手机屏幕。虚拟显示、新显示及手动 ID 配置仍可通过「使用高级显示设置」使用。查询失败时不会启动保存的外接目标。

## Samsung Android 16 黑屏兼容

本机 Samsung SM_S948B 的外接屏通过 scrcpy 4.1 DisplayManager 捕获时仅返回会话信息，没有视频帧。服务端增加 Samsung Android 16 外接现有显示屏的原生 `screenrecord` 分支，将 H.264 Annex B 转成现有 scrcpy 视频包，并将触控映射到原外接屏逻辑 ID。

补丁位于 `QtScrcpy/QtScrcpyCore/server-patches/samsung-external-native-capture.patch`，服务端产物位于该子模块的 `src/third_party/scrcpy-server`。选择手机屏幕等其他场景仍用原捕获流程。原生分支适用于 H.264、未裁剪、方向为 0 的现有外接屏；系统编码器决定帧率，不支持 scrcpy 的 max-fps 或自定义编码器调优。手机真实锁屏或系统关闭外接内容的行为未验收，不保证继续捕获。

## 验证（2026-10-04）

- `cmake --build build -j6`：成功。
- `git diff --check`：通过。
- `displaylist_tests`：显示列表、ID 67→71 保持同一 uniqueId、目标缺失、错误输出及真实手机列表均通过。
- 原始 scrcpy v4.1 源码上的 `patch --dry-run -p1`：通过。
- 已安装 `/Applications/QtScrcpy.app`，签名验证通过。
- 实机界面切换手机/Monitor 选项、退出再打开恢复 Monitor、点击「启动服务」显示外接屏游戏画面：通过。未为本轮验收强制拔插显示器；ID 改变使用解析测试覆盖。

测试程序可在仓库根目录独立运行：

```sh
clang++ -std=c++11 -fPIC \
  -I/opt/homebrew/opt/qt@5/lib/QtCore.framework/Headers \
  -F/opt/homebrew/opt/qt@5/lib -framework QtCore \
  QtScrcpy/tests/displaylist_tests.cpp -o /tmp/displaylist_tests
/tmp/displaylist_tests
```

旧应用主程序和服务端保存在 `~/Library/Application Support/QtScrcpy/backups/external-capture-20261004/`。本次未提交或推送仓库。

## 鼠标捕获修复（2026-10-05）

对比原程序与 10 月 4 日安装程序的反汇编，确认前者在 `switchGameMap` / `updateSize` 调用 `grabCursor`，后者因 Debug 条件编译省略调用。现已移除这两处条件，并在游戏控制模式统一隐藏鼠标；普通模式恢复鼠标。未更改 `三角洲脚本new.json`，项目与运行时脚本逐字节一致。

`cmake --build build -j6`、主仓库和子模块的 `git diff --check`、应用签名验证通过。新增 `QtScrcpy/QtScrcpyCore/tests/cursor_capture_tests.cpp`，以当前 Debug 核心和三角洲脚本运行，验证捕获开启、范围更新、隐藏鼠标、退出释放及恢复，结果 PASS。测试使用 `QT_QPA_PLATFORM=offscreen`，不能代替真实 ATK 鼠标持续移动验收。

已安装并重启 `/Applications/QtScrcpy.app`，保留外接 Monitor，选择并应用三角洲脚本。真实鼠标快速移动后是否仍停转，尚待用户实测。安装前程序备份位于 `~/Library/Application Support/QtScrcpy/backups/mouse-capture-20261005/QtScrcpy.before`。

## 连续转向与地图操作优化（2026-10-05）

连续转向保留光标回绕当次的真实移动量，用回绕位置作为下一次移动的起点；触摸到边界时立即抬起并重建，不再丢弃 5 次移动。Mac 捕获被系统因超时或用户输入停用后，在仍处于捕获模式时重新启用。切换/重载脚本时释放视角触摸和鼠标，避免残留按下状态。光标隐藏/恢复采用幂等状态，避免反复压入覆盖光标栈。

「三角洲脚本new.json」此次有针对性修改：`M` 地图、`Tab` 背包启用 `switchMap`，并删除与该选项互斥的 `touchRange`。打开前先抬起视角触摸，打开后显示鼠标；悬停不发触摸，左键点击或拖动可操作界面。再按同一个键关闭并恢复视角。若通过游戏内关闭按钮退出，请用 `~` 切回普通模式、再按 `~` 恢复视角；程序未读取游戏菜单状态，无法自动识别所有关闭路径。

灵敏度配置在运行时文件 `~/Library/Application Support/QtScrcpy/QtScrcpy/keymap/三角洲脚本new.json` 的 `mouseMoveMap` 中。`speedRatioX` 从 3.25 调为 6.5，`speedRatioY` 从 1.25 调为 2.5，有效转向速度为原来一半。数值越大越慢，X/Y 分别控制左右/上下；存在 X/Y 时旧 `speedRatio` 值不生效。编辑后点击「应用脚本」，再用 `~` 进入视角模式。项目和运行时副本保持一致；修改前备份保存在 `~/Library/Application Support/QtScrcpy/backups/mouse-camera-20261005/`。

验证：`cmake --build build -j6`、`python3 QtScrcpy/QtScrcpyCore/tests/run_camera_tests.py`、主仓库及子模块 `git diff --check` 通过。两组测试覆盖捕获/释放、600 次连续触摸边界更新、静止鼠标、地图/背包悬停及点击、恢复视角、重载清理和灵敏度解析。模拟测试不覆盖真实 ATK 鼠标、系统捕获恢复通知及游戏内所有菜单路径；长期左右转向仍需实机验收。

## 间歇停转诊断（2026-10-05）

用户仍报告间歇停转，当前尚未确认根因。新增每 2 秒一次的本地计数：Mac 捕获是否请求/可用/启用及移动次数、窗口收到/转发的移动次数、控制模式和视角触摸移动次数；控制连接短写也记录。日志位于 `~/Library/Application Support/QtScrcpy/mouse-diagnostics.log`，超过 1 MiB 时重新开始，仅记录计数与状态，不记录键盘内容或截图。需要实际卡住时的记录，才可区分捕获中断、窗口过滤、菜单模式残留与触摸发送问题；该版本不代表已修复实际停转。

实机诊断发现鼠标捕获 `available=0`，系统「设备控制和数据访问」中的 QtScrcpy 开关关闭。经用户授权并由用户完成 Touch ID 后，开关显示开启，但第一次重启仍为 `available=0`。通过系统设置移除旧条目、重新添加 `/Applications/QtScrcpy.app`，再次重启后确认 `available=1`。已恢复 Monitor 投屏并选中三角洲 new 脚本；普通模式下 `enabled=0` 属预期状态，进入自定义视角模式后的启用和长期实际鼠标转向仍待验证。未扩大到其他应用权限。

## 突然转向修复（2026-10-05）

用户报告普通拖动以及抬起、放下鼠标后突然大幅转向。原路径按窗口坐标差计算视角，同时回绕光标；排队中的回绕前事件可能被与回绕后的原点比较，生成错误的大幅或反向位移。Mac 捕获可用时改从已有事件捕获读取 `kCGMouseEventDeltaX/Y`，排队传递实际增量，自定义视角期间忽略普通绝对位置移动事件；普通菜单鼠标保留原行为。累计增量标记为应用产生的事件，核心不对它执行光标回绕。切换模式、调整窗口或释放捕获时使旧队列失效，回调使用弱引用保护窗口生命周期。未更改三角洲 new 的按键与灵敏度。

`cmake --build build -j6` 和 `python3 QtScrcpy/QtScrcpyCore/tests/run_camera_tests.py` 通过。新增 300 次连续向右相对移动的方向断言，累计位置越过窗口范围仍不反转，重建触摸后也保持方向；零增量不发送触摸。保留原 600 次边界移动与地图/背包回归。测试不代替真实鼠标抬起放下验收，传感器若上报异常增量仍需要实际记录进一步定位。

用户随后报告鼠标接近边缘后转身停止。上面的初版相对移动路径只保留 Mac 捕获层的边缘限制，没有配套回中；模拟方向测试不能覆盖系统光标到边缘后的行为，初版不应视为实机通过。后续在相对移动捕获层增加提前回中：距四个边缘不足 50 点（小窗口限制为尺寸的四分之一）时将系统光标移回捕获区域中心，仍只向视角传实际增量，避免用回中后的绝对位置计算转向。诊断新增 `recenters` 次数。独立 `QtScrcpy/tests/relative_cursor_tests.cpp` 覆盖四边、越界、小窗口和连续 10000 步回中，通过；相对方向与菜单回归也通过，实际边缘转向仍需用户复测。

**后续实机反馈不通过，以上两轮方案已撤回。** 用户报告移动断续且视角无法正常转动。源码已移除相对移动处理、普通鼠标事件屏蔽和新增提前回中路径；应用恢复到 `backups/relative-mouse-20261005/QtScrcpy.before`，保留此前捕获修复、地图/背包处理、灵敏度和低频诊断。实验辅助文件保留作调查记录，但不接入运行程序，不能将实验测试通过称为实际修复。原偶发停转/跳转仍未完成实机验收。

## 回绕旧事件及触摸释放修复（2026-10-05）

保持原窗口坐标控制路径。新增 `CursorMotion` 区分已排队的回绕前坐标与回绕后坐标：旧队列按旧起点计算小步长，确认新位置后改用回绕目标，避免将窗口宽度当作移动量。切换菜单、模式和窗口尺寸时重置跟踪。Mac 捕获层仅将边缘移动事件路由回游戏，移动时不再执行另一套光标 warp；核心负责回绕，避免原生捕获层覆盖核心刚设置的位置。新增每 2 秒汇总 `warps`、`queued` 和 `max_delta`，不记录鼠标绝对位置。

触摸更新先计算候选位置，越界时用此前有效坐标抬起，再在起点重建。原实现先更新到越界点，再发送抬起，现已避免越界释放。没有引入相对增量捕获、提前回中、鼠标平滑滤波或灵敏度变化。

独立验证将旧的坐标差算法编译进临时核心库，新增四边旧事件检查实际报错 `queued pre-warp event produced a jump or reversal`；新实现通过。回归覆盖四边回绕前排队事件、600 次触摸边界更新及所有触摸坐标有效、静止鼠标、菜单和重载。`cmake --build build -j6`、`python3 QtScrcpy/QtScrcpyCore/tests/run_camera_tests.py`、主仓库及子模块 `git diff --check` 通过。模拟检查仍不能证明真实鼠标长期不再停转或跳转；手感和实际设备结果需单独确认。

新版已替换 `/Applications/QtScrcpy.app` 的主程序并完成签名验证；原程序保存在 `~/Library/Application Support/QtScrcpy/backups/queued-warp-20261005/QtScrcpy.before`。通过系统设置恢复同一应用的既有权限关联，运行诊断确认 `available=1`。已选中 `三角洲脚本new.json`。本轮重启时 ADB 在线，但设备显示器枚举只有内置屏幕，界面显示“已选显示器未连接”；外接屏投屏与真实鼠标验收待重新连接后继续。

## 日志积累造成的界面负担修复（2026-10-09）

原 `Dialog::outLog` 为每条消息排队一个 GUI 更新，日志文档没有上限；本轮实际旧界面仍保存了多次启动、ADB 刷新及历史故障日志。改为线程安全的有界队列：保留最近 256 条待处理消息，单条最多 2048 字符，每 100ms 最多刷新 32 条；文档最多保留 1000 个文本段落。过量旧日志会丢弃，长日志会截断；界面使用纯文本避免对日志内容作 HTML 解析。没有改变键鼠映射、编码参数或帧率。

`cmake --build build -j6`、日志压力检查及 `git diff --check` 通过。检查使用工作线程产生 100000 条消息，确认队列有界、保留最新消息、分批大小、单条截断及文档上限。测试源和可重跑入口：`QtScrcpy/tests/logbuffer_tests.cpp`、`python3 QtScrcpy/tests/run_logbuffer_tests.py`。

新版主程序已替换 `/Applications/QtScrcpy.app` 并签名验证成功，旧程序保存在 `~/Library/Application Support/QtScrcpy/backups/log-stutter-20261009/QtScrcpy.before`。应用启动成功，日志能够显示。新版签名导致原鼠标权限关联需要刷新；系统已弹出 Touch ID，由用户完成认证。本轮 ADB 没有在线设备，未复现游戏卡顿，因此仅确认日志性能缺陷已修正，不能断言全部键鼠失效已解决。

用户完成 Touch ID 后，重新关联同一应用权限并重启。运行诊断确认鼠标捕获权限 `available=1`；ADB 已在线，外接 Monitor 当前逻辑 ID 为 15，外接屏投屏显示三角洲游戏大厅，已加载“三角洲脚本new”。手机报告该 Monitor 当前刷新率约 60 Hz；这不是 QtScrcpy 输出帧率的实测结果。当前仍处于普通鼠标模式（`custom=0`），长期游戏中的停顿及键鼠失效尚未完成实机复测。

## 下滚轮切换倍率（2026-10-16）

用户要求将“三角洲脚本new”的鼠标下滚轮绑定到跳跃右边的倍率按钮。通过当前投屏的布局画面定位（画面显示 3.2X / 6.8X），加入 `WheelDown` 点击节点，归一化位置 `(0.2915, 0.2952)`，不切换鼠标捕获。核心新增下滚事件处理：累计 120 角度单位触发一次 20ms 短按；短按期间快速滚动合并，不无限排队；上滚、水平及零增量不触发。普通鼠标模式保留原滚动行为。切换模式与重载清除残余滚量、释放触摸，并使旧释放回调失效。

本机 Command Line Tools 的新 SDK 与现有 Xcode 链接器不匹配。构建改用已安装的 Xcode MacOSX15.5.sdk（仅本地 CMake 配置，未修改项目 CMake 文件）。`cmake --build build -j6`、`python3 QtScrcpy/QtScrcpyCore/tests/run_wheel_tests.py` 及两层 `git diff --check` 通过。滚轮测试检查方向、分段滚量、目标位置、快速滚动合并、静止无重复、捕获模式及模式退出/脚本重载释放。没有运行仍要求 Tab/M 释放鼠标的旧相机测试，该要求已被用户后续指令取消。

已部署新版主程序到 `/Applications/QtScrcpy.app`，签名验证通过；运行目录的“三角洲脚本new.json”已同步，更新前确认原有设置与源码一致，未覆盖额外运行配置。旧主程序和脚本备份于 `~/Library/Application Support/QtScrcpy/backups/wheel-magnification-20261016-231639/`。重启后已在应用选中“三角洲脚本new.json”。手机此前在线且投屏可见，重启后变为 offline；重连 `192.168.31.12:44911` 报 `No route to host`，mDNS 仍公布同一地址。游戏内实际滚轮切换倍率尚未验收，不能以自动测试替代实机结果。
