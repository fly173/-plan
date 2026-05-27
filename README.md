# UCAV 总体参数初步设计工具

这是一个 C++/Qt Widgets 桌面程序原型，用于无人作战飞机总体参数初步设计。当前版本已经包含：

- 设计指标输入
- 8 段默认任务剖面编辑
- 气动参数和发动机参数输入
- 最大起飞重量迭代
- 巡航、转弯、爬升、起飞距离、着陆距离约束分析
- 界限线图绘制和鼠标选取设计点
- JSON 项目保存与导入

## 文件结构

- `src/core`：不依赖 Qt 的核心计算模块
- `src/ui`：Qt Widgets 界面和界限线图控件
- `tests`：核心算法命令行测试
- `docs/windows_build.md`：Windows 构建和打包说明

## 当前说明

当前仓库没有附带 QCustomPlot 源码，因此界限线图先用 `QPainter` 自绘控件实现，交互和数据接口已经隔离在 `ConstraintPlotWidget` 中。拿到 QCustomPlot 后可替换该控件内部绘图实现。
