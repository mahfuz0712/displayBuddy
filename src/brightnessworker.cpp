#include "brightnessworker.h"

BrightnessWorker::BrightnessWorker(QObject* parent) : QObject(parent) {
    qRegisterMetaType<DisplayInfo>("DisplayInfo");
    qRegisterMetaType<QVector<DisplayInfo>>("QVector<DisplayInfo>");
}

void BrightnessWorker::scan() {
    emit scanned(BrightnessBackend::displays());
}

void BrightnessWorker::apply(const QString& id, int value) {
    QString error;
    const bool ok = BrightnessBackend::setBrightness(id, value, &error);
    emit applied(id, value, ok, error);
}
