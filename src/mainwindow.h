#pragma once

#include <QMainWindow>
#include <QVector>
#include <QMap>
#include "brightnessbackend.h"

class QScrollArea;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QSlider;
class QPushButton;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void refreshDisplays();
    void applyPreset(const QString& name, int value);
    void saveCustomPreset();
    void removeCustomPreset(const QString& name);

private:
    struct DisplayWidgets {
        QString id;
        QLabel* value = nullptr;
        QSlider* slider = nullptr;
        QLabel* status = nullptr;
        QTimer* debounce = nullptr;
    };

    QVector<DisplayInfo> m_displays;
    QVector<DisplayWidgets> m_widgets;
    QWidget* m_listHost = nullptr;
    QVBoxLayout* m_listLayout = nullptr;
    QHBoxLayout* m_customPresetLayout = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_count = nullptr;
    QMap<QString, int> m_customPresets;

    void buildDisplayCards();
    void loadCustomPresets();
    void rebuildCustomPresetButtons();
    void setDisplayBrightness(int index, int value);
    void setPresetButtonState(const QString& name);
};
