#include "MainWindow.h"

#include <QComboBox>
#include <QDoubleValidator>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QAction>
#include <QCloseEvent>
#include <QFileInfo>
#include <QJsonParseError>
#include <QLocale>
#include <QSaveFile>
#include <QScopedValueRollback>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <cmath>

namespace {

constexpr double kMetersPerKilometer = 1000.0;
constexpr double kSecondsPerMinute = 60.0;
constexpr double kSecondsPerHour = 3600.0;

QLineEdit* numericEdit(double value)
{
    auto* edit = new QLineEdit(QString::number(value, 'g', 10));
    auto* validator = new QDoubleValidator(edit);
    validator->setNotation(QDoubleValidator::ScientificNotation);
    validator->setLocale(QLocale::c());
    edit->setValidator(validator);
    edit->setClearButtonEnabled(true);
    return edit;
}

double valueOf(const QLineEdit* edit)
{
    return edit->text().toDouble();
}

double tableNumber(const QTableWidget* table, int row, int column)
{
    const QTableWidgetItem* cell = table->item(row, column);
    return cell ? cell->text().toDouble() : 0.0;
}

std::string tableText(const QTableWidget* table, int row, int column)
{
    const QTableWidgetItem* cell = table->item(row, column);
    return cell ? cell->text().toStdString() : std::string();
}

void setCell(QTableWidget* table, int row, int column, double value)
{
    table->setItem(row, column, new QTableWidgetItem(QString::number(value, 'g', 10)));
}

void setCell(QTableWidget* table, int row, int column, const QString& value)
{
    table->setItem(row, column, new QTableWidgetItem(value));
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("未命名项目[*] - 无人作战飞机初步设计工具");
    resize(1180, 760);

    auto* tabs = new QTabWidget(this);
    m_tabs = tabs;
    tabs->addTab(buildDesignTab(), "设计要求");
    tabs->addTab(buildMissionTab(), "任务剖面");
    tabs->addTab(buildParameterTab(), "气动 / 发动机");
    tabs->addTab(buildResultTab(), "计算结果");
    setCentralWidget(tabs);

    auto* toolbar = addToolBar("项目操作");
    toolbar->setMovable(false);
    auto* openAction = toolbar->addAction("打开项目");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::loadProject);
    auto* saveAction = toolbar->addAction("保存项目");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    toolbar->addSeparator();
    auto* runAction = toolbar->addAction("开始计算");
    runAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(runAction, &QAction::triggered, this, &MainWindow::runCalculation);
    connect(toolbar->addAction("查看结果"), &QAction::triggered, this, [this] {
        m_tabs->setCurrentIndex(3);
    });
    statusBar()->addPermanentWidget(new QLabel("界面单位：SI 国际单位制", this));
    m_updating = true;
    loadDefaults();
    m_updating = false;
    for (auto* edit : findChildren<QLineEdit*>()) {
        connect(edit, &QLineEdit::textEdited, this, &MainWindow::markModified);
    }
    connect(m_missionTable, &QTableWidget::cellChanged, this, &MainWindow::markModified);
    statusBar()->showMessage("就绪；Ctrl+O 打开，Ctrl+S 保存，F5 计算。");
}

void MainWindow::markModified()
{
    if (m_updating) return;
    setWindowModified(true);
    m_resultsCurrent = false;
    m_resultStatus->setText("输入已修改，结果尚未更新。请重新计算。");
    m_plot->setEnabled(false);
    statusBar()->showMessage("输入已修改；项目尚未保存，结果尚未更新。");
}

bool MainWindow::confirmDiscard()
{
    if (!isWindowModified()) return true;
    QMessageBox box(QMessageBox::Question, "尚未保存", "当前项目有未保存的修改。", QMessageBox::NoButton, this);
    auto* save = box.addButton("保存", QMessageBox::AcceptRole);
    auto* discard = box.addButton("不保存", QMessageBox::DestructiveRole);
    auto* cancel = box.addButton("取消", QMessageBox::RejectRole);
    box.setDefaultButton(save);
    box.setEscapeButton(cancel);
    box.exec();
    if (box.clickedButton() == discard) return true;
    if (box.clickedButton() != save) return false;
    saveProject();
    return !isWindowModified();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (confirmDiscard()) event->accept();
    else event->ignore();
}

