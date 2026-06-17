#include "mainwindow.h"
#include "codeeditordialog.h"
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QActionGroup>
#include <QShortcut>
#include <QImageWriter>
#include <QProgressDialog>
#include <QInputDialog>
#include <QStandardPaths>

// ─── Helpers ─────────────────────────────────────────────────────────────────
static QPushButton* colorButton(const QColor &c) {
    auto *btn = new QPushButton;
    btn->setFixedSize(28,22);
    btn->setStyleSheet(QString("background:%1;border:1px solid #555;border-radius:4px;").arg(c.name()));
    return btn;
}
static void setColorButton(QPushButton *btn, const QColor &c) {
    btn->setStyleSheet(QString("background:%1;border:1px solid #555;border-radius:4px;").arg(c.name()));
}
static QWidget* makeRow(const QString &lbl, QWidget *w, int lw=80) {
    auto *row = new QWidget; auto *hl = new QHBoxLayout(row);
    hl->setContentsMargins(0,0,0,0); hl->setSpacing(4);
    auto *l = new QLabel(lbl); l->setFixedWidth(lw); l->setStyleSheet("font-size:11px;color:#9090aa;");
    hl->addWidget(l); hl->addWidget(w,1); return row;
}

// ─── MainWindow ──────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Pivot Animator Pro");
    setMinimumSize(1100, 700);
    resize(1280, 800);

    // Canvas (central)
    m_canvas = new CanvasWidget(this);
    setCentralWidget(m_canvas);

    setupMenus();
    setupToolbar();
    setupDocks();
    setupStatusBar();

    // Timeline at bottom
    m_timeline = new TimelineWidget(this);
    auto *tlDock = new QDockWidget("الجدول الزمني", this);
    tlDock->setWidget(m_timeline);
    tlDock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea, tlDock);

    // Signals
    connect(m_canvas, &CanvasWidget::frameChanged,    this, &MainWindow::onFrameChanged);
    connect(m_canvas, &CanvasWidget::selectionChanged,this, &MainWindow::onSelectionChanged);
    connect(m_canvas, &CanvasWidget::projectModified, this, &MainWindow::onProjectModified);
    connect(m_timeline, &TimelineWidget::frameSelected, this, &MainWindow::onFrameSelected);

    // Keyboard shortcuts
    new QShortcut(QKeySequence("Ctrl+Z"), this, [this](){ /* undo placeholder */ });
    new QShortcut(QKeySequence("F5"), this, [this](){ togglePlay(); });
    new QShortcut(QKeySequence("S"), this, m_canvas, [this](){ m_canvas->setTool(Tool::Select); });
    new QShortcut(QKeySequence("N"), this, m_canvas, [this](){ m_canvas->setTool(Tool::AddNode); });
    new QShortcut(QKeySequence("B"), this, m_canvas, [this](){ m_canvas->setTool(Tool::AddBone); });

    initNewProject();
}

void MainWindow::initNewProject()
{
    m_project = AnimProject();
    m_project.name = "مشروع جديد";
    Frame f; f.duration=100; f.label="1";
    m_project.frames.append(f);
    m_canvas->setProject(&m_project);
    m_canvas->addHumanFigure();
    m_timeline->setProject(&m_project);
    m_timeline->refresh(0);
    refreshAll();
    setModified(false);
}

