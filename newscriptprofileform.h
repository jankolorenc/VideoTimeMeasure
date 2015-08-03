#ifndef NEWSCRIPTPROFILEFORM_H
#define NEWSCRIPTPROFILEFORM_H

#include <QDialog>

namespace Ui {
class NewScriptProfileForm;
}

class NewScriptProfileForm : public QDialog
{
    Q_OBJECT

public:
    explicit NewScriptProfileForm(QWidget *parent = 0);
    QString getProfileName();
    ~NewScriptProfileForm();

private:
    Ui::NewScriptProfileForm *ui;
};

#endif // NEWSCRIPTPROFILEFORM_H
