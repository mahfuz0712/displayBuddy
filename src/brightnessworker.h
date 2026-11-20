#pragma once
#include <QObject>
#include <QVector>
#include "brightnessbackend.h"

// Lives in its own QThread so slow DDC/CI, WMI or ddcutil calls never freeze the GUI.
// Requests are processed strictly in order, so the last slider change always wins.
class BrightnessWorker : public QObject {
    Q_OBJECT
public:
    explicit BrightnessWorker(QObject* parent = nullptr);

public slots:
    void scan();
    void apply(const QString& id, int value);

signals:
    void scanned(const QVector<DisplayInfo>& displays);
    void applied(const QString& id, int value, bool ok, const QString& error);
};
