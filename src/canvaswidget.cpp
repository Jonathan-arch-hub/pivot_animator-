#include "canvaswidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QUuid>
#include <QtMath>
#include <QCursor>

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    connect(&m_playTimer, &QTimer::timeout, this, [this](){
        if (!m_proj || m_proj->frames.isEmpty()) return;
        int n = m_proj->frames.size();
        int next = m_curFrame + m_playDir;
        if (m_proj->loopMode == "pingpong") {
            if (next >= n) { m_playDir = -1; next = n-2; }
            else if (next < 0) { m_playDir = 1; next = 1; }
        } else if (m_proj->loopMode == "once") {
            if (next >= n) { pause(); return; }
        } else {
            next = ((next % n) + n) % n;
        }
        m_curFrame = next;
        m_figures  = m_proj->frames[m_curFrame].figures;
        update();
        emit frameChanged(m_curFrame);
    });
}

void CanvasWidget::setProject(AnimProject *p)
{
    m_proj = p;
    m_curFrame = 0;
    if (p && !p->frames.isEmpty())
        m_figures = p->frames[0].figures;
    m_panX = width()/2.0;
    m_panY = height()/2.0;
    selectedFigure = nullptr;
    selectedNode   = nullptr;
    selectedBone   = nullptr;
    update();
}

void CanvasWidget::setCurrentFrame(int i)
{
    if (!m_proj || i < 0 || i >= m_proj->frames.size()) return;
    saveCurrentFrame();
    m_curFrame = i;
    m_figures  = m_proj->frames[i].figures;
    // Fix pointers
    selectedNode   = nullptr;
    selectedBone   = nullptr;
    if (selectedFigure) {
        QString id = selectedFigure->id;
        selectedFigure = nullptr;
        for (auto &f : m_figures) if (f.id == id) { selectedFigure = &f; break; }
    }
    update();
    emit frameChanged(i);
}

void CanvasWidget::saveCurrentFrame()
{
    if (!m_proj || m_curFrame >= m_proj->frames.size()) return;
    m_proj->frames[m_curFrame].figures = m_figures;
}

void CanvasWidget::setTool(Tool t)
{
    m_tool = t;
    m_boneFromNode = nullptr;
    m_boneFromFig  = nullptr;
    switch(t) {
        case Tool::AddNode: setCursor(Qt::CrossCursor); break;
        case Tool::AddBone: setCursor(Qt::CrossCursor); break;
        case Tool::Pan:     setCursor(Qt::OpenHandCursor); break;
        default:            setCursor(Qt::ArrowCursor); break;
    }
    update();
}

void CanvasWidget::play()
{
    if (!m_proj || m_proj->frames.size() < 2) return;
    m_playing = true;
    m_playDir = 1;
    int dur = m_proj->frames[m_curFrame].duration;
    m_playTimer.start(dur > 0 ? dur : (1000 / qMax(1, m_proj->fps)));
}

void CanvasWidget::pause()
{
    m_playing = false;
    m_playTimer.stop();
}

void CanvasWidget::stop()
{
    pause();
    saveCurrentFrame();
    m_curFrame = 0;
    if (m_proj && !m_proj->frames.isEmpty())
        m_figures = m_proj->frames[0].figures;
    selectedNode = nullptr;
    selectedBone = nullptr;
    update();
    emit frameChanged(0);
}

// ─── Coordinate helpers ──────────────────────────────────────────────────────
QPointF CanvasWidget::screenToWorld(QPointF s) const {
    return { (s.x() - m_panX) / m_zoom, (s.y() - m_panY) / m_zoom };
}
QPointF CanvasWidget::worldToScreen(QPointF w) const {
    return { w.x() * m_zoom + m_panX, w.y() * m_zoom + m_panY };
}

// ─── Hit testing ─────────────────────────────────────────────────────────────
Node* CanvasWidget::nodeAt(QPointF world, Figure **outFig)
{
    for (auto &fig : m_figures) {
        if (!fig.visible) continue;
        for (auto &n : fig.nodes) {
            if (!n.visible) continue;
            double dx = n.x - world.x(), dy = n.y - world.y();
            if (qSqrt(dx*dx+dy*dy) <= n.size + 5.0/m_zoom) {
                if (outFig) *outFig = &fig;
                return &n;
            }
        }
    }
    return nullptr;
}

