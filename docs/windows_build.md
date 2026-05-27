# Windows 构建与打包说明

## 开发环境

1. 安装 Qt 5.15.x 或 Qt 6.x，选择 MinGW 或 MSVC 套件。
2. 用 Qt Creator 打开 `UCAVDesigner.pro`。
3. 选择 Release 构建配置并编译运行。

## 命令行构建

```bat
qmake UCAVDesigner.pro
mingw32-make release
```

如果使用 MSVC，把 `mingw32-make` 替换为 Qt Kit 对应的 `nmake` 或在 Qt Creator 中构建。

## 打包 `.exe`

进入 Release 输出目录，执行：

```bat
windeployqt UCAVDesigner.exe
```

完成后将输出目录整体复制到无 Qt 环境的 Windows 机器上测试。若后续接入 QCustomPlot，只需把 `qcustomplot.h/.cpp` 加入工程，并用 QCustomPlot 替换当前的 `ConstraintPlotWidget` 绘图实现。
