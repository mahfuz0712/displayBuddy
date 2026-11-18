#pragma once
#include <QString>
#include <QVector>

struct DisplayInfo {
    QString id;
    QString name;
    int brightness = 50;
    int minimum = 0;
    int maximum = 100;
    bool controllable = true;
};

class BrightnessBackend {
public:
    static QVector<DisplayInfo> displays();
    static bool setBrightness(const QString& id, int value, QString* error = nullptr);
};
