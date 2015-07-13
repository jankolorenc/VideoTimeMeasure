#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

#include <QDialog>

namespace Ui {
class ScriptEditor;
}

class ScriptEditor : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptEditor(QWidget *parent = 0);

    void setScript(QString script);
    QString getScript();

    ~ScriptEditor();

private:
    Ui::ScriptEditor *ui;
};

#endif // SCRIPTEDITOR_H
