#include "newscriptprofileform.h"
#include "ui_newscriptprofileform.h"

NewScriptProfileForm::NewScriptProfileForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewScriptProfileForm)
{
    ui->setupUi(this);
}

NewScriptProfileForm::~NewScriptProfileForm()
{
    delete ui;
}

QString NewScriptProfileForm::getProfileName()
{
    return ui->profileNameLineEdit->text();
}
