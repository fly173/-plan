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

namespace {

constexpr double kMetersPerKilometer = 1000.0;
constexpr double kSecondsPerMinute = 60.0;
constexpr double kSecondsPerHour = 3600.0;

QLineEdit* numericEdit(double value)
{
    auto* edit = new QLineEdit(QString::number(value, 'g', 10));
    auto* validator = new QDoubleValidator(edit);
    validator->setNotation(QDoubleValidator::StandardNotation);
    edit->setValidator(validator);
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
    setWindowTitle("无人作战飞机初步设计工具");
    resize(1180, 760);

    auto* tabs = new QTabWidget(this);
    tabs->addTab(buildDesignTab(), "设计要求");
    tabs->addTab(buildMissionTab(), "任务剖面");
    tabs->addTab(buildParameterTab(), "气动 / 发动机");
    tabs->addTab(buildResultTab(), "计算结果");
    setCentralWidget(tabs);

    loadDefaults();
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

    form->addRow("巡航马赫数", m_cruiseMach);
    form->addRow("巡航高度 (m)", m_cruiseAltitude);
    form->addRow("作战半径 (m)", m_combatRadius);
    form->addRow("任务载荷 (kg)", m_payload);
    form->addRow("固定设备 / 裕度 (kg)", m_fixedWeight);
    form->addRow("起飞距离 (m)", m_takeoffDistance);
    form->addRow("着陆距离 (m)", m_landingDistance);
    form->addRow("盘旋过载系数", m_turnLoadFactor);
    form->addRow("爬升率 (m/s)", m_climbRate);

    auto* group = new QGroupBox("设计要求", page);
    group->setLayout(form);

    auto* buttons = new QHBoxLayout();
    auto* calculate = new QPushButton("开始计算", page);
    auto* save = new QPushButton("保存项目", page);
    auto* load = new QPushButton("加载项目", page);
    buttons->addWidget(calculate);
    buttons->addWidget(save);
    buttons->addWidget(load);
    buttons->addStretch();

    connect(calculate, &QPushButton::clicked, this, &MainWindow::runCalculation);
    connect(save, &QPushButton::clicked, this, &MainWindow::saveProject);
    connect(load, &QPushButton::clicked, this, &MainWindow::loadProject);

    layout->addWidget(group);
    layout->addLayout(buttons);
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
        "马赫数", "投放载荷 (kg)", "固定燃油系数"});
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
    aeroForm->addRow("最大升阻比", m_maxLiftDrag);
    aeroForm->addRow("零升阻力系数 CD0", m_zeroLiftDrag);
    aeroForm->addRow("诱导阻力因子 K", m_inducedDrag);
    aeroForm->addRow("洁净构型 CLmax", m_cleanClMax);
    aeroForm->addRow("起飞构型 CLmax", m_takeoffClMax);
    aeroForm->addRow("着陆构型 CLmax", m_landingClMax);

    auto* engineForm = new QFormLayout();
    m_engineThrust = numericEdit(65000.0);
    m_cruiseSfc = numericEdit(0.78);
    m_loiterSfc = numericEdit(0.66);
    m_thrustLapse = numericEdit(0.42);
    engineForm->addRow("海平面静推力 (N)", m_engineThrust);
    engineForm->addRow("巡航耗油率 (1/s)", m_cruiseSfc);
    engineForm->addRow("留空耗油率 (1/s)", m_loiterSfc);
    engineForm->addRow("巡航推力衰减系数", m_thrustLapse);

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

    auto* left = new QVBoxLayout();
    m_summaryLabel = new QLabel("尚未运行计算。", page);
    m_summaryLabel->setWordWrap(true);
    m_marginLabel = new QLabel("点击约束图可选择设计点。", page);
    m_marginLabel->setWordWrap(true);

    m_iterationTable = new QTableWidget(0, 5, page);
    m_iterationTable->setHorizontalHeaderLabels({
        "迭代次数", "起飞重量 (kg)", "空重系数",
        "燃油系数", "残差 (kg)"});
    m_iterationTable->horizontalHeader()->setStretchLastSection(true);

    m_segmentResultTable = new QTableWidget(0, 4, page);
    m_segmentResultTable->setHorizontalHeaderLabels({
        "任务段", "燃油系数", "段前重量 (kg)", "段后重量 (kg)"});
    m_segmentResultTable->horizontalHeader()->setStretchLastSection(true);

    left->addWidget(m_summaryLabel);
    left->addWidget(m_marginLabel);
    left->addWidget(m_iterationTable);
    left->addWidget(m_segmentResultTable);

    m_plot = new ConstraintPlotWidget(page);
    connect(m_plot, &ConstraintPlotWidget::designPointSelected, this, &MainWindow::handleDesignPoint);

    root->addLayout(left, 1);
    root->addWidget(m_plot, 2);
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
    handleDesignPoint(m_lastConstraintResult.recommendedWingLoading,
                      m_lastConstraintResult.recommendedThrustToWeight);
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
        "最大起飞重量: %2 kg\n"
        "空机重量: %3 kg\n"
        "燃油重量: %4 kg\n"
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
        "设计点: W/S=%1 N/m^2, T/W=%2\n"
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
    ucav::ConstraintAnalysis analysis;
    showMargins(analysis.margins(wingLoading, thrustToWeight, readRequirements(), readAero(), readEngine()));
}

void MainWindow::saveProject()
{
    const QString path = QFileDialog::getSaveFileName(this, "保存项目", QString(), "无人机项目 (*.json)");
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

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "保存失败", file.errorString());
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
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
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
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
}