bool MainWindow::validateInput()
{
    const auto showInvalid = [this](QWidget* widget, const QString& message) {
        for (int i = 0; i < m_tabs->count(); ++i) {
            if (m_tabs->widget(i)->isAncestorOf(widget)) m_tabs->setCurrentIndex(i);
        }
        QMessageBox::warning(this, "输入格式有误", message);
        widget->setFocus();
    };
    for (auto* edit : findChildren<QLineEdit*>()) {
        if (!edit->validator()) continue;
        bool ok = false;
        const double value = edit->text().toDouble(&ok);
        if (!ok || !std::isfinite(value) || !edit->hasAcceptableInput()) {
            showInvalid(edit, "请填写有效数字，使用小数点；支持科学计数法，例如 1.2e-4。");
            edit->selectAll();
            return false;
        }
    }
    for (int row = 0; row < m_missionTable->rowCount(); ++row) {
        for (int col = 2; col < m_missionTable->columnCount(); ++col) {
            const auto* item = m_missionTable->item(row, col);
            bool ok = false;
            const double value = item ? item->text().toDouble(&ok) : 0.0;
            if (!ok || !std::isfinite(value)) {
                m_missionTable->setCurrentCell(row, col);
                showInvalid(m_missionTable, QString("第 %1 行的“%2”需要填写有效数字。")
                    .arg(row + 1).arg(m_missionTable->horizontalHeaderItem(col)->text()));
                return false;
            }
        }
    }
    return true;
}

QWidget* MainWindow::buildDesignTab()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* form = new QFormLayout();

    m_cruiseMach = numericEdit(0.8);
    m_cruiseAltitude = numericEdit(12000.0);
    m_combatRadius = numericEdit(1600.0);
    m_payload = numericEdit(2000.0);
    m_fixedWeight = numericEdit(450.0);
    m_takeoffDistance = numericEdit(1200.0);
    m_landingDistance = numericEdit(1100.0);
    m_turnLoadFactor = numericEdit(2.5);
    m_climbRate = numericEdit(25.0);

    form->addRow("巡航马赫数（无量纲）", m_cruiseMach);
    form->addRow("巡航高度 (m)", m_cruiseAltitude);
    form->addRow("作战半径 (m)", m_combatRadius);
    form->addRow("任务载荷 (kg)", m_payload);
    form->addRow("固定设备 / 裕度 (kg)", m_fixedWeight);
    form->addRow("起飞距离 (m)", m_takeoffDistance);
    form->addRow("着陆距离 (m)", m_landingDistance);
    form->addRow("盘旋过载系数（无量纲）", m_turnLoadFactor);
    form->addRow("爬升率 (m/s)", m_climbRate);

    auto* group = new QGroupBox("设计要求", page);
    group->setLayout(form);

    layout->addWidget(group);
    auto* unitHint = new QLabel("单位说明：长度用米（m），质量用千克（kg），时间用秒（s），力用牛顿（N）。马赫数和各类比值、系数为无量纲量。", page);
    unitHint->setWordWrap(true);
    layout->addWidget(unitHint);
    layout->addStretch();
    return page;
}

QWidget* MainWindow::buildMissionTab()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    m_missionTable = new QTableWidget(0, 8, page);
    m_missionTable->setHorizontalHeaderLabels({
        "类型", "名称", "距离 (m)", "持续时间 (s)", "高度 (m)",
        "马赫数（无量纲）", "投放载荷 (kg)", "固定燃油系数（无量纲）"});
    m_missionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_missionTable->horizontalHeader()->setStretchLastSection(true);
    m_missionTable->verticalHeader()->setVisible(false);

    auto* buttons = new QHBoxLayout();
    auto* add = new QPushButton("添加任务段", page);
    auto* remove = new QPushButton("删除选中任务段", page);
    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch();
    connect(add, &QPushButton::clicked, this, &MainWindow::addMissionSegment);
    connect(remove, &QPushButton::clicked, this, &MainWindow::removeMissionSegment);

    layout->addWidget(m_missionTable);
    layout->addLayout(buttons);
    return page;
}

