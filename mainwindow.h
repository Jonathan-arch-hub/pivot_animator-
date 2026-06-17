#pragma once
#include <QMainWindow>
#include <QToolBar>
#include <QDockWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QColorDialog>
#include <QPushButton>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QTimer>
#include <QDialog>
#include <QListWidget>
#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <algorithm>
#include "types.h"
#include "canvas.h"
#include "timeline.h"
#include "codepanel.h"
#include "interpreter.h"
#include "exporter.h"

class ColorButton : public QPushButton {
    Q_OBJECT
    QColor col;
public:
    ColorButton(QColor c, QWidget* p=nullptr) : QPushButton(p), col(c) {
        setFixedSize(28,22); updateStyle();
    }
    QColor color() const { return col; }
    void setColor(QColor c) { col=c; updateStyle(); }
signals:
    void colorChanged(QColor);
protected:
    void mousePressEvent(QMouseEvent*) override {
        QColor c = QColorDialog::getColor(col, this, "اختر لوناً");
        if (c.isValid()) { col=c; updateStyle(); emit colorChanged(c); }
    }
    void updateStyle() {
        setStyleSheet(QString("background:%1;border:1px solid #999;border-radius:3px;").arg(col.name()));
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* p=nullptr) : QMainWindow(p) {
        setWindowTitle("Pivot Animator Pro — C++/Qt6");
        resize(1280, 780);
        setMinimumSize(900, 600);

        buildMenus();
        buildToolbar();
        buildCentral();
        buildLeftPanel();
        buildRightPanel();
        buildTimeline();
        buildStatusBar();

        // init
        addHumanFigure();
        canvas->figures = &frames[currentFrame].figures;
        syncFromFrame();
        setTool(Tool::Select);
    }

private:
    // state
    QVector<Frame> frames;
    int currentFrame = 0;
    bool playing = false;
    int playDir  = 1;
    QString loopMode = "loop";
    int fps = 12;

    // widgets
    Canvas*    canvas;
    Timeline*  timeline;
    CodePanel* codePanel;

    // left panel
    QListWidget*  figureList;
    ColorButton*  nodeColorBtn;
    QSpinBox*     nodeSizeSpin;
    ColorButton*  boneColorBtn;
    QSpinBox*     boneWidthSpin;
    QCheckBox*    boneRoundChk;

    // right panel
    QDoubleSpinBox* propX;
    QDoubleSpinBox* propY;
    QDoubleSpinBox* propSz;
    ColorButton*    propCol;
    QCheckBox*      propVis;
    QSpinBox*       frameDurSpin;
    QLineEdit*      frameLabelEdit;
    QSpinBox*       fpsSpin;
    QComboBox*      loopCombo;
    ColorButton*    bgColorBtn;
    QCheckBox*      gridChk;
    QCheckBox*      rulersChk;

    // playback
    QTimer* playTimer;
    QAction* playAct;

    // undo
    QVector<QVector<Frame>> undoStack;
    QVector<QVector<Frame>> redoStack;

    // ─── Helpers ──────────────────────────────────────────────────
    void pushUndo() {
        undoStack.append(frames);
        if (undoStack.size()>50) undoStack.removeFirst();
        redoStack.clear();
    }

    Frame& cur() { return frames[currentFrame]; }
    QVector<Figure>& figs() { return cur().figures; }

    Figure* selectedFig()  { return canvas->selectedFig; }
    Node*   selectedNode() { return canvas->selectedNode; }

