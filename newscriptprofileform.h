#ifndef NEWSCRIPTPROFILEFORM_H
#define NEWSCRIPTPROFILEFORM_H

#include <QDialog>

namespace Ui {
class NewScriptProfileForm;
}

/**
 * @brief The NewScriptProfileForm class
 * Form to create new profile script
 */
class NewScriptProfileForm : public QDialog
{
    Q_OBJECT

public:
    explicit NewScriptProfileForm(QWidget *parent = 0);
    /**
     * @brief getProfileName
     * @return profile name entered by user
     */
    QString getProfileName();
    ~NewScriptProfileForm();

private:
    Ui::NewScriptProfileForm *ui;
};

#endif // NEWSCRIPTPROFILEFORM_H
