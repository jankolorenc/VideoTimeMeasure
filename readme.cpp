#include "readme.h"
#include "ui_readme.h"
#include <QFile>
#include <QTextStream>

ReadMe::ReadMe(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReadMe)
{
    ui->setupUi(this);

    QFile file(":/README.html");
    QString content;
    if (file.open(QFile::ReadOnly | QFile::Text)){
        QTextStream in(&file);
        content = in.readAll();
    }
    ui->textBrowser->setHtml(content);
}

ReadMe::~ReadMe()
{
    delete ui;
}
