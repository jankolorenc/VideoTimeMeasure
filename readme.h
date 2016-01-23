#ifndef README_H
#define README_H

#include <QDialog>

namespace Ui {
class ReadMe;
}

/**
 * @brief The ReadMe class
 * Dialog to show readme
 */
class ReadMe : public QDialog
{
    Q_OBJECT

public:
    explicit ReadMe(QWidget *parent = 0);
    ~ReadMe();

private:
    Ui::ReadMe *ui;
};

#endif // README_H
