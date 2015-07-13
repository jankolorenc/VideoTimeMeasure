#include "scripteditor.h"
#include "ui_scripteditor.h"

ScriptEditor::ScriptEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScriptEditor)
{
    ui->setupUi(this);
}

ScriptEditor::~ScriptEditor()
{
    delete ui;
}

void ScriptEditor::setScript(QString script){
    ui->textEdit->setPlainText(script);
}

QString ScriptEditor::getScript(){
    return ui->textEdit->toPlainText().trimmed();
}
