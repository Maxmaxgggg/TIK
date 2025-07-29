#include "widget.h"
#include "ui_widget.h"
//#include <omp.h>
#include "kernel.cuh"
#include <cuda_runtime.h>

Widget::Widget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    QSettings s;
    this->restoreGeometry(s.value("geometry").toByteArray());
    ui->matrixPTE->setPlainText(s.value("matrix").toString());
    ui->spectrumPTE->setPlainText(s.value("spectrum").toString());
    ui->spectrumCPT->installEventFilter(this);
    splitter = new QSplitter(Qt::Horizontal);  
    splitter->addWidget(ui->spectrumPTE);
    splitter->addWidget(ui->spectrumCPT);
    ui->verticalLayout->insertWidget(3, splitter);
    ui->verticalLayout->setStretch(1, 1);
    ui->verticalLayout->setStretch(3, 1);
    connect(ui->exitPBN, &QPushButton::clicked, this, &QWidget::close);

    splitter->restoreState(s.value("splitter").toByteArray());
    spectrum = nullptr;
    spectrumBars = new QCPBars(ui->spectrumCPT->xAxis, ui->spectrumCPT->yAxis);
    spectrumBars->setName("Гистограмма");
    spectrumBars->setBrush(QColor(50, 100, 250, 70));  // Цвет заливки
    spectrumBars->setPen(QPen(Qt::black));             // Обводка

    QVariantList spectrumData = s.value("spectrumData").toList();
    if (!spectrumData.isEmpty()) {
        n = spectrumData.size() - 1;
        spectrum = (quint64*)calloc(n + 1, sizeof(quint64));
        for (int i = 0; i < n + 1; ++i) {
            spectrum[i] = spectrumData[i].toULongLong();
        }
        this->updatePlot();
    }
}

Widget::~Widget()
{
    QSettings s;
    if ( spectrum != nullptr )
        free(spectrum);
    s.setValue("geometry", this->saveGeometry());
    s.setValue("matrix", ui->matrixPTE->toPlainText());
    s.setValue("spectrum", ui->spectrumPTE->toPlainText());
    s.setValue("splitter", this->splitter->saveState());
    delete ui;
}