Bone* CanvasWidget::boneAt(QPointF world, Figure **outFig)
{
    for (auto &fig : m_figures) {
        for (auto &b : fig.bones) {
            Node *n1 = fig.nodeById(b.fromId);
            Node *n2 = fig.nodeById(b.toId);
            if (!n1 || !n2) continue;
            double dx = n2->x-n1->x, dy = n2->y-n1->y;
            double len2 = dx*dx+dy*dy;
            if (len2 < 1) continue;
            double t = ((world.x()-n1->x)*dx+(world.y()-n1->y)*dy)/len2;
            if (t<0||t>1) continue;
            double px=n1->x+t*dx, py=n1->y+t*dy;
            double dist=qSqrt((world.x()-px)*(world.x()-px)+(world.y()-py)*(world.y()-py));
            if (dist <= b.width/2.0 + 5.0/m_zoom) {
                if (outFig) *outFig = &fig;
                return &b;
            }
        }
    }
    return nullptr;
}

QString CanvasWidget::uniqueId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

// ─── Figure factories ─────────────────────────────────────────────────────────
void CanvasWidget::addEmptyFigure()
{
    if (!m_proj) return;
    Figure fig;
    fig.id   = uniqueId();
    fig.name = QString("شخصية %1").arg(m_figures.size()+1);
    m_figures.append(fig);
    selectedFigure = &m_figures.last();
    saveCurrentFrame();
    emit projectModified();
    update();
}

void CanvasWidget::addHumanFigure()
{
    if (!m_proj) return;
    Figure fig;
    fig.id   = uniqueId();
    fig.name = QString("إنسان %1").arg(m_figures.size()+1);

    auto addN=[&](QString id,double x,double y,double sz,QColor c){
        Node n; n.id=id; n.x=x; n.y=y; n.size=sz; n.color=c; fig.nodes.append(n);
    };
    auto addB=[&](QString f,QString t,double w,QColor c,int r=0){
        Bone b; b.fromId=f; b.toId=t; b.width=w; b.color=c; b.round=r; fig.bones.append(b);
    };

    QColor blue(55,138,221), teal(29,158,117), gray(136,135,128), lgray(180,178,169);
    addN("head", 0,-120, 14, blue);
    addN("neck", 0, -95,  7, teal);
    addN("ls",  -30,-75,  7, teal);
    addN("rs",   30,-75,  7, teal);
    addN("le",  -52,-40,  6, gray);
    addN("re",   52,-40,  6, gray);
    addN("lh",  -58, -8,  5, gray);
    addN("rh",   58, -8,  5, gray);
    addN("hip",  0, -20,  8, teal);
    addN("lk",  -22, 32,  6, gray);
    addN("rk",   22, 32,  6, gray);
    addN("lf",  -27, 85,  6, gray);
    addN("rf",   27, 85,  6, gray);

    addB("head","neck", 4, gray);
    addB("neck","ls",   5, gray);
    addB("neck","rs",   5, gray);
    addB("ls",  "le",   4, lgray);
    addB("rs",  "re",   4, lgray);
    addB("le",  "lh",   3, lgray);
    addB("re",  "rh",   3, lgray);
    addB("neck","hip",  6, gray);
    addB("hip", "lk",   5, lgray);
    addB("hip", "rk",   5, lgray);
    addB("lk",  "lf",   4, lgray);
    addB("rk",  "rf",   4, lgray);

    m_figures.append(fig);
    selectedFigure = &m_figures.last();
    saveCurrentFrame();
    emit projectModified();
    update();
}

