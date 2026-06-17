#pragma once
#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include "animdata.h"

class CodeEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CodeEditorDialog(QWidget *parent = nullptr);
    void setProject(AnimProject *p) { m_proj = p; }

signals:
    void codeApplied();

private slots:
    void runCode();
    void loadSample();

private:
    AnimProject *m_proj = nullptr;
    QTextEdit   *m_editor = nullptr;
    QLabel      *m_status = nullptr;

    // Script interpreter
    struct ScriptFigure {
        QString id, name;
        QList<Node> nodes;
        QList<Bone> bones;
    };
    QString uniqueId();
};