void Widget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event); // если не используешь размеры

    updatePlot(); // обновляем график при изменении размера окна

    QWidget::resizeEvent(event); // обязательно вызываем базовый обработчик
}
void Widget::on_executePBN_clicked()
{
    ui->executePBN->setEnabled(false);
    QStringList rows = ui->matrixPTE->toPlainText().split('\n', Qt::SkipEmptyParts);


    // Проверяем, что все строки матрицы одной длины
    bool allSameLength = true;
    if (!rows.isEmpty()) {
        int expectedLength = rows.first().length();

        for (const QString& row : std::as_const(rows)) {
            if (row.length() != expectedLength) {
                allSameLength = false;
                break;
            }
        }
    }
    else {
        ui->executePBN->setEnabled(true);
        return;
    }
    if (!allSameLength) {
        QMessageBox::critical(this, "Error", "Error: not all rows of the matrix have the same length");
        ui->executePBN->setEnabled(true);
        return;
    }
    n = rows.first().length();
    k = rows.size();
    if (n > 1024) {
        QMessageBox::critical(this, "Error", "Error: matrix row length must not exceed 1024");
        ui->executePBN->setEnabled(true);
        return;
    }
    if ( n*k > 64*8192) {
        QMessageBox::critical(this, "Error", "Error: matrix must contain less than 524 288 elements");
        ui->executePBN->setEnabled(true);
        return;
    }
    // Максимальное число единиц - n, минимальное - 0. Всего n+1 вариант
    spectrum = (quint64*)calloc(n + 1, sizeof(quint64));
    if (!spectrum) {
        QMessageBox::critical(this, "Error", "Error: failed to allocate memory");
        ui->executePBN->setEnabled(true);
        return;
    }

    // Сколько 64-битных слов нужно на строку
    quint64 blockCount = (n + 63) / 64;
    // Выделяем сырую память и сразу обнуляем через calloc
    quint64* packed = (quint64*)calloc(k * blockCount, sizeof(quint64));
    if (!packed) {
        QMessageBox::critical(this, "Error", "Error: failed to allocate memory");
        return;
    }
    // Заполняем биты: каждая строка занимает blockCount подряд слов
    for (quint64 i = 0; i < k; ++i) {
        const QString& row = rows[i];
        quint64* rowData = packed + i * blockCount;
        for (quint64 j = 0; j < n; ++j) {
            if (row.at(j) == QLatin1Char('1')) {
                quint64 blockIdx = j / 64;
                quint64 bitIdx = j % 64;
                rowData[blockIdx] |= (quint64(1) << bitIdx);
            }
        }
    }
#ifdef QT_DEBUG
    // Выводим упакованные данные для отладки через qDebug()
    for (quint64 i = 0; i < k; ++i) {
        quint64* rowData = packed + i * blockCount;
        // Начинаем строку вывода
        QString line = QString("Row %1:").arg(i);
        for (quint64 b = 0; b < blockCount; ++b) {
            // Форматируем 64-битное слово в двоичную строку с ведущими нулями
            line += ' ' + QString::number(rowData[b], 2).rightJustified(64, QChar('0'));
        }
        qDebug().noquote() << line;
    }
#endif
    qint64 numComb = 1LL << k;


    const int threadsPerBlock = 512;
    const int blocks = 64;

    // spectrum и packed уже есть: 
    //   packed — упакованная матрица на хосте (k * blockCount элементов)
    //   spectrum — массив нулей длины n+1

    // Вызываем CUDA–ядро
    try {
        launchSpectrumKernel(
            packed,            // h_matrixPacked
            spectrum,          // h_spectrum
            n,                 // длина слова
            k,                 // число строк
            static_cast<int>(blockCount),  // blockCount
            threadsPerBlock,
            blocks
        );
    }
    catch (const std::exception& ex) {
        QMessageBox::critical(this, "Error", ex.what());
        ui->executePBN->setEnabled(true);
        free(packed);
        return;
    }
    this->updatePlot();
    // 5. Вывод спектра
    QString s = "";
    for (quint64 w = 0; w <= n; ++w) {
        if (spectrum[w] != 0) {
            #ifdef QT_DEBUG
                qDebug() << w << " - " << spectrum[w];
            #endif
            s += QString::number(w) + " - " + QString::number(spectrum[w]) + '\n';
        }
    }
    if (!s.isEmpty() && s.endsWith('\n'))
        s.chop(1); // удаляет последний символ
    ui->spectrumPTE->setPlainText(s);
    ui->executePBN->setEnabled(true);
    free(packed);
}

void Widget::updatePlot()
{
    if (spectrum != nullptr && spectrumBars != nullptr)
    {
        int size = n + 1; // n — глобальная длина
        QVector<double> x(size), y(size);
        QVector<double> ticks;
        QVector<QString> labels;

        // Выбор шага для меток: если окно узкое, показываем через каждые 5
        int step = (ui->spectrumCPT->width() < 800) ? 5 : 1;
        if (ui->spectrumCPT->width() < 200)
            step = 10;
        for (int i = 0; i < size; ++i) {
            x[i] = i;
            y[i] = static_cast<double>(spectrum[i]);
            ticks << i;
            if (i % step == 0)
                labels << QString::number(i); // отображаем метку
            else
                labels << ""; // пропускаем метку
        }

        // Установка данных в гистограмму
        spectrumBars->setData(x, y);

        // Настройка оси X с текстовыми метками
        QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
        textTicker->addTicks(ticks, labels);
        ui->spectrumCPT->xAxis->setTicker(textTicker);

        ui->spectrumCPT->xAxis->setSubTicks(false);
        ui->spectrumCPT->xAxis->setTickLength(0, 4);
        ui->spectrumCPT->xAxis->setRange(-1, size);

        // Автоматическая подстройка Y-оси
        double maxVal = *std::max_element(y.begin(), y.end());
        ui->spectrumCPT->yAxis->setRange(0, maxVal * 1.1);

        // Перерисовка графика
        ui->spectrumCPT->replot(QCustomPlot::rpQueuedReplot);

        QSettings s;
        QVariantList spectrumData;
        for (int i = 0; i < n + 1; ++i) {
            spectrumData << spectrum[i];
        }
        s.setValue("spectrumData", spectrumData);
    }
}
bool Widget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->spectrumCPT && event->type() == QEvent::Resize) {
        updatePlot(); // вызывать при изменении размера QCustomPlot
    }
    return QWidget::eventFilter(watched, event);
}

