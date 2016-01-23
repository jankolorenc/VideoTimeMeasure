#ifndef SCRIPTEDITOR_H
#define SCRIPTEDITOR_H

#include <QDialog>

namespace Ui {
class ScriptEditor;
}

/**
 * @brief The ScriptEditor class
 * Dialog to edit profile script
 */
class ScriptEditor : public QDialog
{
    Q_OBJECT

public:
    explicit ScriptEditor(QWidget *parent = 0);

    /**
     * @brief setScript
     * Set script to script editor to let user modify it
     * @param script
     */
    void setScript(QString script);
    /**
     * @brief getScript
     * @return scrit from script editor modified by user
     */
    QString getScript();

    ~ScriptEditor();

private:
    Ui::ScriptEditor *ui;
};

#endif // SCRIPTEDITOR_H