    // ─── Build UI ─────────────────────────────────────────────────
    void buildMenus() {
        auto* mb = menuBar();

        // File
        auto* fileM = mb->addMenu("ملف");
        fileM->addAction("جديد",    this, &MainWindow::newProject,  QKeySequence::New);
        fileM->addAction("فتح JSON",this, &MainWindow::openProject,  QKeySequence::Open);
        fileM->addAction("حفظ JSON",this, &MainWindow::saveProject,  QKeySequence::Save);
        fileM->addSeparator();
        auto* expM = fileM->addMenu("تصدير");
        expM->addAction("PNG (سلسلة إطارات)", this,[this]{Exporter::exportPNGSequence(frames,canvas->bgColor);});
        expM->addAction("Sprite Sheet",        this,[this]{Exporter::exportSpriteSheet(frames,canvas->bgColor);});
        expM->addAction("HTML متحرك",          this,[this]{Exporter::exportHTML(frames,canvas->bgColor);});
        expM->addAction("GDScript (Godot)",    this,[this]{Exporter::exportGDScript(frames);});
        expM->addAction("CSS Animation",       this,[this]{Exporter::exportCSSAnimation(frames);});
        fileM->addSeparator();
        fileM->addAction("خروج", qApp, &QApplication::quit, QKeySequence::Quit);

        // Edit
        auto* editM = mb->addMenu("تعديل");
        editM->addAction("تراجع", this, &MainWindow::undo, QKeySequence::Undo);
        editM->addAction("إعادة", this, &MainWindow::redo, QKeySequence::Redo);
        editM->addSeparator();
        editM->addAction("حذف المحدد", this, &MainWindow::deleteSelected, QKeySequence::Delete);
        editM->addAction("نسخ إطار",  this, &MainWindow::dupFrame);

        // View
        auto* viewM = mb->addMenu("عرض");
        viewM->addAction("محرر الكود", this, &MainWindow::toggleCode, QKeySequence("Ctrl+E"));
        viewM->addAction("ملء الشاشة", this, &QMainWindow::showFullScreen, QKeySequence::FullScreen);
        viewM->addAction("نافذة عادية",this, &QMainWindow::showNormal);

        // Animation
        auto* animM = mb->addMenu("أنيميشن");
        animM->addAction("تشغيل/إيقاف", this, &MainWindow::togglePlay, QKeySequence("Space"));
        animM->addAction("توقف",         this, &MainWindow::stopAnim,  QKeySequence("Escape"));
        animM->addSeparator();
        animM->addAction("إضافة شخصية إنسان", this, &MainWindow::addHumanFigure);
        animM->addAction("إضافة شخصية عصا",   this, &MainWindow::addStickFigure);
    }

    void buildToolbar() {
        auto* tb = addToolBar("أدوات");
        tb->setMovable(false);
        tb->setIconSize({18,18});

        auto addBtn = [&](const QString& txt, const QString& tip, auto slot, bool checkable=false) {
            auto* a = tb->addAction(txt);
            a->setToolTip(tip);
            a->setCheckable(checkable);
            connect(a, &QAction::triggered, this, slot);
            return a;
        };

        addBtn("⬡ تحديد",   "أداة التحديد والتحريك (S)",  [this]{ setTool(Tool::Select);  }, false);
        addBtn("● مفصل",    "إضافة مفصل (N)",             [this]{ setTool(Tool::AddNode); }, false);
        addBtn("━ عظمة",    "رسم عظمة بين مفصلين (B)",    [this]{ setTool(Tool::AddBone); }, false);
        tb->addSeparator();
        addBtn("👤 إنسان",  "شخصية إنسان كاملة",           [this]{ pushUndo(); addHumanFigure(); });
        addBtn("🕴 عصا",    "شخصية عصا بسيطة",             [this]{ pushUndo(); addStickFigure(); });
        addBtn("+ شخصية",   "شخصية فارغة",                 [this]{ pushUndo(); addEmptyFigure(); });
        tb->addSeparator();
        addBtn("+ إطار",    "إضافة إطار جديد",             [this]{ pushUndo(); addFrame(); });
        addBtn("⎘ نسخ",     "نسخ الإطار الحالي",           [this]{ pushUndo(); dupFrame(); });
        addBtn("🗑 حذف إطار","حذف الإطار الحالي",           [this]{ pushUndo(); delFrame(); });
        tb->addSeparator();
        playAct = addBtn("▶ تشغيل", "تشغيل الأنيميشن (Space)", [this]{ togglePlay(); }, true);
        addBtn("■ توقف",    "توقف وعودة للبداية (Esc)",   [this]{ stopAnim(); });
        tb->addSeparator();
        addBtn("</> كود",   "محرر الكود (Ctrl+E)",          [this]{ toggleCode(); });
        addBtn("↗ تصدير",   "تصدير الأنيميشن",             [this]{ showExportMenu(); });
    }

