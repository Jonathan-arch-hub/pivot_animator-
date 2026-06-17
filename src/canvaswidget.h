#pragma once
#include <QWidget>
#include <QTimer>
#include "animdata.h"

enum class Tool { Select, AddNode, AddBone, Pan };

class CanvasWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CanvasWidget(QWidget *parent = nullptr);

    // Project access
    AnimProject* project() { return m_proj; }
    void setProject(AnimProject *p);

    // Current frame
    int  currentFrame() const { return m_curFrame; }
    void setCurrentFrame(int i);
    void saveCurrentFrame();

    // Tool
    Tool tool() const { return m_tool; }
    void setTool(Tool t);

    // Playback
    void play();
    void pause();
    void stop();
    bool isPlaying() const { return m_playing; }

    // Defaults for new nodes/bones
    QColor defaultNodeColor = QColor(55,138,221);
    double defaultNodeSize  = 8;
    QColor defaultBoneColor = QColor(136,135,128);
    double defaultBoneWidth = 4;
    int    defaultBoneRound = 0;

    // Grid / rulers
    bool showGrid   = true;
    bool showRulers = true;

    // Selected items
    Figure *selectedFigure = nullptr;
    Node   *selectedNode   = nullptr;
    Bone   *selectedBone   = nullptr;

    // Helpers for external use
    void addHumanFigure();
    void addStickFigure();
    void addEmptyFigure();
    Figure* addFigureToFrame(Frame &fr);

    // Export a single frame to image
    QImage renderFrameToImage(int frameIdx, int w, int h);

signals:
    void frameChanged(int idx);
    void selectionChanged();
    void projectModified();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void resizeEvent(QResizeEvent *) override;

private:
    AnimProject *m_proj    = nullptr;
    int          m_curFrame = 0;
    Tool         m_tool    = Tool::Select;

    // View transform
    double m_zoom = 1.0;
    double m_panX = 0, m_panY = 0;

    // Drag state
    bool    m_draggingNode = false;
    QPointF m_dragOffset;
    bool    m_panning = false;
    QPointF m_panStart;

    // Add-bone state
    Node  *m_boneFromNode = nullptr;
    Figure *m_boneFromFig  = nullptr;

    // Playback
    bool   m_playing   = false;
    int    m_playDir   = 1;
    QTimer m_playTimer;

    // Helpers
    QPointF screenToWorld(QPointF s) const;
    QPointF worldToScreen(QPointF w) const;
    Node*   nodeAt(QPointF world, Figure **outFig = nullptr);
    Bone*   boneAt(QPointF world, Figure **outFig = nullptr);
    void    drawGrid(QPainter &p);
    void    drawRulers(QPainter &p);
    void    drawFigures(QPainter &p, const QList<Figure> &figs, bool interactive);
    QString uniqueId();

    void syncFiguresToFrame();   // apply current m_figures to frame
    QList<Figure> m_figures;     // live working copy
};
