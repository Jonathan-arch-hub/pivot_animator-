#pragma once
#include <QMainWindow>
#include <QActionGroup>
#include "animdata.h"
#include "canvaswidget.h"
#include "timelinewidget.h"

QT_BEGIN_NAMESPACE
class QLabel; class QSpinBox; class QDoubleSpinBox;
class QColorDialog; class QPushButton; class QListWidget;
class QListWidgetItem; class QSlider; class QCheckBox;
class QComboBox; class QDockWidget; class QLineEdit;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();

    void addHuman();
    void addStick();
    void addEmpty();
    void deleteFigure();

    void addFrame();
    void dupFrame();
    void deleteFrame();
    void onFrameChanged(int idx);
    void onFrameSelected(int idx);

    void togglePlay();
    void stopPlayback();

    void onSelectionChanged();
    void onToolChanged(QAction*);
    void onProjectModified();

    void showCodeEditor();
    void exportAnimation();

    void pickNodeColor();
    void pickBoneColor();
    void pickBgColor();

    void updateFigureList();
    void onFigureListItemClicked(QListWidgetItem*);

private:
    void setupMenus();
    void setupToolbar();
    void setupDocks();
    void setupStatusBar();
    void refreshAll();
    void applyProjectToUI();

    AnimProject   m_project;
    CanvasWidget *m_canvas    = nullptr;
    TimelineWidget *m_timeline = nullptr;

    // Docks
    QDockWidget *m_figDock  = nullptr;
    QDockWidget *m_propDock = nullptr;

    // Figure list
    QListWidget *m_figList = nullptr;

    // Properties panel widgets
    QDoubleSpinBox *m_propX    = nullptr;
    QDoubleSpinBox *m_propY    = nullptr;
    QDoubleSpinBox *m_propSize = nullptr;
    QPushButton    *m_propNodeColor = nullptr;
    QCheckBox      *m_propVisible   = nullptr;

    // Node defaults
    QPushButton *m_nodeColorBtn  = nullptr;
    QSlider     *m_nodeSizeSlider= nullptr;
    QPushButton *m_boneColorBtn  = nullptr;
    QSlider     *m_boneWidthSlider=nullptr;
    QSlider     *m_boneRoundSlider=nullptr;

    // Frame / anim
    QSpinBox  *m_frameDurSpin = nullptr;
    QLineEdit *m_frameLabelEdit=nullptr;
    QSpinBox  *m_fpsSpin      = nullptr;
    QComboBox *m_loopCombo    = nullptr;
    QPushButton *m_bgColorBtn = nullptr;
    QCheckBox   *m_gridCheck  = nullptr;
    QCheckBox   *m_rulerCheck = nullptr;

    // Toolbar actions
    QAction *m_actPlay  = nullptr;
    QAction *m_actStop  = nullptr;

    QString m_currentFile;
    bool    m_modified = false;

    void setModified(bool m);
    void initNewProject();
};