QWidget* MainWindow::buildParameterTab()
{
    auto* page = new QWidget(this);
    auto* layout = new QHBoxLayout(page);

    auto* aeroForm = new QFormLayout();
    m_maxLiftDrag = numericEdit(17.0);
    m_zeroLiftDrag = numericEdit(0.022);
    m_inducedDrag = numericEdit(0.045);
    m_cleanClMax = numericEdit(1.45);
    m_takeoffClMax = numericEdit(1.8);
    m_landingClMax = numericEdit(2.0);
    aeroForm->addRow("最大升阻比（无量纲）", m_maxLiftDrag);
    aeroForm->addRow("零升阻力系数 CD0（无量纲）", m_zeroLiftDrag);
    aeroForm->addRow("诱导阻力因子 K（无量纲）", m_inducedDrag);
    aeroForm->addRow("洁净构型最大升力系数（无量纲）", m_cleanClMax);
    aeroForm->addRow("起飞构型最大升力系数（无量纲）", m_takeoffClMax);
    aeroForm->addRow("着陆构型最大升力系数（无量纲）", m_landingClMax);

    auto* engineForm = new QFormLayout();
    m_engineThrust = numericEdit(65000.0);
    m_cruiseSfc = numericEdit(0.78);
    m_loiterSfc = numericEdit(0.66);
    m_thrustLapse = numericEdit(0.42);
    engineForm->addRow("海平面静推力 (N)", m_engineThrust);
    engineForm->addRow("巡航耗油率 (s⁻¹)", m_cruiseSfc);
    engineForm->addRow("留空耗油率 (s⁻¹)", m_loiterSfc);
    engineForm->addRow("巡航推力衰减系数（无量纲）", m_thrustLapse);
    m_cruiseSfc->setToolTip("按每秒显示，s⁻¹ 与 1/s 含义相同。每小时的数值除以 3600 后得到每秒的数值。");
    m_loiterSfc->setToolTip(m_cruiseSfc->toolTip());

    auto* aeroGroup = new QGroupBox("气动参数", page);
    aeroGroup->setLayout(aeroForm);
    auto* engineGroup = new QGroupBox("发动机参数", page);
    engineGroup->setLayout(engineForm);
    layout->addWidget(aeroGroup);
    layout->addWidget(engineGroup);
    return page;
}

QWidget* MainWindow::buildResultTab()
{
    auto* page = new QWidget(this);
    auto* root = new QHBoxLayout(page);

    auto* splitter = new QSplitter(Qt::Horizontal, page);
    splitter->setChildrenCollapsible(false);
    auto* resultPanel = new QWidget(splitter);
    auto* left = new QVBoxLayout(resultPanel);
    m_resultStatus = new QLabel("尚未运行计算。", resultPanel);
    m_resultStatus->setWordWrap(true);
    left->addWidget(m_resultStatus);
    m_summaryLabel = new QLabel("尚未运行计算。", page);
    m_summaryLabel->setWordWrap(true);
    m_marginLabel = new QLabel("点击约束图可选择设计点。", page);
    m_marginLabel->setWordWrap(true);

    m_iterationTable = new QTableWidget(0, 5, page);
    m_iterationTable->setHorizontalHeaderLabels({
        "迭代次数", "起飞质量 (kg)", "空重系数",
        "燃油系数", "残差 (kg)"});
    m_iterationTable->horizontalHeader()->setStretchLastSection(true);
    m_iterationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_iterationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_segmentResultTable = new QTableWidget(0, 4, page);
    m_segmentResultTable->setHorizontalHeaderLabels({
        "任务段", "燃油系数", "段前质量 (kg)", "段后质量 (kg)"});
    m_segmentResultTable->horizontalHeader()->setStretchLastSection(true);
    m_segmentResultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_segmentResultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_summaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_marginLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    left->addWidget(m_summaryLabel);
    left->addWidget(m_marginLabel);
    left->addWidget(m_iterationTable);
    left->addWidget(m_segmentResultTable);

    m_plot = new ConstraintPlotWidget(page);
    connect(m_plot, &ConstraintPlotWidget::designPointSelected, this, &MainWindow::handleDesignPoint);

    splitter->addWidget(resultPanel);
    splitter->addWidget(m_plot);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    root->addWidget(splitter);
    return page;
}

void MainWindow::loadDefaults()
{
    const ucav::DesignRequirements req;
    writeRequirements(req);
    writeAero({});
    writeEngine({});
    writeMission(ucav::defaultMission(req));
}

