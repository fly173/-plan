#pragma once

#include "../core/ConstraintAnalysis.h"
#include "../core/WeightAnalysis.h"
#include "ConstraintPlotWidget.h"

#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QTableWidget>
#include <QTabWidget>

class QCloseEvent;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void markModified();
    bool confirmDiscard();
    bool validateInput();
    QTabWidget* m_tabs = nullptr;
    QLabel* m_resultStatus = nullptr;
    bool m_updating = false;
    bool m_resultsCurrent = false;
    QString m_projectPath;
    QWidget* buildDesignTab();
    QWidget* buildMissionTab();
    QWidget* buildParameterTab();
    QWidget* buildResultTab();

    void loadDefaults();
    void runCalculation();
    void addMissionRow(const ucav::MissionSegment& segment);
    void addMissionSegment();
    void removeMissionSegment();
    void saveProject();
    void loadProject();
    void handleDesignPoint(double wingLoading, double thrustToWeight);

    ucav::DesignRequirements readRequirements() const;
    ucav::AeroParameters readAero() const;
    ucav::EngineParameters readEngine() const;
    std::vector<ucav::MissionSegment> readMission() const;

    void writeRequirements(const ucav::DesignRequirements& req);
    void writeAero(const ucav::AeroParameters& aero);
    void writeEngine(const ucav::EngineParameters& engine);
    void writeMission(const std::vector<ucav::MissionSegment>& mission);
    void showWeightResult(const ucav::WeightResult& result);
    void showMargins(const ucav::MarginResult& margins);

    QLineEdit* m_cruiseMach = nullptr;
    QLineEdit* m_cruiseAltitude = nullptr;
    QLineEdit* m_combatRadius = nullptr;
    QLineEdit* m_payload = nullptr;
    QLineEdit* m_fixedWeight = nullptr;
    QLineEdit* m_takeoffDistance = nullptr;
    QLineEdit* m_landingDistance = nullptr;
    QLineEdit* m_turnLoadFactor = nullptr;
    QLineEdit* m_climbRate = nullptr;

    QLineEdit* m_maxLiftDrag = nullptr;
    QLineEdit* m_zeroLiftDrag = nullptr;
    QLineEdit* m_inducedDrag = nullptr;
    QLineEdit* m_cleanClMax = nullptr;
    QLineEdit* m_takeoffClMax = nullptr;
    QLineEdit* m_landingClMax = nullptr;
    QLineEdit* m_engineThrust = nullptr;
    QLineEdit* m_cruiseSfc = nullptr;
    QLineEdit* m_loiterSfc = nullptr;
    QLineEdit* m_thrustLapse = nullptr;

    QTableWidget* m_missionTable = nullptr;
    QTableWidget* m_iterationTable = nullptr;
    QTableWidget* m_segmentResultTable = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QLabel* m_marginLabel = nullptr;
    ConstraintPlotWidget* m_plot = nullptr;

    ucav::WeightResult m_lastWeightResult;
    ucav::ConstraintResult m_lastConstraintResult;
};
