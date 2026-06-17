#pragma once
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <cmath>
#include "types.h"

enum class Tool { Select, AddNode, AddBone };

class Canvas : public QWidget {
    Q_OBJECT
public:
    QVector<Figure>* figures = nullptr;
    Tool tool = Tool::Select;

    qreal zoom = 1.0;
    qreal panX = 0, panY = 0;
    QColor bgColor = QColor("#f0f0f0");
    bool showGrid = true;
    bool showRulers = false;

    Figure* selectedFig = nullptr;
    Node*   selectedNode = nullptr;
    Bone*   selectedBone = nullptr;

    // for add-bone
    Figure* boneFromFig = nullptr;
    Node*   boneFromNode = nullptr;

    // default props for new nodes/bones
    QColor  defNodeColor = QColor("#378ADD");
    qreal   defNodeSize  = 8;
    QColor  defBoneColor = QColor("#888780");
    qreal   defBoneWidth = 4;
    bool    defBoneRound = false;

    explicit Canvas(QWidget* parent=nullptr) : QWidget(parent) {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
    }

signals:
    void nodeSelected(Node* n, Figure* f);
    void selectionCleared();
    void figuresChanged();
    void statusMessage(const QString& msg);

protected:
    bool   dragging    = false;
    bool   panning     = false;
    Node*  dragNode    = nullptr;
    QPointF dragOffset;
    QPointF panStart;

    QPointF screenToWorld(QPointF p) const {
        return { (p.x() - panX) / zoom, (p.y() - panY) / zoom };
    }
    QPointF worldToScreen(QPointF p) const {
        return { p.x() * zoom + panX, p.y() * zoom + panY };
    }

    Node* hitNode(QPointF world, Figure** outFig = nullptr) {
        if (!figures) return nullptr;
        for (auto& fig : *figures) {
            if (!fig.visible) continue;
            for (auto& n : fig.nodes) {
                if (!n.visible) continue;
                qreal dx = n.x - world.x(), dy = n.y - world.y();
                if (std::sqrt(dx*dx+dy*dy) <= n.size + 5) {
                    if (outFig) *outFig = &fig;
                    return &n;
                }
            }
        }
        return nullptr;
    }

    Bone* hitBone(QPointF world, Figure** outFig = nullptr) {
        if (!figures) return nullptr;
        for (auto& fig : *figures) {
            for (auto& b : fig.bones) {
                const Node* n1 = fig.findNode(b.from);
                const Node* n2 = fig.findNode(b.to);
                if (!n1 || !n2) continue;
                qreal dx = n2->x-n1->x, dy = n2->y-n1->y;
                qreal len = std::sqrt(dx*dx+dy*dy);
                if (len < 1) continue;
                qreal t = ((world.x()-n1->x)*dx+(world.y()-n1->y)*dy)/(len*len);
                if (t < 0 || t > 1) continue;
                qreal px = n1->x+t*dx, py = n1->y+t*dy;
                qreal dist = std::sqrt(std::pow(world.x()-px,2)+std::pow(world.y()-py,2));
                if (dist <= b.width/2.0 + 5) {
                    if (outFig) *outFig = &fig;
                    return &b;
                }
            }
        }
        return nullptr;
    }

    void mousePressEvent(QMouseEvent* e) override {
        QPointF sp(e->pos());
        if (e->button() == Qt::MiddleButton || (e->button()==Qt::LeftButton && e->modifiers()&Qt::AltModifier)) {
            panning = true; panStart = sp - QPointF(panX,panY);
            setCursor(Qt::ClosedHandCursor); return;
        }
        if (e->button() != Qt::LeftButton) return;

        QPointF wp = screenToWorld(sp);

        if (tool == Tool::Select) {
            Figure* fig = nullptr;
            Node* n = hitNode(wp, &fig);
            if (n) {
                selectedFig = fig; selectedNode = n; selectedBone = nullptr;
                dragNode = n; dragging = true;
                dragOffset = { wp.x()-n->x, wp.y()-n->y };
                emit nodeSelected(n, fig);
            } else {
                Figure* bfig = nullptr;
                Bone* b = hitBone(wp, &bfig);
                if (b) { selectedBone = b; selectedNode = nullptr; selectedFig = bfig; }
                else   { selectedNode = nullptr; selectedBone = nullptr; emit selectionCleared(); }
            }

        } else if (tool == Tool::AddNode) {
            if (!selectedFig && figures && !figures->isEmpty()) selectedFig = &figures->last();
            if (!selectedFig) return;
            Node nd(wp.x(), wp.y(), defNodeSize, defNodeColor);
            selectedFig->nodes.append(nd);
            selectedNode = &selectedFig->nodes.last();
            emit nodeSelected(selectedNode, selectedFig);
            emit figuresChanged();

        } else if (tool == Tool::AddBone) {
            Figure* fig = nullptr;
            Node* n = hitNode(wp, &fig);
            if (!n) return;
            if (!boneFromNode) {
                boneFromNode = n; boneFromFig = fig;
                emit statusMessage("الآن انقر على المفصل الثاني لرسم العظمة");
            } else {
                if (boneFromFig == fig && boneFromNode->id != n->id) {
                    Bone b; b.from = boneFromNode->id; b.to = n->id;
                    b.width = defBoneWidth; b.color = defBoneColor; b.rounded = defBoneRound;
                    fig->bones.append(b);
                    emit figuresChanged();
                    emit statusMessage("عظمة مضافة");
                }
                boneFromNode = nullptr; boneFromFig = nullptr;
            }
        }
        update();
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        QPointF sp(e->pos());
        if (panning) { panX=sp.x()-panStart.x(); panY=sp.y()-panStart.y(); update(); return; }
        if (dragging && dragNode) {
            QPointF wp = screenToWorld(sp);
            dragNode->x = wp.x() - dragOffset.x();
            dragNode->y = wp.y() - dragOffset.y();
            emit figuresChanged();
            emit nodeSelected(dragNode, selectedFig);
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button()==Qt::MiddleButton || panning) { panning=false; setCursor(Qt::ArrowCursor); }
        dragging = false; dragNode = nullptr;
    }