void MainWindow::runCalculation()
{
    if (!validateInput()) return;
    const ucav::DesignRequirements req = readRequirements();
    const ucav::AeroParameters aero = readAero();
    const ucav::EngineParameters engine = readEngine();
    const std::vector<ucav::MissionSegment> mission = readMission();

    ucav::WeightAnalysis weightAnalysis;
    ucav::ConstraintAnalysis constraintAnalysis;
    m_lastWeightResult = weightAnalysis.solve(req, aero, engine, mission);
    m_lastConstraintResult = constraintAnalysis.solve(req, aero, engine);

    showWeightResult(m_lastWeightResult);
    m_plot->setConstraintResult(m_lastConstraintResult);
    m_plot->setSelectedPoint(m_lastConstraintResult.recommendedWingLoading,
                             m_lastConstraintResult.recommendedThrustToWeight);
    m_resultsCurrent = true;
    m_plot->setEnabled(true);
    handleDesignPoint(m_lastConstraintResult.recommendedWingLoading,
                      m_lastConstraintResult.recommendedThrustToWeight);
    m_resultStatus->setText("结果对应当前输入。输入修改后需重新计算。");
    statusBar()->showMessage("计算已结束；结果是否收敛请查看摘要。");
    m_tabs->setCurrentIndex(3);
}

void MainWindow::addMissionRow(const ucav::MissionSegment& segment)
{
    const int row = m_missionTable->rowCount();
    m_missionTable->insertRow(row);
    auto* combo = new QComboBox(m_missionTable);
    for (int i = 0; i < 8; ++i) {
        combo->addItem(QString::fromStdString(ucav::missionSegmentTypeName(ucav::missionSegmentTypeFromIndex(i))));
    }
    combo->setCurrentIndex(ucav::missionSegmentTypeIndex(segment.type));
    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::markModified);
    m_missionTable->setCellWidget(row, 0, combo);
    setCell(m_missionTable, row, 1, QString::fromStdString(segment.name));
    setCell(m_missionTable, row, 2, segment.distanceKm * kMetersPerKilometer);
    setCell(m_missionTable, row, 3, segment.durationMin * kSecondsPerMinute);
    setCell(m_missionTable, row, 4, segment.altitudeM);
    setCell(m_missionTable, row, 5, segment.mach);
    setCell(m_missionTable, row, 6, segment.payloadDropKg);
    setCell(m_missionTable, row, 7, segment.fixedFuelFraction);
}

void MainWindow::addMissionSegment()
{
    addMissionRow({ucav::MissionSegmentType::Cruise, "新任务段", 100.0, 0.0, 10000.0, 0.7, 0.0, 0.0});
}

void MainWindow::removeMissionSegment()
{
    const int row = m_missionTable->currentRow();
    if (row >= 0) {
        m_missionTable->removeRow(row);
        markModified();
    }
}

ucav::DesignRequirements MainWindow::readRequirements() const
{
    ucav::DesignRequirements req;
    req.cruiseMach = valueOf(m_cruiseMach);
    req.cruiseAltitudeM = valueOf(m_cruiseAltitude);
    req.combatRadiusKm = valueOf(m_combatRadius) / kMetersPerKilometer;
    req.payloadKg = valueOf(m_payload);
    req.crewAndFixedKg = valueOf(m_fixedWeight);
    req.takeoffDistanceM = valueOf(m_takeoffDistance);
    req.landingDistanceM = valueOf(m_landingDistance);
    req.turnLoadFactor = valueOf(m_turnLoadFactor);
    req.climbRateMs = valueOf(m_climbRate);
    return req;
}

ucav::AeroParameters MainWindow::readAero() const
{
    return {valueOf(m_maxLiftDrag), valueOf(m_zeroLiftDrag), valueOf(m_inducedDrag),
            valueOf(m_cleanClMax), valueOf(m_takeoffClMax), valueOf(m_landingClMax)};
}

ucav::EngineParameters MainWindow::readEngine() const
{
    return {valueOf(m_engineThrust), valueOf(m_cruiseSfc) * kSecondsPerHour,
            valueOf(m_loiterSfc) * kSecondsPerHour, valueOf(m_thrustLapse)};
}