void CanvasWidget::addStickFigure()
{
    if (!m_proj) return;
    Figure fig;
    fig.id   = uniqueId();
    fig.name = QString("عصا %1").arg(m_figures.size()+1);

    auto addN=[&](QString id,double x,double y,double sz,QColor c){
        Node n; n.id=id; n.x=x; n.y=y; n.size=sz; n.color=c; fig.nodes.append(n);
    };
    auto addB=[&](QString f,QString t){
        Bone b; b.fromId=f; b.toId=t; b.width=3; b.color=QColor(68,68,65); fig.bones.append(b);
    };

    addN("h",  0,-100, 18, QColor(55,138,221));
    addN("b",  0, -30,  5, QColor(136,135,128));
    addN("lh", -45, 15, 5, QColor(136,135,128));
    addN("rh",  45, 15, 5, QColor(136,135,128));
    addN("lf", -22, 95, 5, QColor(68,68,65));
    addN("rf",  22, 95, 5, QColor(68,68,65));

    addB("h","b"); addB("b","lh"); addB("b","rh"); addB("b","lf"); addB("b","rf");

    m_figures.append(fig);
    selectedFigure = &m_figures.last();
    saveCurrentFrame();
    emit projectModified();
    update();
}

QImage CanvasWidget::renderFrameToImage(int frameIdx, int w, int h)
{
    if (!m_proj || frameIdx >= m_proj->frames.size()) return {};
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(m_proj->background);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.translate(w/2, h/2);
    drawFigures(p, m_proj->frames[frameIdx].figures, false);
    return img;
}

// ─── Painting ─────────────────────────────────────────────────────────────────
void CanvasWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), m_proj ? m_proj->background : QColor(30,30,40));

    if (!m_proj || m_proj->frames.isEmpty()) {
        p.setPen(QColor(100,100,120));
        p.setFont(QFont("sans",14));
        p.drawText(rect(), Qt::AlignCenter, "افتح مشروعاً أو ابدأ بإضافة شخصية");
        return;
    }

    if (showGrid)   drawGrid(p);
    if (showRulers) drawRulers(p);

    p.save();
    p.translate(m_panX, m_panY);
    p.scale(m_zoom, m_zoom);

    // Origin cross
    p.setPen(QPen(QColor(80,80,100,120), 1.0/m_zoom));
    p.drawLine(QLineF(-20/m_zoom, 0, 20/m_zoom, 0));
    p.drawLine(QLineF(0, -20/m_zoom, 0, 20/m_zoom));

    drawFigures(p, m_figures, true);

    // Bone-from indicator
    if (m_boneFromNode) {
        p.setPen(QPen(QColor(55,138,221,180), 2.0/m_zoom, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(m_boneFromNode->x, m_boneFromNode->y),
                      m_boneFromNode->size+8/m_zoom, m_boneFromNode->size+8/m_zoom);
    }

    p.restore();
}

void CanvasWidget::drawGrid(QPainter &p)
{
    double gridSm = 20 * m_zoom;
    double gridBg = 100 * m_zoom;

    p.setPen(QPen(QColor(100,100,130,40), 0.5));
    double ox = fmod(m_panX, gridSm); if(ox<0) ox+=gridSm;
    double oy = fmod(m_panY, gridSm); if(oy<0) oy+=gridSm;
    for (double x=ox; x<width();  x+=gridSm) p.drawLine(QLineF(x,0,x,height()));
    for (double y=oy; y<height(); y+=gridSm) p.drawLine(QLineF(0,y,width(),y));

    p.setPen(QPen(QColor(100,100,130,90), 0.8));
    ox = fmod(m_panX, gridBg); if(ox<0) ox+=gridBg;
    oy = fmod(m_panY, gridBg); if(oy<0) oy+=gridBg;
    for (double x=ox; x<width();  x+=gridBg) p.drawLine(QLineF(x,0,x,height()));
    for (double y=oy; y<height(); y+=gridBg) p.drawLine(QLineF(0,y,width(),y));
}