    void buildCentral() {
        auto* central = new QWidget;
        auto* vlay = new QVBoxLayout(central);
        vlay->setContentsMargins(0,0,0,0);
        vlay->setSpacing(0);

        // canvas + code panel stack
        auto* canvasStack = new QWidget;
        canvasStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        auto* slay = new QStackedLayout(canvasStack); // NOT QStackedLayout, just overlap
        // Use QVBoxLayout with canvas and overlay codePanel
        delete canvasStack->layout();
        auto* csl = new QVBoxLayout(canvasStack);
        csl->setContentsMargins(0,0,0,0);

        canvas = new Canvas;
        canvas->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        csl->addWidget(canvas);

        codePanel = new CodePanel(canvasStack);
        codePanel->hide();
        // Position code panel over canvas with geometry
        connect(canvas, &Canvas::resized, [this](){
            if (codePanel->isVisible())
                codePanel->setGeometry(canvas->rect());
        });

        vlay->addWidget(canvasStack);

        // timeline
        timeline = new Timeline;
        vlay->addWidget(timeline);

        setCentralWidget(central);

        playTimer = new QTimer(this);
        connect(playTimer, &QTimer::timeout, this, &MainWindow::nextPlayFrame);

        // canvas signals
        connect(canvas, &Canvas::figuresChanged,  this, &MainWindow::onFiguresChanged);
        connect(canvas, &Canvas::nodeSelected,    this, &MainWindow::onNodeSelected);
        connect(canvas, &Canvas::selectionCleared,this, &MainWindow::onSelectionCleared);
        connect(canvas, &Canvas::statusMessage,   this, [this](const QString& s){ statusBar()->showMessage(s); });

        // timeline
        connect(timeline, &Timeline::frameClicked, this, &MainWindow::loadFrame);

        // code panel
        connect(codePanel, &CodePanel::runRequested, this, &MainWindow::runCode);
    }

    QGroupBox* makeGroup(const QString& title) {
        auto* g = new QGroupBox(title);
        g->setStyleSheet("QGroupBox{font-size:11px;font-weight:600;color:#555;border:none;border-top:1px solid #ddd;margin-top:6px;padding-top:4px;}"
                         "QGroupBox::title{subcontrol-origin:margin;padding:0 4px;}");
        return g;
    }

    void buildLeftPanel() {
        auto* dock = new QDockWidget("الشخصيات", this);
        dock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        dock->setMinimumWidth(180);

        auto* w = new QWidget;
        auto* lay = new QVBoxLayout(w);
        lay->setContentsMargins(6,6,6,6);
        lay->setSpacing(4);

        // figure list
        auto* fg = makeGroup("الشخصيات");
        auto* fgl = new QVBoxLayout(fg);
        figureList = new QListWidget;
        figureList->setMaximumHeight(150);
        figureList->setStyleSheet("font-size:12px;");
        auto* fbRow = new QHBoxLayout;
        auto* addFigBtn  = new QPushButton("+"); addFigBtn->setFixedWidth(28);
        auto* delFigBtn  = new QPushButton("✕"); delFigBtn->setFixedWidth(28);
        auto* renFigBtn  = new QPushButton("✎"); renFigBtn->setFixedWidth(28);
        auto* visFigBtn  = new QPushButton("👁"); visFigBtn->setFixedWidth(28);
        fbRow->addWidget(addFigBtn); fbRow->addWidget(delFigBtn); fbRow->addWidget(renFigBtn); fbRow->addWidget(visFigBtn); fbRow->addStretch();
        fgl->addWidget(figureList); fgl->addLayout(fbRow);
        connect(addFigBtn,  &QPushButton::clicked, this, [this]{ pushUndo(); addEmptyFigure(); });
        connect(delFigBtn,  &QPushButton::clicked, this, &MainWindow::deleteFigure);
        connect(renFigBtn,  &QPushButton::clicked, this, &MainWindow::renameFigure);
        connect(visFigBtn,  &QPushButton::clicked, this, &MainWindow::toggleFigureVisible);
        connect(figureList, &QListWidget::currentRowChanged, this, &MainWindow::onFigureListSelect);
        lay->addWidget(fg);

        // node props
        auto* ng = makeGroup("المفصل الجديد");
        auto* ngl = new QGridLayout(ng);
        ngl->setSpacing(4);
        ngl->addWidget(new QLabel("لون:"),0,0);
        nodeColorBtn = new ColorButton(QColor("#378ADD"));
        ngl->addWidget(nodeColorBtn,0,1);
        ngl->addWidget(new QLabel("حجم:"),1,0);
        nodeSizeSpin = new QSpinBox; nodeSizeSpin->setRange(2,40); nodeSizeSpin->setValue(8);
        ngl->addWidget(nodeSizeSpin,1,1);
        connect(nodeColorBtn, &ColorButton::colorChanged, this,[this](QColor c){ canvas->defNodeColor=c; });
        connect(nodeSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this,[this](int v){ canvas->defNodeSize=v; });
        lay->addWidget(ng);

        // bone props
        auto* bg = makeGroup("العظمة الجديدة");
        auto* bgl = new QGridLayout(bg);
        bgl->setSpacing(4);
        bgl->addWidget(new QLabel("لون:"),0,0);
        boneColorBtn = new ColorButton(QColor("#888780"));
        bgl->addWidget(boneColorBtn,0,1);
        bgl->addWidget(new QLabel("سمك:"),1,0);
        boneWidthSpin = new QSpinBox; boneWidthSpin->setRange(1,32); boneWidthSpin->setValue(4);
        bgl->addWidget(boneWidthSpin,1,1);
        bgl->addWidget(new QLabel("مستدير:"),2,0);
        boneRoundChk = new QCheckBox;
        bgl->addWidget(boneRoundChk,2,1);
        connect(boneColorBtn, &ColorButton::colorChanged, this,[this](QColor c){ canvas->defBoneColor=c; });
        connect(boneWidthSpin,qOverload<int>(&QSpinBox::valueChanged),this,[this](int v){ canvas->defBoneWidth=v; });
        connect(boneRoundChk, &QCheckBox::toggled, this,[this](bool v){ canvas->defBoneRound=v; });
        lay->addWidget(bg);

        lay->addStretch();
        dock->setWidget(w);
        addDockWidget(Qt::LeftDockWidgetArea, dock);
    }

