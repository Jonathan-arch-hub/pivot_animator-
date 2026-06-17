#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QHBoxLayout>
#include "animdata.h"

class TimelineWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget *parent = nullptr);
    void setProject(AnimProject *p);
    void refresh(int currentFrame);

signals:
    void frameSelected(int idx);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    AnimProject *m_proj   = nullptr;
    int          m_cur    = 0;
    QScrollArea *m_scroll = nullptr;
    QWidget     *m_inner  = nullptr;
    QHBoxLayout *m_layout = nullptr;
    void rebuild();
};
// Note: eventFilter declared in protected above