void MainWindow::setupMenus()
{
    auto *mb = menuBar();

    // File
    auto *fileMenu = mb->addMenu("ملف");
    fileMenu->addAction("جديد",   this, &MainWindow::newProject,   QKeySequence::New);
    fileMenu->addAction("فتح...", this, &MainWindow::openProject,  QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("حفظ",    this, &MainWindow::saveProject,  QKeySequence::Save);
    fileMenu->addAction("حفظ باسم...", this, &MainWindow::saveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("تصدير...", this, &MainWindow::exportAnimation, QKeySequence("Ctrl+E"));
    fileMenu->addSeparator();
    fileMenu->addAction("خروج", this, &QWidget::close, QKeySequence::Quit);

    // Edit
    auto *editMenu = mb->addMenu("تحرير");
    editMenu->addAction("إضافة إطار",  this, &MainWindow::addFrame,    QKeySequence("Ctrl+J"));
    editMenu->addAction("نسخ إطار",    this, &MainWindow::dupFrame,    QKeySequence("Ctrl+D"));
    editMenu->addAction("حذف إطار",    this, &MainWindow::deleteFrame, QKeySequence::Delete);

    // Figure
    auto *figMenu = mb->addMenu("شخصية");
    figMenu->addAction("إنسان كامل",      this, &MainWindow::addHuman);
    figMenu->addAction("عصا (Stick)",     this, &MainWindow::addStick);
    figMenu->addAction("شخصية فارغة",    this, &MainWindow::addEmpty);
    figMenu->addSeparator();
    figMenu->addAction("حذف الشخصية المحددة", this, &MainWindow::deleteFigure);

    // Anim
    auto *animMenu = mb->addMenu("أنيميشن");
    m_actPlay = animMenu->addAction("تشغيل / إيقاف", this, &MainWindow::togglePlay, QKeySequence("F5"));
    m_actStop = animMenu->addAction("إيقاف وعودة",   this, &MainWindow::stopPlayback, QKeySequence("F6"));

    // Script
    auto *scriptMenu = mb->addMenu("كود");
    scriptMenu->addAction("محرر الكود", this, &MainWindow::showCodeEditor, QKeySequence("F9"));
}

void MainWindow::setupToolbar()
{
    auto *tb = addToolBar("أدوات");
    tb->setIconSize(QSize(16,16));
    tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tb->setMovable(false);

    // Tool group
    auto *tg = new QActionGroup(this);
    tg->setExclusive(true);
    connect(tg, &QActionGroup::triggered, this, &MainWindow::onToolChanged);

    auto makeToolAct = [&](const QString &txt, Tool t, const QString &shortcut) -> QAction* {
        auto *a = tg->addAction(txt); a->setCheckable(true); a->setData((int)t);
        a->setShortcut(QKeySequence(shortcut)); tb->addAction(a); return a;
    };
    makeToolAct("⬡ تحديد",  Tool::Select,  "S")->setChecked(true);
    makeToolAct("◉ مفصل",   Tool::AddNode, "N");
    makeToolAct("╱ عظمة",   Tool::AddBone, "B");
    makeToolAct("✥ تحريك",  Tool::Pan,     "M");

    tb->addSeparator();
    tb->addAction("👤 إنسان", this, &MainWindow::addHuman);
    tb->addAction("🦯 عصا",   this, &MainWindow::addStick);
    tb->addAction("＋ شخصية", this, &MainWindow::addEmpty);

    tb->addSeparator();
    tb->addAction("⊞ إطار",  this, &MainWindow::addFrame);
    tb->addAction("⎘ نسخ",   this, &MainWindow::dupFrame);
    tb->addAction("✕ حذف",   this, &MainWindow::deleteFrame);

    tb->addSeparator();
    m_actPlay = tb->addAction("▶ تشغيل", this, &MainWindow::togglePlay);
    m_actStop = tb->addAction("■ إيقاف", this, &MainWindow::stopPlayback);

    tb->addSeparator();
    tb->addAction("</> كود",     this, &MainWindow::showCodeEditor);
    tb->addAction("⇩ تصدير",    this, &MainWindow::exportAnimation);
}

void MainWindow::setupDocks()
{
    // ── Left dock: Figures + Defaults ────────────────────────────────────────
    m_figDock = new QDockWidget("الشخصيات والأدوات", this);
    m_figDock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto *figWidget = new QWidget;
    auto *figVL = new QVBoxLayout(figWidget);
    figVL->setContentsMargins(4,4,4,4); figVL->setSpacing(6);

    // Figure list
    auto *figGrp = new QGroupBox("الشخصيات");
    auto *figGL = new QVBoxLayout(figGrp);
    m_figList = new QListWidget;
    m_figList->setMaximumHeight(130);
    figGL->addWidget(m_figList);
    auto *figBtnRow = new QHBoxLayout;
    auto *addHBtn = new QPushButton("إنسان"); addHBtn->setFixedHeight(24);
    auto *addSBtn = new QPushButton("عصا");   addSBtn->setFixedHeight(24);
    auto *delFBtn = new QPushButton("حذف");   delFBtn->setFixedHeight(24);
    figBtnRow->addWidget(addHBtn); figBtnRow->addWidget(addSBtn); figBtnRow->addWidget(delFBtn);
    figGL->addLayout(figBtnRow);
    connect(addHBtn, &QPushButton::clicked, this, &MainWindow::addHuman);
    connect(addSBtn, &QPushButton::clicked, this, &MainWindow::addStick);
    connect(delFBtn, &QPushButton::clicked, this, &MainWindow::deleteFigure);
    connect(m_figList, &QListWidget::itemClicked, this, &MainWindow::onFigureListItemClicked);
    figVL->addWidget(figGrp);

    // Node defaults
    auto *ndGrp = new QGroupBox("إعدادات المفصل");
    auto *ndVL = new QVBoxLayout(ndGrp);
    m_nodeColorBtn = colorButton(QColor(55,138,221));
    m_nodeSizeSlider = new QSlider(Qt::Horizontal); m_nodeSizeSlider->setRange(3,30); m_nodeSizeSlider->setValue(8);
    ndVL->addWidget(makeRow("اللون:", m_nodeColorBtn));
    ndVL->addWidget(makeRow("الحجم:", m_nodeSizeSlider));
    figVL->addWidget(ndGrp);
    connect(m_nodeColorBtn,  &QPushButton::clicked, this, &MainWindow::pickNodeColor);
    connect(m_nodeSizeSlider,&QSlider::valueChanged, this,[this](int v){m_canvas->defaultNodeSize=v;});

    // Bone defaults
    auto *bnGrp = new QGroupBox("إعدادات العظمة");
    auto *bnVL = new QVBoxLayout(bnGrp);
    m_boneColorBtn   = colorButton(QColor(136,135,128));
    m_boneWidthSlider = new QSlider(Qt::Horizontal); m_boneWidthSlider->setRange(1,20); m_boneWidthSlider->setValue(4);
    m_boneRoundSlider = new QSlider(Qt::Horizontal); m_boneRoundSlider->setRange(0,20); m_boneRoundSlider->setValue(0);
    bnVL->addWidget(makeRow("اللون:", m_boneColorBtn));
    bnVL->addWidget(makeRow("السمك:", m_boneWidthSlider));
    bnVL->addWidget(makeRow("تدوير:", m_boneRoundSlider));
    figVL->addWidget(bnGrp);
    figVL->addStretch();
    connect(m_boneColorBtn,   &QPushButton::clicked, this, &MainWindow::pickBoneColor);
    connect(m_boneWidthSlider,&QSlider::valueChanged,this,[this](int v){m_canvas->defaultBoneWidth=v;});
    connect(m_boneRoundSlider,&QSlider::valueChanged,this,[this](int v){m_canvas->defaultBoneRound=v;});

    m_figDock->setWidget(figWidget);
    addDockWidget(Qt::LeftDockWidgetArea, m_figDock);

    // ── Right dock: Properties ─────────────────────────────────────────────
    m_propDock = new QDockWidget("الخصائص", this);
    m_propDock->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto *propWidget = new QWidget;
    auto *propVL = new QVBoxLayout(propWidget);
    propVL->setContentsMargins(4,4,4,4); propVL->setSpacing(6);

    // Selected node props
    auto *nodeGrp = new QGroupBox("المفصل المحدد");
    auto *nodeGL = new QVBoxLayout(nodeGrp);
    m_propX    = new QDoubleSpinBox; m_propX->setRange(-5000,5000); m_propX->setDecimals(1);
    m_propY    = new QDoubleSpinBox; m_propY->setRange(-5000,5000); m_propY->setDecimals(1);
    m_propSize = new QDoubleSpinBox; m_propSize->setRange(1,100);
    m_propNodeColor = colorButton(Qt::white);
    m_propVisible   = new QCheckBox("مرئي"); m_propVisible->setChecked(true);
    nodeGL->addWidget(makeRow("X:", m_propX));
    nodeGL->addWidget(makeRow("Y:", m_propY));
    nodeGL->addWidget(makeRow("الحجم:", m_propSize));
    nodeGL->addWidget(makeRow("اللون:", m_propNodeColor));
    nodeGL->addWidget(m_propVisible);
    propVL->addWidget(nodeGrp);

    connect(m_propX,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,[this](double v){
        if(m_canvas->selectedNode){m_canvas->selectedNode->x=v;m_canvas->saveCurrentFrame();m_canvas->update();}});
    connect(m_propY,   QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,[this](double v){
        if(m_canvas->selectedNode){m_canvas->selectedNode->y=v;m_canvas->saveCurrentFrame();m_canvas->update();}});
    connect(m_propSize,QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,[this](double v){
        if(m_canvas->selectedNode){m_canvas->selectedNode->size=v;m_canvas->saveCurrentFrame();m_canvas->update();}});
    connect(m_propNodeColor,&QPushButton::clicked,this,[this](){
        if(!m_canvas->selectedNode) return;
        QColor c=QColorDialog::getColor(m_canvas->selectedNode->color,this,"لون المفصل");
        if(c.isValid()){m_canvas->selectedNode->color=c;setColorButton(m_propNodeColor,c);m_canvas->saveCurrentFrame();m_canvas->update();}});
    connect(m_propVisible,&QCheckBox::toggled,this,[this](bool v){
        if(m_canvas->selectedNode){m_canvas->selectedNode->visible=v;m_canvas->saveCurrentFrame();m_canvas->update();}});

    // Frame props
    auto *frGrp = new QGroupBox("الإطار الحالي");
    auto *frGL = new QVBoxLayout(frGrp);
    m_frameDurSpin  = new QSpinBox; m_frameDurSpin->setRange(16,10000); m_frameDurSpin->setSuffix(" ms"); m_frameDurSpin->setValue(100);
    m_frameLabelEdit= new QLineEdit; m_frameLabelEdit->setPlaceholderText("تسمية الإطار...");
    frGL->addWidget(makeRow("المدة:", m_frameDurSpin));
    frGL->addWidget(makeRow("الاسم:", m_frameLabelEdit));
    propVL->addWidget(frGrp);
    connect(m_frameDurSpin,QOverload<int>::of(&QSpinBox::valueChanged),this,[this](int v){
        if(!m_project.frames.isEmpty()) m_project.frames[m_canvas->currentFrame()].duration=v;});
    connect(m_frameLabelEdit,&QLineEdit::textChanged,this,[this](const QString &t){
        if(!m_project.frames.isEmpty()){m_project.frames[m_canvas->currentFrame()].label=t;m_timeline->refresh(m_canvas->currentFrame());}});

    // Anim settings
    auto *animGrp = new QGroupBox("إعدادات الأنيميشن");
    auto *animGL = new QVBoxLayout(animGrp);
    m_fpsSpin   = new QSpinBox; m_fpsSpin->setRange(1,120); m_fpsSpin->setValue(12); m_fpsSpin->setSuffix(" fps");
    m_loopCombo = new QComboBox;
    m_loopCombo->addItems({"loop — دائم","once — مرة","pingpong — ذهاب وإياب"});
    animGL->addWidget(makeRow("السرعة:", m_fpsSpin));
    animGL->addWidget(makeRow("التكرار:", m_loopCombo));
    propVL->addWidget(animGrp);
    connect(m_fpsSpin,QOverload<int>::of(&QSpinBox::valueChanged),this,[this](int v){m_project.fps=v;});
    connect(m_loopCombo,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int i){
        static const char* modes[]={"loop","once","pingpong"};
        m_project.loopMode=modes[i];});

    // Canvas settings
    auto *canvGrp = new QGroupBox("إعدادات الكانفس");
    auto *canvGL = new QVBoxLayout(canvGrp);
    m_bgColorBtn = colorButton(QColor(30,30,40));
    m_gridCheck  = new QCheckBox("عرض الشبكة");   m_gridCheck->setChecked(true);
    m_rulerCheck = new QCheckBox("عرض المساطر");  m_rulerCheck->setChecked(true);
    canvGL->addWidget(makeRow("الخلفية:", m_bgColorBtn));
    canvGL->addWidget(m_gridCheck);
    canvGL->addWidget(m_rulerCheck);
    propVL->addWidget(canvGrp);
    propVL->addStretch();

    connect(m_bgColorBtn,&QPushButton::clicked,this,&MainWindow::pickBgColor);
    connect(m_gridCheck, &QCheckBox::toggled,this,[this](bool v){m_canvas->showGrid=v;m_canvas->update();});
    connect(m_rulerCheck,&QCheckBox::toggled,this,[this](bool v){m_canvas->showRulers=v;m_canvas->update();});

    m_propDock->setWidget(propWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_propDock);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("جاهز | S=تحديد  N=مفصل  B=عظمة  F5=تشغيل  Del=حذف مفصل  عجلة=تكبير  Alt+سحب=تحريك");
}

// ─── File operations ──────────────────────────────────────────────────────────
void MainWindow::newProject()
{
    if (m_modified && QMessageBox::question(this,"تأكيد","المشروع غير محفوظ. تجاهل؟")!=QMessageBox::Yes) return;
    initNewProject();
}

void MainWindow::openProject()
{
    QString path = QFileDialog::getOpenFileName(this,"فتح مشروع","","Pivot Projects (*.pvt *.json)");
    if (path.isEmpty()) return;
    QFile f(path); if(!f.open(QIODevice::ReadOnly)){QMessageBox::warning(this,"خطأ","تعذر فتح الملف");return;}
    auto doc = QJsonDocument::fromJson(f.readAll());
    m_project = AnimProject::fromJson(doc.object());
    m_currentFile = path;
    m_canvas->setProject(&m_project);
    m_timeline->setProject(&m_project);
    m_timeline->refresh(0);
    refreshAll();
    setModified(false);
    statusBar()->showMessage("تم فتح: "+path,4000);
}

void MainWindow::saveProject()
{
    if (m_currentFile.isEmpty()) { saveProjectAs(); return; }
    m_canvas->saveCurrentFrame();
    QFile f(m_currentFile); if(!f.open(QIODevice::WriteOnly)){QMessageBox::warning(this,"خطأ","تعذر الحفظ");return;}
    f.write(QJsonDocument(m_project.toJson()).toJson());
    setModified(false);
    statusBar()->showMessage("تم الحفظ: "+m_currentFile,3000);
}

void MainWindow::saveProjectAs()
{
    QString path = QFileDialog::getSaveFileName(this,"حفظ المشروع","","Pivot Projects (*.pvt)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".pvt")) path+=".pvt";
    m_currentFile = path;
    saveProject();
}

// ─── Figure operations ────────────────────────────────────────────────────────
void MainWindow::addHuman()  { m_canvas->addHumanFigure(); updateFigureList(); m_timeline->refresh(m_canvas->currentFrame()); }
void MainWindow::addStick()  { m_canvas->addStickFigure(); updateFigureList(); m_timeline->refresh(m_canvas->currentFrame()); }
void MainWindow::addEmpty()  { m_canvas->addEmptyFigure(); updateFigureList(); m_timeline->refresh(m_canvas->currentFrame()); }

void MainWindow::deleteFigure()
{
    if (!m_canvas->selectedFigure) return;
    QString id = m_canvas->selectedFigure->id;
    // Remove from all frames
    for (auto &fr : m_project.frames)
        fr.figures.removeIf([&](const Figure &f){ return f.id==id; });
    m_canvas->setCurrentFrame(m_canvas->currentFrame());
    m_canvas->selectedFigure = nullptr;
    updateFigureList(); m_timeline->refresh(m_canvas->currentFrame());
    setModified(true);
}

// ─── Frame operations ─────────────────────────────────────────────────────────
void MainWindow::addFrame()
{
    m_canvas->saveCurrentFrame();
    int ci = m_canvas->currentFrame();
    Frame f = m_project.frames[ci];
    f.label = QString::number(m_project.frames.size()+1);
    m_project.frames.insert(ci+1, f);
    m_canvas->setCurrentFrame(ci+1);
    m_timeline->refresh(ci+1);
    setModified(true);
}

void MainWindow::dupFrame()
{
    m_canvas->saveCurrentFrame();
    int ci = m_canvas->currentFrame();
    Frame f = m_project.frames[ci];
    f.label = QString::number(m_project.frames.size()+1);
    m_project.frames.insert(ci+1, f);
    m_canvas->setCurrentFrame(ci+1);
    m_timeline->refresh(ci+1);
    setModified(true);
}

void MainWindow::deleteFrame()
{
    if (m_project.frames.size()<=1){statusBar()->showMessage("لا يمكن حذف الإطار الأخير",2000);return;}
    int ci = m_canvas->currentFrame();
    m_project.frames.removeAt(ci);
    int ni = qMin(ci, m_project.frames.size()-1);
    m_canvas->setCurrentFrame(ni);
    m_timeline->refresh(ni);
    setModified(true);
}

void MainWindow::onFrameChanged(int idx)
{
    m_timeline->refresh(idx);
    if (!m_project.frames.isEmpty() && idx < m_project.frames.size()) {
        const Frame &fr = m_project.frames[idx];
        m_frameDurSpin->blockSignals(true);
        m_frameDurSpin->setValue(fr.duration);
        m_frameDurSpin->blockSignals(false);
        m_frameLabelEdit->blockSignals(true);
        m_frameLabelEdit->setText(fr.label);
        m_frameLabelEdit->blockSignals(false);
    }
    statusBar()->showMessage(QString("الإطار %1 / %2").arg(idx+1).arg(m_project.frames.size()),2000);
}

void MainWindow::onFrameSelected(int idx) { m_canvas->setCurrentFrame(idx); }

// ─── Playback ─────────────────────────────────────────────────────────────────
void MainWindow::togglePlay()
{
    if (m_canvas->isPlaying()) {
        m_canvas->pause();
        m_actPlay->setText("▶ تشغيل");
    } else {
        m_canvas->play();
        m_actPlay->setText("⏸ إيقاف");
    }
}

void MainWindow::stopPlayback()
{
    m_canvas->stop();
    m_actPlay->setText("▶ تشغيل");
    m_timeline->refresh(0);
}

// ─── Selection ────────────────────────────────────────────────────────────────
void MainWindow::onSelectionChanged()
{
    Node *n = m_canvas->selectedNode;
    if (n) {
        m_propX->blockSignals(true);    m_propX->setValue(n->x);     m_propX->blockSignals(false);
        m_propY->blockSignals(true);    m_propY->setValue(n->y);     m_propY->blockSignals(false);
        m_propSize->blockSignals(true); m_propSize->setValue(n->size);m_propSize->blockSignals(false);
        setColorButton(m_propNodeColor, n->color);
        m_propVisible->blockSignals(true); m_propVisible->setChecked(n->visible); m_propVisible->blockSignals(false);
    }
}

void MainWindow::onToolChanged(QAction *a)
{
    m_canvas->setTool((Tool)a->data().toInt());
}

void MainWindow::onProjectModified()
{
    setModified(true);
    updateFigureList();
}

// ─── Code editor ─────────────────────────────────────────────────────────────
void MainWindow::showCodeEditor()
{
    auto *dlg = new CodeEditorDialog(this);
    dlg->setProject(&m_project);
    connect(dlg, &CodeEditorDialog::codeApplied, this, [this](){
        m_canvas->setProject(&m_project);
        m_timeline->setProject(&m_project);
        m_timeline->refresh(0);
        updateFigureList();
        setModified(true);
        statusBar()->showMessage("تم تطبيق الكود بنجاح",3000);
    });
    dlg->exec();
}

// ─── Export ──────────────────────────────────────────────────────────────────
void MainWindow::exportAnimation()
{
    QStringList types = {
        "سلسلة PNG (png-seq)",
        "SVG متحرك (svg)",
        "HTML قابل للتضمين (html)",
        "JSON مشروع كامل (json)",
        "كود GDScript لـ Godot (gd)",
        "كود CSS Animation (css)"
    };
    bool ok;
    QString choice = QInputDialog::getItem(this,"تصدير الأنيميشن","اختر نوع التصدير:", types, 0, false, &ok);
    if (!ok) return;

    m_canvas->saveCurrentFrame();

    if (choice.contains("png-seq")) {
        QString dir = QFileDialog::getExistingDirectory(this,"اختر مجلد الحفظ");
        if (dir.isEmpty()) return;
        QProgressDialog prog("جاري التصدير...","إلغاء",0,m_project.frames.size(),this);
        prog.setWindowModality(Qt::WindowModal);
        for (int i=0; i<m_project.frames.size(); i++) {
            prog.setValue(i);
            if(prog.wasCanceled()) break;
            QImage img = m_canvas->renderFrameToImage(i,600,600);
            img.save(dir+QString("/frame_%1.png").arg(i+1,4,10,QLatin1Char('0')));
        }
        prog.setValue(m_project.frames.size());
        statusBar()->showMessage("تم تصدير PNG إلى: "+dir,4000);
    }
    else if (choice.contains("json")) {
        QString path = QFileDialog::getSaveFileName(this,"حفظ JSON","","JSON (*.json)");
        if(path.isEmpty()) return;
        QFile f(path); if(!f.open(QIODevice::WriteOnly)) return;
        f.write(QJsonDocument(m_project.toJson()).toJson(QJsonDocument::Indented));
        statusBar()->showMessage("تم التصدير: "+path,4000);
    }
    else if (choice.contains("html")) {
        QString path = QFileDialog::getSaveFileName(this,"حفظ HTML","","HTML (*.html)");
        if(path.isEmpty()) return;
        // Build embedded SVG animation
        QString html = "<!DOCTYPE html><html><head><meta charset='utf-8'><title>"+m_project.name+"</title></head>\n";
        html += "<body style='background:"+m_project.background.name()+";margin:0;display:flex;justify-content:center;align-items:center;min-height:100vh;'>\n";
        html += "<div id='anim' style='position:relative;width:600px;height:600px;'>\n";
        for (int i=0; i<m_project.frames.size(); i++) {
            const Frame &fr = m_project.frames[i];
            html += QString("<svg id='f%1' width='600' height='600' style='position:absolute;top:0;left:0;display:%2;'>\n").arg(i).arg(i==0?"block":"none");
            for (const auto &fig : fr.figures) {
                for (const auto &b : fig.bones) {
                    const Node *n1=nullptr,*n2=nullptr;
                    for(const auto &n:fig.nodes){if(n.id==b.fromId)n1=&n;if(n.id==b.toId)n2=&n;}
                    if(!n1||!n2) continue;
                    html+=QString("<line x1='%1' y1='%2' x2='%3' y2='%4' stroke='%5' stroke-width='%6' stroke-linecap='round'/>\n")
                        .arg(n1->x+300).arg(n1->y+300).arg(n2->x+300).arg(n2->y+300).arg(b.color.name()).arg(b.width);
                }
                for(const auto &n:fig.nodes){
                    html+=QString("<circle cx='%1' cy='%2' r='%3' fill='%4'/>\n")
                        .arg(n.x+300).arg(n.y+300).arg(n.size).arg(n.color.name());
                }
            }
            html+="</svg>\n";
        }
        html+="</div>\n";
        QString durs="["; for(int i=0;i<m_project.frames.size();i++){if(i)durs+=","; durs+=QString::number(m_project.frames[i].duration);}
        html+="]";
        html+="<script>\nconst frames=document.querySelectorAll('#anim svg');\nconst durs="+durs+";\nlet cur=0;\n";
        html+="function next(){frames[cur].style.display='none';cur=(cur+1)%frames.length;frames[cur].style.display='block';setTimeout(next,durs[cur]);}\n";
        html+="setTimeout(next,durs[0]);\n</script>\n</body></html>";
        QFile f(path); if(f.open(QIODevice::WriteOnly)) f.write(html.toUtf8());
        statusBar()->showMessage("تم التصدير HTML: "+path,4000);
    }
    else if (choice.contains("gd")) {
        QString path = QFileDialog::getSaveFileName(this,"حفظ GDScript","","GDScript (*.gd)");
        if(path.isEmpty()) return;
        QString code="# GDScript - Generated by Pivot Animator Pro\n# Project: "+m_project.name+"\n\n";
        code+="func play_animation(node: Node2D) -> void:\n";
        code+="\tvar tween := create_tween()\n";
        code+="\ttween.set_loops()\n\n";
        for(int i=0;i<m_project.frames.size();i++){
            const Frame &fr=m_project.frames[i];
            code+=QString("\t# ── Frame %1 ────────────────\n").arg(i+1);
            for(const auto &fig:fr.figures){
                for(const auto &n:fig.nodes){
                    code+=QString("\t# node '%1' at (%2, %3)\n").arg(n.id).arg((int)n.x).arg((int)n.y);
                }
            }
            code+=QString("\tawait get_tree().create_timer(%1).timeout\n\n").arg(fr.duration/1000.0,0,'f',3);
        }
        QFile f(path); if(f.open(QIODevice::WriteOnly)) f.write(code.toUtf8());
        statusBar()->showMessage("تم التصدير GDScript: "+path,4000);
    }
    else if (choice.contains("css")) {
        QString path = QFileDialog::getSaveFileName(this,"حفظ CSS","","CSS (*.css)");
        if(path.isEmpty()) return;
        double total=0; for(auto &fr:m_project.frames) total+=fr.duration;
        QString css="/* CSS Animation - Generated by Pivot Animator Pro */\n";
        css+="/* Project: "+m_project.name+" */\n\n";
        css+=".pivot-anim { animation: pivot_main "+QString::number(total/1000.0,'f',2)+"s infinite; }\n\n";
        css+="@keyframes pivot_main {\n";
        double acc=0;
        for(int i=0;i<m_project.frames.size();i++){
            double pct=acc/total*100;
            css+=QString("  %1% { /* frame %2 */ }\n").arg(pct,0,'f',1).arg(i+1);
            acc+=m_project.frames[i].duration;
        }
        css+="  100% {}\n}\n";
        QFile f(path); if(f.open(QIODevice::WriteOnly)) f.write(css.toUtf8());
        statusBar()->showMessage("تم التصدير CSS: "+path,4000);
    }
}

// ─── Color pickers ────────────────────────────────────────────────────────────
void MainWindow::pickNodeColor() {
    QColor c=QColorDialog::getColor(m_canvas->defaultNodeColor,this,"لون المفصل الافتراضي");
    if(c.isValid()){m_canvas->defaultNodeColor=c;setColorButton(m_nodeColorBtn,c);}
}
void MainWindow::pickBoneColor() {
    QColor c=QColorDialog::getColor(m_canvas->defaultBoneColor,this,"لون العظمة الافتراضي");
    if(c.isValid()){m_canvas->defaultBoneColor=c;setColorButton(m_boneColorBtn,c);}
}
void MainWindow::pickBgColor() {
    QColor c=QColorDialog::getColor(m_project.background,this,"لون الخلفية");
    if(c.isValid()){m_project.background=c;setColorButton(m_bgColorBtn,c);m_canvas->update();}
}

// ─── Figure list ─────────────────────────────────────────────────────────────
void MainWindow::updateFigureList()
{
    m_figList->clear();
    if (m_project.frames.isEmpty()) return;
    int ci = m_canvas->currentFrame();
    if (ci >= m_project.frames.size()) return;
    for (const auto &fig : m_project.frames[ci].figures) {
        auto *item = new QListWidgetItem(fig.visible ? "◉ "+fig.name : "○ "+fig.name);
        item->setData(Qt::UserRole, fig.id);
        m_figList->addItem(item);
    }
}

void MainWindow::onFigureListItemClicked(QListWidgetItem *item)
{
    QString id = item->data(Qt::UserRole).toString();
    m_canvas->selectedFigure = nullptr;
    m_canvas->selectedNode   = nullptr;
    // Find in live figures
    int ci = m_canvas->currentFrame();
    if (ci < m_project.frames.size()) {
        for (auto &fig : m_project.frames[ci].figures) {
            if (fig.id==id) { /* we'd need canvas to expose its working copy */ break; }
        }
    }
}

void MainWindow::refreshAll()
{
    updateFigureList();
    if (!m_project.frames.isEmpty()) {
        m_fpsSpin->blockSignals(true);   m_fpsSpin->setValue(m_project.fps); m_fpsSpin->blockSignals(false);
        int li=0;
        if(m_project.loopMode=="once") li=1;
        else if(m_project.loopMode=="pingpong") li=2;
        m_loopCombo->blockSignals(true); m_loopCombo->setCurrentIndex(li); m_loopCombo->blockSignals(false);
        setColorButton(m_bgColorBtn, m_project.background);
    }
}

void MainWindow::setModified(bool m)
{
    m_modified = m;
    setWindowTitle(m ? "Pivot Animator Pro *" : "Pivot Animator Pro");
}