    void buildRightPanel() {
        auto* dock = new QDockWidget("الخصائص", this);
        dock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        dock->setMinimumWidth(180);

        auto* w = new QWidget;
        auto* lay = new QVBoxLayout(w);
        lay->setContentsMargins(6,6,6,6);
        lay->setSpacing(4);

        // node props
        auto* npg = makeGroup("المفصل المحدد");
        auto* npgl = new QGridLayout(npg);
        npgl->setSpacing(4);
        npgl->addWidget(new QLabel("X:"),0,0);
        propX = new QDoubleSpinBox; propX->setRange(-9999,9999); propX->setDecimals(1);
        npgl->addWidget(propX,0,1);
        npgl->addWidget(new QLabel("Y:"),1,0);
        propY = new QDoubleSpinBox; propY->setRange(-9999,9999); propY->setDecimals(1);
        npgl->addWidget(propY,1,1);
        npgl->addWidget(new QLabel("حجم:"),2,0);
        propSz = new QDoubleSpinBox; propSz->setRange(1,100); propSz->setDecimals(1);
        npgl->addWidget(propSz,2,1);
        npgl->addWidget(new QLabel("لون:"),3,0);
        propCol = new ColorButton(QColor("#378ADD"));
        npgl->addWidget(propCol,3,1);
        npgl->addWidget(new QLabel("مرئي:"),4,0);
        propVis = new QCheckBox; propVis->setChecked(true);
        npgl->addWidget(propVis,4,1);
        connect(propX,  qOverload<double>(&QDoubleSpinBox::valueChanged),this,&MainWindow::applyNodeProps);
        connect(propY,  qOverload<double>(&QDoubleSpinBox::valueChanged),this,&MainWindow::applyNodeProps);
        connect(propSz, qOverload<double>(&QDoubleSpinBox::valueChanged),this,&MainWindow::applyNodeProps);
        connect(propCol,&ColorButton::colorChanged,this,[this](QColor){ applyNodeProps(); });
        connect(propVis,&QCheckBox::toggled,       this,[this](bool){ applyNodeProps(); });
        lay->addWidget(npg);

        // frame props
        auto* fpg = makeGroup("الإطار الحالي");
        auto* fpgl = new QGridLayout(fpg);
        fpgl->setSpacing(4);
        fpgl->addWidget(new QLabel("مدة (ms):"),0,0);
        frameDurSpin = new QSpinBox; frameDurSpin->setRange(16,10000); frameDurSpin->setValue(100);
        fpgl->addWidget(frameDurSpin,0,1);
        fpgl->addWidget(new QLabel("تسمية:"),1,0);
        frameLabelEdit = new QLineEdit;
        fpgl->addWidget(frameLabelEdit,1,1);
        connect(frameDurSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ cur().duration=v; timeline->rebuild(frames,currentFrame,canvas->bgColor); });
        connect(frameLabelEdit, &QLineEdit::textChanged, this, [this](const QString& t){ cur().label=t; timeline->rebuild(frames,currentFrame,canvas->bgColor); });
        lay->addWidget(fpg);