void CanvasWidget::drawRulers(QPainter &p)
{
    p.fillRect(0, 0, width(), 18, QColor(25,25,35,200));
    p.fillRect(0, 0, 18, height(), QColor(25,25,35,200));
    p.setPen(QColor(120,120,140));
    QFont f("monospace", 8); p.setFont(f);
    for (int x=0; x<width(); x+=50) {
        int wx = qRound((x-m_panX)/m_zoom);
        p.drawText(x+2, 12, QString::number(wx));
    }
    p.save();
    p.rotate(-90);
    for (int y=18; y<height(); y+=50) {
        int wy = qRound((y-m_panY)/m_zoom);
        p.drawText(-y-30, 12, QString::number(wy));
    }
    p.restore();
}

void CanvasWidget::drawFigures(QPainter &p, const QList<Figure> &figs, bool interactive)
{
    for (const auto &fig : figs) {
        if (!fig.visible) continue;

        // Bones
        for (const auto &b : fig.bones) {
            if (!b.visible) continue;
            const Node *n1 = nullptr, *n2 = nullptr;
            for (const auto &n : fig.nodes) {
                if (n.id==b.fromId) n1=&n;
                if (n.id==b.toId)   n2=&n;
            }
            if (!n1||!n2||!n1->visible||!n2->visible) continue;

            QPen pen(b.color, b.width);
            pen.setCapStyle(b.round>0 ? Qt::RoundCap : Qt::FlatCap);
            pen.setJoinStyle(Qt::RoundJoin);
            if (interactive && selectedBone == &b)
                pen.setColor(pen.color().lighter(160));
            p.setPen(pen);
            p.drawLine(QLineF(n1->x, n1->y, n2->x, n2->y));
        }

        // Nodes
        for (const auto &n : fig.nodes) {
            if (!n.visible) continue;
            bool isSel = interactive && (selectedNode == &n);
            p.setPen(isSel ? QPen(Qt::white,2) : QPen(n.color.darker(150),1));
            p.setBrush(n.color);
            p.drawEllipse(QPointF(n.x,n.y), n.size, n.size);
            if (isSel) {
                p.setPen(QPen(QColor(55,138,221,200), 2.0/m_zoom, Qt::DotLine));
                p.setBrush(Qt::NoBrush);
                p.drawEllipse(QPointF(n.x,n.y), n.size+5/m_zoom, n.size+5/m_zoom);
            }
        }
    }
}