    void wheelEvent(QWheelEvent* e) override {
        QPointF mp(e->position());
        qreal oldZ = zoom;
        zoom *= e->angleDelta().y() > 0 ? 1.12 : 0.88;
        zoom = qBound(0.05, zoom, 10.0);
        panX = mp.x() - (mp.x()-panX)*(zoom/oldZ);
        panY = mp.y() - (mp.y()-panY)*(zoom/oldZ);
        update();
    }

    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // background
        p.fillRect(rect(), bgColor);

        // grid
        if (showGrid) {
            qreal gs = 20*zoom;
            qreal ox = fmod(fmod(panX,gs)+gs, gs);
            qreal oy = fmod(fmod(panY,gs)+gs, gs);
            p.setPen(QPen(QColor(0,0,0,25), 0.5));
            for (qreal x=ox; x<width(); x+=gs) p.drawLine(QPointF(x,0),QPointF(x,height()));
            for (qreal y=oy; y<height(); y+=gs) p.drawLine(QPointF(0,y),QPointF(width(),y));
            qreal bgs = 100*zoom;
            qreal bx = fmod(fmod(panX,bgs)+bgs,bgs);
            qreal by = fmod(fmod(panY,bgs)+bgs,bgs);
            p.setPen(QPen(QColor(0,0,0,50), 0.8));
            for (qreal x=bx; x<width(); x+=bgs) p.drawLine(QPointF(x,0),QPointF(x,height()));
            for (qreal y=by; y<height(); y+=bgs) p.drawLine(QPointF(0,y),QPointF(width(),y));
        }

        // rulers
        if (showRulers) {
            p.fillRect(0,0,width(),18,QColor(0,0,0,18));
            p.fillRect(0,0,18,height(),QColor(0,0,0,18));
            p.setPen(QColor(80,80,80,180));
            p.setFont(QFont("monospace",7));
            for (int x=0;x<width();x+=50) p.drawText(x+2,12,QString::number(int((x-panX)/zoom)));
            for (int y=0;y<height();y+=50) p.drawText(2,y+16,QString::number(int((y-panY)/zoom)));
        }

        p.save();
        p.translate(panX, panY);
        p.scale(zoom, zoom);

        if (!figures) { p.restore(); return; }

        for (auto& fig : *figures) {
            if (!fig.visible) continue;

            // bones
            for (auto& b : fig.bones) {
                const Node* n1 = fig.findNode(b.from);
                const Node* n2 = fig.findNode(b.to);
                if (!n1||!n2||!n1->visible||!n2->visible) continue;

                QPen pen(b.color, b.width);
                pen.setCapStyle(b.rounded ? Qt::RoundCap : Qt::FlatCap);
                pen.setJoinStyle(Qt::RoundJoin);
                p.setPen(pen);

                if (selectedBone == &b) {
                    QPen glow(QColor(55,138,221,80), b.width+8);
                    glow.setCapStyle(Qt::RoundCap);
                    p.setPen(glow);
                    p.drawLine(QPointF(n1->x,n1->y), QPointF(n2->x,n2->y));
                    p.setPen(pen);
                }
                p.drawLine(QPointF(n1->x,n1->y), QPointF(n2->x,n2->y));
            }

            // nodes
            for (auto& n : fig.nodes) {
                if (!n.visible) continue;
                bool sel = (selectedNode == &n);
                if (sel) {
                    p.setPen(Qt::NoPen);
                    p.setBrush(QColor(55,138,221,60));
                    p.drawEllipse(QPointF(n.x,n.y), n.size+5, n.size+5);
                }
                p.setPen(sel ? QPen(Qt::white,2) : Qt::NoPen);
                p.setBrush(n.color);
                p.drawEllipse(QPointF(n.x,n.y), n.size, n.size);
            }
        }

        // add-bone preview
        if (boneFromNode) {
            p.setPen(QPen(QColor("#378ADD"), 1.5, Qt::DashLine));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(boneFromNode->x,boneFromNode->y), boneFromNode->size+7, boneFromNode->size+7);
        }

        p.restore();
    }
};