        // anim settings
        auto* ag = makeGroup("إعدادات الأنيميشن");
        auto* agl = new QGridLayout(ag);
        agl->setSpacing(4);
        agl->addWidget(new QLabel("FPS:"),0,0);
        fpsSpin = new QSpinBox; fpsSpin->setRange(1,120); fpsSpin->setValue(12);
        agl->addWidget(fpsSpin,0,1);
        agl->addWidget(new QLabel("تكرار:"),1,0);
        loopCombo = new QComboBox;
        loopCombo->addItems({"دائم","مرة واحدة","ذهاب وإياب"});
        agl->addWidget(loopCombo,1,1);
        connect(fpsSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v){ fps=v; });
        connect(loopCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int i){ loopMode=QStringList{"loop","once","pingpong"}[i]; });
        lay->addWidget(ag);

        // view settings
        auto* vg = makeGroup("العرض");
        auto* vgl = new QGridLayout(vg);
        vgl->setSpacing(4);
        vgl->addWidget(new QLabel("خلفية:"),0,0);
        bgColorBtn = new ColorButton(QColor("#f0f0f0"));
        vgl->addWidget(bgColorBtn,0,1);
        vgl->addWidget(new QLabel("شبكة:"),1,0);
        gridChk = new QCheckBox; gridChk->setChecked(true);
        vgl->addWidget(gridChk,1,1);
        vgl->addWidget(new QLabel("مساطر:"),2,0);
        rulersChk = new QCheckBox;
        vgl->addWidget(rulersChk,2,1);
        connect(bgColorBtn, &ColorButton::colorChanged,  this,[this](QColor c){ canvas->bgColor=c; canvas->update(); });
        connect(gridChk,    &QCheckBox::toggled,         this,[this](bool v){ canvas->showGrid=v; canvas->update(); });
        connect(rulersChk,  &QCheckBox::toggled,         this,[this](bool v){ canvas->showRulers=v; canvas->update(); });
        lay->addWidget(vg);

        lay->addStretch();
        dock->setWidget(w);
        addDockWidget(Qt::RightDockWidgetArea, dock);
    }

    void buildTimeline() {
        // timeline is in central widget, built in buildCentral
    }

    void buildStatusBar() {
        statusBar()->showMessage("جاهز — اسحب المفاصل لتحريكها | Space للتشغيل | Ctrl+E للكود");
    }

    // ─── Figures ──────────────────────────────────────────────────
    void addEmptyFigure() {
        Figure fig; fig.name = QString("شخصية %1").arg(figs().size()+1);
        figs().append(fig);
        canvas->selectedFig = &figs().last();
        syncToCanvas(); refreshFigureList();
    }

    void addHumanFigure() {
        Figure fig; fig.name = QString("إنسان %1").arg(figs().size()+1);
        auto& n = fig.nodes;
        n.append(Node(0,-120,14,QColor("#378ADD"))); n.last().id="head";
        n.append(Node(0, -90, 7,QColor("#5DCAA5"))); n.last().id="neck";
        n.append(Node(0, -20, 8,QColor("#5DCAA5"))); n.last().id="body";
        n.append(Node(-30,-75, 7,QColor("#888780"))); n.last().id="ls";
        n.append(Node( 30,-75, 7,QColor("#888780"))); n.last().id="rs";
        n.append(Node(-55,-40, 6,QColor("#888780"))); n.last().id="le";
        n.append(Node( 55,-40, 6,QColor("#888780"))); n.last().id="re";
        n.append(Node(-60,-10, 5,QColor("#888780"))); n.last().id="lh";
        n.append(Node( 60,-10, 5,QColor("#888780"))); n.last().id="rh";
        n.append(Node(0,  20,  8,QColor("#5DCAA5"))); n.last().id="hip";
        n.append(Node(-20, 70, 6,QColor("#888780"))); n.last().id="lk";
        n.append(Node( 20, 70, 6,QColor("#888780"))); n.last().id="rk";
        n.append(Node(-25,120, 6,QColor("#444441"))); n.last().id="lf";
        n.append(Node( 25,120, 6,QColor("#444441"))); n.last().id="rf";
        auto addB=[&](const QString& a,const QString& b,int w,const QString& c){
            Bone bn; bn.from=a;bn.to=b;bn.width=w;bn.color=QColor(c);bn.rounded=true; fig.bones.append(bn);};
        addB("head","neck",4,"#888780"); addB("neck","body",6,"#888780");
        addB("neck","ls",5,"#888780");   addB("neck","rs",5,"#888780");
        addB("ls","le",4,"#B4B2A9");     addB("rs","re",4,"#B4B2A9");
        addB("le","lh",3,"#B4B2A9");     addB("re","rh",3,"#B4B2A9");
        addB("body","hip",6,"#888780");  addB("hip","lk",5,"#B4B2A9"); addB("hip","rk",5,"#B4B2A9");
        addB("lk","lf",4,"#B4B2A9");     addB("rk","rf",4,"#B4B2A9");
        figs().append(fig);
        canvas->selectedFig = &figs().last();
        syncToCanvas(); refreshFigureList();
        statusBar()->showMessage("شخصية إنسان مضافة — اسحب المفاصل لتشكيلها");
    }

    void addStickFigure() {
        Figure fig; fig.name = QString("عصا %1").arg(figs().size()+1);
        auto& n=fig.nodes;
        n.append(Node(0,-100,16,QColor("#378ADD"))); n.last().id="h";
        n.append(Node(0, -30, 6,QColor("#888780"))); n.last().id="b";
        n.append(Node(-40,10, 5,QColor("#888780"))); n.last().id="larm";
        n.append(Node( 40,10, 5,QColor("#888780"))); n.last().id="rarm";
        n.append(Node(-20,90, 5,QColor("#444441"))); n.last().id="lleg";
        n.append(Node( 20,90, 5,QColor("#444441"))); n.last().id="rleg";
        auto addB=[&](const QString& a,const QString& b,int w){
            Bone bn; bn.from=a;bn.to=b;bn.width=w;bn.color=QColor("#444441");bn.rounded=false; fig.bones.append(bn);};
        addB("h","b",3); addB("b","larm",3); addB("b","rarm",3); addB("b","lleg",3); addB("b","rleg",3);
        figs().append(fig);
        canvas->selectedFig = &figs().last();
        syncToCanvas(); refreshFigureList();
    }

    void deleteFigure() {
        int row = figureList->currentRow();
        if (row<0 || row>=figs().size()) return;
        pushUndo();
        figs().removeAt(row);
        canvas->selectedFig=nullptr; canvas->selectedNode=nullptr;
        syncToCanvas(); refreshFigureList();
    }

    void renameFigure() {
        int row=figureList->currentRow();
        if (row<0||row>=figs().size()) return;
        bool ok; QString name=QInputDialog::getText(this,"إعادة تسمية","الاسم الجديد:",QLineEdit::Normal,figs()[row].name,&ok);
        if (ok&&!name.isEmpty()) { figs()[row].name=name; refreshFigureList(); }
    }

    void toggleFigureVisible() {
        int row=figureList->currentRow();
        if (row<0||row>=figs().size()) return;
        figs()[row].visible=!figs()[row].visible;
        canvas->update(); refreshFigureList();
    }

    // ─── Frames ───────────────────────────────────────────────────
    void initFrames() {
        frames.clear();
        Frame f; f.duration=100; f.label="1";
        frames.append(f); currentFrame=0;
    }

    void addFrame() {
        Frame f = cur(); f.label=QString::number(frames.size()+1);
        frames.insert(currentFrame+1, f);
        loadFrame(currentFrame+1);
    }

    void dupFrame() {
        Frame f = cur(); f.label=QString::number(frames.size()+1);
        frames.insert(currentFrame+1, f);
        loadFrame(currentFrame+1);
    }

    void delFrame() {
        if (frames.size()<=1){ statusBar()->showMessage("لا يمكن حذف الإطار الأخير"); return; }
        frames.removeAt(currentFrame);
        loadFrame(qMin(currentFrame,frames.size()-1));
    }

    void loadFrame(int i) {
        if (i<0||i>=frames.size()) return;
        currentFrame=i;
        syncFromFrame();
    }

    void syncFromFrame() {
        canvas->figures = &cur().figures;
        canvas->selectedNode=nullptr;
        canvas->selectedBone=nullptr;
        canvas->update();
        timeline->rebuild(frames,currentFrame,canvas->bgColor);
        refreshFigureList();
        frameDurSpin->blockSignals(true);
        frameDurSpin->setValue(cur().duration);
        frameDurSpin->blockSignals(false);
        frameLabelEdit->blockSignals(true);
        frameLabelEdit->setText(cur().label);
        frameLabelEdit->blockSignals(false);
    }

    void syncToCanvas() {
        canvas->figures = &cur().figures;
        canvas->update();
        timeline->rebuild(frames,currentFrame,canvas->bgColor);
    }

    // ─── Selection ────────────────────────────────────────────────
    void onFiguresChanged() {
        timeline->rebuild(frames,currentFrame,canvas->bgColor);
    }

    void onNodeSelected(Node* n, Figure*) {
        propX->blockSignals(true); propX->setValue(n->x); propX->blockSignals(false);
        propY->blockSignals(true); propY->setValue(n->y); propY->blockSignals(false);
        propSz->blockSignals(true);propSz->setValue(n->size);propSz->blockSignals(false);
        propCol->setColor(n->color);
        propVis->blockSignals(true);propVis->setChecked(n->visible);propVis->blockSignals(false);
    }

    void onSelectionCleared() {}

    void applyNodeProps() {
        Node* n = canvas->selectedNode;
        if (!n) return;
        n->x       = propX->value();
        n->y       = propY->value();
        n->size    = propSz->value();
        n->color   = propCol->color();
        n->visible = propVis->isChecked();
        canvas->update();
        timeline->rebuild(frames,currentFrame,canvas->bgColor);
    }

    void onFigureListSelect(int row) {
        if (row>=0 && row<figs().size()) {
            canvas->selectedFig  = &figs()[row];
            canvas->selectedNode = nullptr;
        }
    }

    void refreshFigureList() {
        figureList->blockSignals(true);
        figureList->clear();
        for (auto& fig : figs()) {
            QString name = (fig.visible?"👁 ":"🚫 ") + fig.name +
                           QString(" (%1م %2ع)").arg(fig.nodes.size()).arg(fig.bones.size());
            figureList->addItem(name);
        }
        figureList->blockSignals(false);
    }

    // ─── Playback ─────────────────────────────────────────────────
    void togglePlay() {
        playing=!playing;
        if (playing) {
            playAct->setText("⏸ إيقاف");
            playAct->setChecked(true);
            int ms=qMax(1,1000/fps);
            playTimer->start(ms);
            statusBar()->showMessage("تشغيل... Space للإيقاف");
        } else {
            playAct->setText("▶ تشغيل");
            playAct->setChecked(false);
            playTimer->stop();
            statusBar()->showMessage("موقوف");
        }
    }

    void stopAnim() {
        playing=false;
        playTimer->stop();
        playAct->setText("▶ تشغيل");
        playAct->setChecked(false);
        loadFrame(0);
        statusBar()->showMessage("توقف — عودة للإطار الأول");
    }

    void nextPlayFrame() {
        int next=currentFrame+playDir;
        if (loopMode=="pingpong") {
            if (next>=frames.size()||next<0) { playDir=-playDir; next=currentFrame+playDir; }
        } else if (loopMode=="once") {
            if (next>=frames.size()) { stopAnim(); return; }
        } else {
            next=((next%frames.size())+frames.size())%frames.size();
        }
        // fast load (no full rebuild)
        currentFrame=next;
        canvas->figures=&cur().figures;
        canvas->update();
        timeline->setCurrentFrame(next);
    }

    // ─── Tools ────────────────────────────────────────────────────
    void setTool(Tool t) {
        canvas->tool=t;
        canvas->boneFromNode=nullptr;
        QString tips[]={
            "تحديد: انقر لتحديد المفصل، اسحب لتحريكه",
            "مفصل: انقر على Canvas لإضافة نقطة جديدة",
            "عظمة: انقر مفصل أول ثم مفصل ثاني لرسم عظمة بينهما"
        };
        statusBar()->showMessage(tips[(int)t]);
    }

    void deleteSelected() {
        if (canvas->selectedNode && canvas->selectedFig) {
            pushUndo();
            QString nid=canvas->selectedNode->id;
            auto& fig=*canvas->selectedFig;
            fig.nodes.removeIf([&](const Node& n){ return n.id==nid; });
            fig.bones.removeIf([&](const Bone& b){ return b.from==nid||b.to==nid; });
            canvas->selectedNode=nullptr;
            syncToCanvas();
        }
    }

    void undo() {
        if (undoStack.isEmpty()) return;
        redoStack.append(frames);
        frames=undoStack.takeLast();
        currentFrame=qMin(currentFrame,frames.size()-1);
        syncFromFrame();
    }

    void redo() {
        if (redoStack.isEmpty()) return;
        undoStack.append(frames);
        frames=redoStack.takeLast();
        currentFrame=qMin(currentFrame,frames.size()-1);
        syncFromFrame();
    }

    // ─── Code ─────────────────────────────────────────────────────
    void toggleCode() {
        if (codePanel->isVisible()) {
            codePanel->hide();
        } else {
            codePanel->setParent(centralWidget());
            codePanel->setGeometry(0,0,centralWidget()->width(),centralWidget()->height()-timeline->height());
            codePanel->show();
            codePanel->raise();
        }
    }

    void runCode() {
        CodeInterpreter interp;
        bool ok = interp.run(codePanel->code());
        if (!interp.frames.isEmpty()) {
            pushUndo();
            frames=interp.frames;
            currentFrame=0;
            syncFromFrame();
            codePanel->setStatus(QString("✓ نجح — %1 شخصية, %2 إطار").arg(interp.figures.size()).arg(frames.size()), true);
            codePanel->hide();
            statusBar()->showMessage(QString("تم تنفيذ الكود: %1 إطار").arg(frames.size()));
        }
        if (!interp.errors.isEmpty()) {
            QString err;
            for (auto& e : interp.errors) err+=QString("سطر %1: %2\n").arg(e.line).arg(e.msg);
            codePanel->setStatus("خطأ: "+err.trimmed(), false);
        }
    }

    // ─── Project ──────────────────────────────────────────────────
    void newProject() {
        if (QMessageBox::question(this,"مشروع جديد","هل تريد مسح المشروع الحالي؟")==QMessageBox::No) return;
        pushUndo();
        frames.clear();
        Frame f; f.duration=100; f.label="1";
        frames.append(f); currentFrame=0;
        addHumanFigure();
        syncFromFrame();
    }

    void saveProject() { Exporter::exportJSON(frames,canvas->bgColor,fps,loopMode); }

    void openProject() {
        QColor bg; QVector<Frame> newFrames;
        Exporter::importJSON(newFrames, bg, this);
        if (!newFrames.isEmpty()) {
            pushUndo(); frames=newFrames; canvas->bgColor=bg;
            bgColorBtn->setColor(bg);
            currentFrame=0; syncFromFrame();
        }
    }

    void showExportMenu() {
        QMenu m; m.setTitle("تصدير");
        m.addAction("PNG سلسلة إطارات", this,[this]{Exporter::exportPNGSequence(frames,canvas->bgColor);});
        m.addAction("Sprite Sheet",      this,[this]{Exporter::exportSpriteSheet(frames,canvas->bgColor);});
        m.addAction("HTML متحرك",        this,[this]{Exporter::exportHTML(frames,canvas->bgColor);});
        m.addAction("GDScript Godot",    this,[this]{Exporter::exportGDScript(frames);});
        m.addAction("CSS Animation",     this,[this]{Exporter::exportCSSAnimation(frames);});
        m.addAction("JSON (مشروع)",      this,&MainWindow::saveProject);
        m.exec(QCursor::pos());
    }

    // Fix: canvas needs to emit resized
};