// ─── Mouse events ─────────────────────────────────────────────────────────────
void CanvasWidget::mousePressEvent(QMouseEvent *e)
{
    setFocus();
    QPointF sp = e->position();

    // Middle-click or Alt+Left = pan
    if (e->button()==Qt::MiddleButton || (e->button()==Qt::LeftButton && e->modifiers()&Qt::AltModifier)) {
        m_panning = true;
        m_panStart = sp - QPointF(m_panX, m_panY);
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (e->button() != Qt::LeftButton) return;

    QPointF wp = screenToWorld(sp);

    switch (m_tool) {
    case Tool::Select: {
        Figure *hitFig = nullptr;
        Node   *hitNode = nodeAt(wp, &hitFig);
        if (hitNode) {
            selectedFigure = hitFig;
            selectedNode   = hitNode;
            selectedBone   = nullptr;
            m_draggingNode = true;
            m_dragOffset   = QPointF(wp.x()-hitNode->x, wp.y()-hitNode->y);
        } else {
            Figure *bFig = nullptr;
            Bone *hitBone = boneAt(wp, &bFig);
            if (hitBone) {
                selectedBone   = hitBone;
                selectedFigure = bFig;
                selectedNode   = nullptr;
            } else {
                selectedNode   = nullptr;
                selectedBone   = nullptr;
            }
        }
        emit selectionChanged();
        update();
        break;
    }
    case Tool::AddNode: {
        if (!selectedFigure) addEmptyFigure();
        if (!selectedFigure) break;
        Node n;
        n.id    = uniqueId();
        n.x     = wp.x(); n.y = wp.y();
        n.size  = defaultNodeSize;
        n.color = defaultNodeColor;
        selectedFigure->nodes.append(n);
        selectedNode = &selectedFigure->nodes.last();
        saveCurrentFrame();
        emit projectModified();
        emit selectionChanged();
        update();
        break;
    }
    case Tool::AddBone: {
        Figure *hitFig = nullptr;
        Node *hitNode = nodeAt(wp, &hitFig);
        if (!hitNode) break;
        if (!m_boneFromNode) {
            m_boneFromNode = hitNode;
            m_boneFromFig  = hitFig;
        } else {
            if (m_boneFromFig == hitFig && m_boneFromNode->id != hitNode->id) {
                Bone b;
                b.fromId = m_boneFromNode->id;
                b.toId   = hitNode->id;
                b.width  = defaultBoneWidth;
                b.color  = defaultBoneColor;
                b.round  = defaultBoneRound;
                hitFig->bones.append(b);
                saveCurrentFrame();
                emit projectModified();
            }
            m_boneFromNode = nullptr;
            m_boneFromFig  = nullptr;
        }
        update();
        break;
    }
    default: break;
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (m_panning) {
        QPointF delta = e->position() - m_panStart;
        m_panX = delta.x(); m_panY = delta.y();
        update();
        return;
    }
    if (m_draggingNode && selectedNode && m_tool==Tool::Select) {
        QPointF wp = screenToWorld(e->position());
        selectedNode->x = wp.x() - m_dragOffset.x();
        selectedNode->y = wp.y() - m_dragOffset.y();
        emit selectionChanged();
        update();
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_panning) {
        m_panning = false;
        setCursor(m_tool==Tool::Pan ? Qt::OpenHandCursor : Qt::ArrowCursor);
        return;
    }
    if (m_draggingNode) {
        m_draggingNode = false;
        saveCurrentFrame();
        emit projectModified();
    }
}

void CanvasWidget::wheelEvent(QWheelEvent *e)
{
    double factor = e->angleDelta().y() > 0 ? 1.12 : 1.0/1.12;
    QPointF mp = e->position();
    double oldZoom = m_zoom;
    m_zoom = qBound(0.05, m_zoom*factor, 10.0);
    m_panX = mp.x() - (mp.x()-m_panX)*(m_zoom/oldZoom);
    m_panY = mp.y() - (mp.y()-m_panY)*(m_zoom/oldZoom);
    update();
    emit selectionChanged();
}

void CanvasWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key()==Qt::Key_Delete && selectedNode) {
        if (selectedFigure) {
            // remove bones referencing this node
            QString nid = selectedNode->id;
            selectedFigure->bones.removeIf([&](const Bone &b){
                return b.fromId==nid || b.toId==nid;
            });
            selectedFigure->nodes.removeIf([&](const Node &n){ return n.id==nid; });
            selectedNode = nullptr;
            saveCurrentFrame();
            emit projectModified();
            emit selectionChanged();
            update();
        }
    }
    if (e->key()==Qt::Key_Space) {
        m_playing ? pause() : play();
    }
    // Arrow keys nudge
    if (selectedNode) {
        double step = e->modifiers()&Qt::ShiftModifier ? 10 : 1;
        if (e->key()==Qt::Key_Left)  { selectedNode->x-=step; }
        if (e->key()==Qt::Key_Right) { selectedNode->x+=step; }
        if (e->key()==Qt::Key_Up)    { selectedNode->y-=step; }
        if (e->key()==Qt::Key_Down)  { selectedNode->y+=step; }
        saveCurrentFrame(); emit projectModified(); emit selectionChanged(); update();
    }
    // Zoom
    if (e->key()==Qt::Key_Plus||e->key()==Qt::Key_Equal)  { m_zoom=qMin(10.0,m_zoom*1.2); update(); }
    if (e->key()==Qt::Key_Minus) { m_zoom=qMax(0.05,m_zoom/1.2); update(); }
    if (e->key()==Qt::Key_0)     { m_zoom=1.0; m_panX=width()/2; m_panY=height()/2; update(); }
}

void CanvasWidget::resizeEvent(QResizeEvent *)
{
    if (m_panX==0 && m_panY==0) { m_panX=width()/2; m_panY=height()/2; }
}

void CanvasWidget::syncFiguresToFrame()
{
    saveCurrentFrame();
}
