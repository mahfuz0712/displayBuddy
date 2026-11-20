#pragma once
#include <QMetaType>
#include <QString>
#include <QVector>

struct DisplayInfo {
    QString id;
    QString name;
    int brightness = 50;   // percent
    int minimum = 0;
    int maximum = 100;
    bool controllable = true;
};
Q_DECLARE_METATYPE(DisplayInfo)

class BrightnessBackend {
public:
    // Blocking calls - run them from BrightnessWorker, not from the GUI thread.
    static QVector<DisplayInfo> displays();
    static bool setBrightness(const QString& id, int value, QString* error = nullptr);
};