std::vector<ucav::MissionSegment> MainWindow::readMission() const
{
    std::vector<ucav::MissionSegment> mission;
    for (int row = 0; row < m_missionTable->rowCount(); ++row) {
        auto* combo = qobject_cast<QComboBox*>(m_missionTable->cellWidget(row, 0));
        ucav::MissionSegment segment;
        segment.type = ucav::missionSegmentTypeFromIndex(combo ? combo->currentIndex() : 0);
        segment.name = tableText(m_missionTable, row, 1);
        segment.distanceKm = tableNumber(m_missionTable, row, 2) / kMetersPerKilometer;
        segment.durationMin = tableNumber(m_missionTable, row, 3) / kSecondsPerMinute;
        segment.altitudeM = tableNumber(m_missionTable, row, 4);
        segment.mach = tableNumber(m_missionTable, row, 5);
        segment.payloadDropKg = tableNumber(m_missionTable, row, 6);
        segment.fixedFuelFraction = tableNumber(m_missionTable, row, 7);
        mission.push_back(segment);
    }
    return mission;
}

void MainWindow::writeRequirements(const ucav::DesignRequirements& req)
{
    m_cruiseMach->setText(QString::number(req.cruiseMach));
    m_cruiseAltitude->setText(QString::number(req.cruiseAltitudeM));
    m_combatRadius->setText(QString::number(req.combatRadiusKm * kMetersPerKilometer));
    m_payload->setText(QString::number(req.payloadKg));
    m_fixedWeight->setText(QString::number(req.crewAndFixedKg));
    m_takeoffDistance->setText(QString::number(req.takeoffDistanceM));
    m_landingDistance->setText(QString::number(req.landingDistanceM));
    m_turnLoadFactor->setText(QString::number(req.turnLoadFactor));
    m_climbRate->setText(QString::number(req.climbRateMs));
}

void MainWindow::writeAero(const ucav::AeroParameters& aero)
{
    m_maxLiftDrag->setText(QString::number(aero.maxLiftDrag));
    m_zeroLiftDrag->setText(QString::number(aero.zeroLiftDrag));
    m_inducedDrag->setText(QString::number(aero.inducedDragFactor));
    m_cleanClMax->setText(QString::number(aero.cleanClMax));
    m_takeoffClMax->setText(QString::number(aero.takeoffClMax));
    m_landingClMax->setText(QString::number(aero.landingClMax));
}

void MainWindow::writeEngine(const ucav::EngineParameters& engine)
{
    m_engineThrust->setText(QString::number(engine.seaLevelStaticThrustN));
    m_cruiseSfc->setText(QString::number(engine.cruiseSfcPerHour / kSecondsPerHour, 'g', 10));
    m_loiterSfc->setText(QString::number(engine.loiterSfcPerHour / kSecondsPerHour, 'g', 10));
    m_thrustLapse->setText(QString::number(engine.thrustLapseCruise));
}

void MainWindow::writeMission(const std::vector<ucav::MissionSegment>& mission)
{
    m_missionTable->setRowCount(0);
    for (const ucav::MissionSegment& segment : mission) {
        addMissionRow(segment);
    }
}

void MainWindow::showWeightResult(const ucav::WeightResult& result)
{
    m_summaryLabel->setText(QString(
        "是否收敛: %1\n"
        "最大起飞质量: %2 kg\n"
        "空机质量: %3 kg\n"
        "燃油质量: %4 kg\n"
        "空重系数: %5  燃油系数: %6")
        .arg(result.converged ? "是" : "否")
        .arg(result.takeoffWeightKg, 0, 'f', 1)
        .arg(result.emptyWeightKg, 0, 'f', 1)
        .arg(result.fuelWeightKg, 0, 'f', 1)
        .arg(result.emptyFraction, 0, 'f', 4)
        .arg(result.fuelFraction, 0, 'f', 4));

    m_iterationTable->setRowCount(0);
    for (const ucav::WeightIterationRow& item : result.iterations) {
        const int row = m_iterationTable->rowCount();
        m_iterationTable->insertRow(row);
        setCell(m_iterationTable, row, 0, QString::number(item.iteration));
        setCell(m_iterationTable, row, 1, item.takeoffWeightKg);
        setCell(m_iterationTable, row, 2, item.emptyFraction);
        setCell(m_iterationTable, row, 3, item.missionFuelFraction);
        setCell(m_iterationTable, row, 4, item.residualKg);
    }

    m_segmentResultTable->setRowCount(0);
    for (const ucav::MissionSegmentResult& item : result.segmentResults) {
        const int row = m_segmentResultTable->rowCount();
        m_segmentResultTable->insertRow(row);
        setCell(m_segmentResultTable, row, 0, QString::fromStdString(item.name));
        setCell(m_segmentResultTable, row, 1, item.fuelFraction);
        setCell(m_segmentResultTable, row, 2, item.weightBeforeKg);
        setCell(m_segmentResultTable, row, 3, item.weightAfterKg);
    }
}

