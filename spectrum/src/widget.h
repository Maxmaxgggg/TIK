#ifndef WIDGET_H
#define WIDGET_H

#include "qcustomplot.h"
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
    ~Widget();
protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
private slots:
    void on_executePBN_clicked();
    void updatePlot();
    void on_exitPBN_clicked();

private:
    Ui::Widget *ui;
    quint64  *spectrum;
    QSplitter *splitter;
    QCPBars  *spectrumBars;
    quint64  n;
    quint64  k;
};
#endif // WIDGET_H
