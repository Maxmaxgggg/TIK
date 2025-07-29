#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    const int BUF_SIZE = 1 << 15;
    ~Widget();

private slots:
    void on_outputPBN_clicked();

    void on_inputPBN_clicked();

    void on_executePBN_clicked();

private:
    Ui::Widget  *ui;
    QString     outputFileName;
    QStringList inputFileNames;
    void        setupInputPTEContextMenu();
};
#endif // WIDGET_H