void MainWindow::showMargins(const ucav::MarginResult& margins)
{
    m_marginLabel->setText(QString(
        "设计点：翼载荷 W/S=%1 N/m²，推重比 T/W=%2（无量纲）\n"
        "巡航裕度 %3%  盘旋裕度 %4%  爬升裕度 %5%  "
        "起飞裕度 %6%  着陆裕度 %7%")
        .arg(margins.selectedWingLoading, 0, 'f', 1)
        .arg(margins.selectedThrustToWeight, 0, 'f', 3)
        .arg(margins.cruiseMarginPercent, 0, 'f', 1)
        .arg(margins.turnMarginPercent, 0, 'f', 1)
        .arg(margins.climbMarginPercent, 0, 'f', 1)
        .arg(margins.takeoffMarginPercent, 0, 'f', 1)
        .arg(margins.landingMarginPercent, 0, 'f', 1));
}

void MainWindow::handleDesignPoint(double wingLoading, double thrustToWeight)
{
    if (!m_resultsCurrent) return;
    ucav::ConstraintAnalysis analysis;
    showMargins(analysis.margins(wingLoading, thrustToWeight, readRequirements(), readAero(), readEngine()));
}

void MainWindow::saveProject()
{
    if (!validateInput()) return;
    QFileDialog dialog(this, "保存项目", m_projectPath, "项目文件 (*.json)");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix("json");
    if (dialog.exec() != QDialog::Accepted) return;
    const QString path = dialog.selectedFiles().value(0);
    if (path.isEmpty()) {
        return;
    }

    const ucav::DesignRequirements req = readRequirements();
    const ucav::AeroParameters aero = readAero();
    const ucav::EngineParameters engine = readEngine();
    QJsonObject root;
    root["requirements"] = QJsonObject{{"cruiseMach", req.cruiseMach}, {"cruiseAltitudeM", req.cruiseAltitudeM},
        {"combatRadiusKm", req.combatRadiusKm}, {"payloadKg", req.payloadKg}, {"crewAndFixedKg", req.crewAndFixedKg},
        {"takeoffDistanceM", req.takeoffDistanceM}, {"landingDistanceM", req.landingDistanceM},
        {"turnLoadFactor", req.turnLoadFactor}, {"climbRateMs", req.climbRateMs}};
    root["aero"] = QJsonObject{{"maxLiftDrag", aero.maxLiftDrag}, {"zeroLiftDrag", aero.zeroLiftDrag},
        {"inducedDragFactor", aero.inducedDragFactor}, {"cleanClMax", aero.cleanClMax},
        {"takeoffClMax", aero.takeoffClMax}, {"landingClMax", aero.landingClMax}};
    root["engine"] = QJsonObject{{"seaLevelStaticThrustN", engine.seaLevelStaticThrustN},
        {"cruiseSfcPerHour", engine.cruiseSfcPerHour}, {"loiterSfcPerHour", engine.loiterSfcPerHour},
        {"thrustLapseCruise", engine.thrustLapseCruise}};

    QJsonArray missionArray;
    for (const ucav::MissionSegment& segment : readMission()) {
        missionArray.append(QJsonObject{{"type", ucav::missionSegmentTypeIndex(segment.type)},
            {"name", QString::fromStdString(segment.name)}, {"distanceKm", segment.distanceKm},
            {"durationMin", segment.durationMin}, {"altitudeM", segment.altitudeM}, {"mach", segment.mach},
            {"payloadDropKg", segment.payloadDropKg}, {"fixedFuelFraction", segment.fixedFuelFraction}});
    }
    root["mission"] = missionArray;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "保存失败", file.errorString());
        return;
    }
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        QMessageBox::warning(this, "保存失败", "项目未能完整保存：" + file.errorString());
        return;
    }
    m_projectPath = path;
    setWindowTitle(QFileInfo(path).fileName() + "[*] - 无人作战飞机初步设计工具");
    setWindowModified(false);
    statusBar()->showMessage("项目已保存：" + path);
}

void MainWindow::loadProject()
{
    const QString path = QFileDialog::getOpenFileName(this, "加载项目", QString(), "无人机项目 (*.json)");
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "加载失败", file.errorString());
        return;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, "加载失败", "文件不是有效的 JSON 项目对象。当前输入未改变。");
        return;
    }
    const QJsonObject root = document.object();
    const auto hasNumbers = [](const QJsonObject& object, const QStringList& keys) {
        for (const QString& key : keys) {
            if (!object.value(key).isDouble() || !std::isfinite(object.value(key).toDouble())) return false;
        }
        return true;
    };
    bool valid = root["requirements"].isObject() && root["aero"].isObject()
        && root["engine"].isObject() && root["mission"].isArray();
    valid = valid && hasNumbers(root["requirements"].toObject(),
        {"cruiseMach", "cruiseAltitudeM", "combatRadiusKm", "payloadKg", "crewAndFixedKg",
         "takeoffDistanceM", "landingDistanceM", "turnLoadFactor", "climbRateMs"});
    valid = valid && hasNumbers(root["aero"].toObject(),
        {"maxLiftDrag", "zeroLiftDrag", "inducedDragFactor", "cleanClMax", "takeoffClMax", "landingClMax"});
    valid = valid && hasNumbers(root["engine"].toObject(),
        {"seaLevelStaticThrustN", "cruiseSfcPerHour", "loiterSfcPerHour", "thrustLapseCruise"});
    for (const QJsonValue& value : root["mission"].toArray()) {
        const auto object = value.toObject();
        const double type = object["type"].toDouble(-1);
        valid = valid && value.isObject() && object["name"].isString()
            && hasNumbers(object, {"type", "distanceKm", "durationMin", "altitudeM", "mach", "payloadDropKg", "fixedFuelFraction"})
            && type >= 0 && type <= 7 && std::floor(type) == type;
    }
    if (!valid) {
        QMessageBox::warning(this, "加载失败", "项目缺少必要字段或字段类型不正确。当前输入未改变。");
        return;
    }
    if (!confirmDiscard()) return;
    QScopedValueRollback<bool> updating(m_updating, true);
    const QJsonObject r = root["requirements"].toObject();
    writeRequirements({r["cruiseMach"].toDouble(), r["cruiseAltitudeM"].toDouble(), r["combatRadiusKm"].toDouble(),
        r["payloadKg"].toDouble(), r["crewAndFixedKg"].toDouble(), r["takeoffDistanceM"].toDouble(),
        r["landingDistanceM"].toDouble(), r["turnLoadFactor"].toDouble(), r["climbRateMs"].toDouble()});

    const QJsonObject a = root["aero"].toObject();
    writeAero({a["maxLiftDrag"].toDouble(), a["zeroLiftDrag"].toDouble(), a["inducedDragFactor"].toDouble(),
        a["cleanClMax"].toDouble(), a["takeoffClMax"].toDouble(), a["landingClMax"].toDouble()});

    const QJsonObject e = root["engine"].toObject();
    writeEngine({e["seaLevelStaticThrustN"].toDouble(), e["cruiseSfcPerHour"].toDouble(),
        e["loiterSfcPerHour"].toDouble(), e["thrustLapseCruise"].toDouble()});

    std::vector<ucav::MissionSegment> mission;
    for (const QJsonValue& value : root["mission"].toArray()) {
        const QJsonObject object = value.toObject();
        mission.push_back({ucav::missionSegmentTypeFromIndex(object["type"].toInt()),
            object["name"].toString().toStdString(), object["distanceKm"].toDouble(),
            object["durationMin"].toDouble(), object["altitudeM"].toDouble(), object["mach"].toDouble(),
            object["payloadDropKg"].toDouble(), object["fixedFuelFraction"].toDouble()});
    }
    writeMission(mission);
    m_projectPath = path;
    setWindowTitle(QFileInfo(path).fileName() + "[*] - 无人作战飞机初步设计工具");
    setWindowModified(false);
    m_resultsCurrent = false;
    m_iterationTable->setRowCount(0);
    m_segmentResultTable->setRowCount(0);
    m_summaryLabel->setText("尚未运行计算。");
    m_marginLabel->clear();
    m_plot->clearResult();
    m_plot->setEnabled(false);
    m_resultStatus->setText("项目已加载，尚未生成当前项目的结果。");
    statusBar()->showMessage("项目已加载：" + path);
}
