#include "mainwindow.h"

#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMessageBox>
#include <QFont>
#include <QApplication>
#include <QClipboard>
#include <QStyle>
#include <QScrollArea>
#include <QInputDialog>
#include <QSettings>
#include <QTimer>
#include <QMenu>
#include <QToolButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPainter>
#include <QStyleOption>
#include <QSizePolicy>

namespace {
class GlyphLabel : public QLabel {
public:
    explicit GlyphLabel(const QString& glyph, QWidget* parent = nullptr) : QLabel(glyph, parent) {
        setAlignment(Qt::AlignCenter);
        setFixedSize(28, 28);
        setObjectName("glyph");
    }
};
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("DisplayBuddy");
    setMinimumSize(430, 520);
    resize(470, 720);
    setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);

    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(22, 16, 22, 18);
    root->setSpacing(12);

    // Compact title bar content.
    auto* header = new QHBoxLayout;
    auto* brand = new QLabel("DisplayBuddy");
    QFont brandFont = brand->font();
    brandFont.setPointSize(11);
    brandFont.setWeight(QFont::DemiBold);
    brand->setFont(brandFont);
    header->addWidget(brand);
    header->addStretch();

    auto* refresh = new QToolButton;
    refresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    refresh->setToolTip("Refresh displays");
    refresh->setObjectName("toolButton");
    refresh->setFixedSize(34, 34);
    header->addWidget(refresh);

    auto* settings = new QToolButton;
    settings->setText("⚙");
    settings->setToolTip("Preset settings");
    settings->setObjectName("toolButton");
    settings->setFixedSize(34, 34);
    header->addWidget(settings);
    root->addLayout(header);

    auto* presetTitle = new QLabel("Presets");
    presetTitle->setAlignment(Qt::AlignCenter);
    presetTitle->setObjectName("sectionTitle");
    root->addWidget(presetTitle);

    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(8);
    const auto addPreset = [this, presetRow](const QString& name, int value) {
        auto* button = new QPushButton(name);
        button->setObjectName("presetButton");
        button->setMinimumHeight(36);
        presetRow->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, name, value] {
            applyPreset(name, value);
        });
    };
    addPreset("Designing", 85);
    addPreset("Coding", 65);
    addPreset("Editing", 95);
    root->addLayout(presetRow);

    auto* customRow = new QHBoxLayout;
    customRow->addStretch();
    auto* customButton = new QPushButton("+ Custom preset");
    customButton->setObjectName("customButton");
    customRow->addWidget(customButton);
    root->addLayout(customRow);

    m_customPresetLayout = new QHBoxLayout;
    m_customPresetLayout->setSpacing(8);
    root->addLayout(m_customPresetLayout);

    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("separator");
    root->addWidget(line);

    m_count = new QLabel("Connected displays");
    m_count->setObjectName("sectionTitle");
    root->addWidget(m_count);

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName("displayScroll");
    m_listHost = new QWidget;
    m_listLayout = new QVBoxLayout(m_listHost);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(12);
    m_listLayout->addStretch();
    scroll->setWidget(m_listHost);
    root->addWidget(scroll, 1);

    m_status = new QLabel("Detecting displays…");
    m_status->setObjectName("status");
    m_status->setWordWrap(true);
    root->addWidget(m_status);

    setCentralWidget(central);
    setStyleSheet(R"(
        QMainWindow, QWidget {
            background: #f3f4f8;
            color: #252936;
            font-family: "Segoe UI Variable", "Segoe UI", sans-serif;
            font-size: 9.5pt;
        }
        QLabel#sectionTitle {
            color: #424858;
            font-size: 9pt;
            font-weight: 600;
        }
        QLabel#glyph {
            color: #303744;
            font-size: 18pt;
            background: transparent;
        }
        QLabel#muted { color: #747b89; font-size: 8.5pt; }
        QLabel#brightnessValue { color: #2867a8; font-size: 10pt; font-weight: 600; }
        QLabel#status { color: #697181; font-size: 8.5pt; padding-top: 4px; }
        QFrame#displayCard {
            background: #ffffff;
            border: 1px solid #dfe3eb;
            border-radius: 10px;
        }
        QFrame#separator { color: #dce0e8; max-height: 1px; }
        QScrollArea#displayScroll { background: transparent; }
        QScrollArea#displayScroll QWidget { background: transparent; }
        QPushButton#presetButton {
            background: #ffffff;
            color: #303744;
            border: 1px solid #d6dbe5;
            border-radius: 6px;
            padding: 7px 12px;
        }
        QPushButton#presetButton:hover { background: #eaf3fc; border-color: #8bb8e6; }
        QPushButton#customButton {
            background: transparent;
            color: #2867a8;
            border: 0;
            padding: 4px 2px;
            font-weight: 600;
        }
        QPushButton#customButton:hover { color: #124c88; text-decoration: underline; }
        QToolButton#toolButton {
            background: transparent;
            color: #50596a;
            border: 0;
            border-radius: 6px;
            font-size: 15pt;
        }
        QToolButton#toolButton:hover { background: #e4e8f0; }
        QSlider::groove:horizontal {
            height: 4px;
            background: #858b96;
            border-radius: 2px;
        }
        QSlider::sub-page:horizontal { background: #2867a8; border-radius: 2px; }
        QSlider::handle:horizontal {
            background: #ffffff;
            border: 2px solid #2867a8;
            width: 14px;
            height: 14px;
            margin: -6px 0;
            border-radius: 8px;
        }
        QSlider:disabled { opacity: 0.45; }
        QMenu { background: white; border: 1px solid #d8dce5; padding: 4px; }
        QMenu::item { padding: 7px 24px 7px 10px; border-radius: 4px; }
        QMenu::item:selected { background: #e7f1fb; color: #1f5f9f; }
        QDialog { background: #f3f4f8; }
        QLineEdit {
            background: white; border: 1px solid #cbd2df;
            border-radius: 6px; padding: 7px;
        }
    )");

    connect(refresh, &QToolButton::clicked, this, &MainWindow::refreshDisplays);
    connect(customButton, &QPushButton::clicked, this, &MainWindow::saveCustomPreset);
    connect(settings, &QToolButton::clicked, this, [this, settings] {
        QMenu menu(this);
        QAction* manage = menu.addAction("Manage custom presets");
        QAction* chosen = menu.exec(settings->mapToGlobal(QPoint(0, settings->height())));
        if (chosen == manage) {
            QMenu manageMenu(this);
            if (m_customPresets.isEmpty()) {
                QAction* empty = manageMenu.addAction("No custom presets saved");
                empty->setEnabled(false);
            } else {
                for (auto it = m_customPresets.cbegin(); it != m_customPresets.cend(); ++it) {
                    QAction* action = manageMenu.addAction("Delete  " + it.key());
                    connect(action, &QAction::triggered, this, [this, name = it.key()] {
                        removeCustomPreset(name);
                    });
                }
            }
            manageMenu.exec(settings->mapToGlobal(QPoint(0, settings->height())));
        }
    });

    loadCustomPresets();
    rebuildCustomPresetButtons();
    refreshDisplays();
}

void MainWindow::loadCustomPresets() {
    QSettings settings("DisplayBuddy", "DisplayBuddy");
    const QStringList names = settings.value("customPresetNames").toStringList();
    for (const QString& name : names)
        m_customPresets.insert(name, settings.value("customPresets/" + name, 70).toInt());
}

void MainWindow::rebuildCustomPresetButtons() {
    if (!m_customPresetLayout) return;
    while (m_customPresetLayout->count() > 0) {
        QLayoutItem* item = m_customPresetLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    for (auto it = m_customPresets.cbegin(); it != m_customPresets.cend(); ++it) {
        const QString name = it.key();
        const int value = it.value();
        auto* button = new QPushButton(name);
        button->setObjectName("presetButton");
        button->setMinimumHeight(32);
        button->setToolTip(QString("Apply %1% brightness").arg(value));
        m_customPresetLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, name, value] {
            applyPreset(name, value);
        });
    }
    m_customPresetLayout->addStretch();
}

void MainWindow::refreshDisplays() {
    const auto previous = m_displays;
    m_displays = BrightnessBackend::displays();
    buildDisplayCards();
    if (m_displays.isEmpty()) {
        m_count->setText("No controllable displays found");
        m_status->setText("Connect a display or check DDC/CI support. Refresh to scan again.");
    } else {
        m_count->setText(QString("Connected displays  ·  %1").arg(m_displays.size()));
        m_status->setText("Brightness changes automatically when you release a slider.");
    }
}

void MainWindow::buildDisplayCards() {
    while (m_listLayout->count() > 0) {
        QLayoutItem* item = m_listLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    m_widgets.clear();

    for (int i = 0; i < m_displays.size(); ++i) {
        const DisplayInfo display = m_displays[i];
        auto* card = new QFrame;
        card->setObjectName("displayCard");
        auto* layout = new QVBoxLayout(card);
        layout->setContentsMargins(14, 12, 14, 12);
        layout->setSpacing(9);

        auto* header = new QHBoxLayout;
        auto* monitorIcon = new GlyphLabel("▣");
        header->addWidget(monitorIcon);
        auto* name = new QLabel(display.name);
        name->setTextInteractionFlags(Qt::TextSelectableByMouse);
        name->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        header->addWidget(name, 1);

        auto* menuButton = new QToolButton;
        menuButton->setText("···");
        menuButton->setObjectName("toolButton");
        menuButton->setFixedSize(30, 28);
        menuButton->setToolTip("Display options");
        header->addWidget(menuButton);
        layout->addLayout(header);

        auto* brightnessRow = new QHBoxLayout;
        auto* sunIcon = new GlyphLabel("☼");
        brightnessRow->addWidget(sunIcon);
        auto* slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(display.brightness);
        slider->setEnabled(display.controllable);
        brightnessRow->addWidget(slider, 1);
        auto* value = new QLabel(display.controllable ? QString::number(display.brightness) + "%" : "—");
        value->setObjectName("brightnessValue");
        value->setMinimumWidth(38);
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        brightnessRow->addWidget(value);
        layout->addLayout(brightnessRow);

        auto* hint = new QLabel(display.controllable
            ? "Brightness"
            : "Detected, but brightness control is unavailable (check DDC/CI).");
        hint->setObjectName("muted");
        hint->setWordWrap(true);
        layout->addWidget(hint);

        DisplayWidgets widgets;
        widgets.id = display.id;
        widgets.value = value;
        widgets.slider = slider;
        widgets.status = hint;
        widgets.debounce = new QTimer(this);
        widgets.debounce->setSingleShot(true);
        widgets.debounce->setInterval(180);
        const int index = i;
        connect(slider, &QSlider::valueChanged, this, [value, widgetsRef = &m_widgets, index](int v) {
            value->setText(QString::number(v) + "%");
        });
        connect(slider, &QSlider::sliderReleased, this, [this, index, slider] {
            setDisplayBrightness(index, slider->value());
        });
        connect(menuButton, &QToolButton::clicked, this, [this, menuButton, display] {
            QMenu menu(this);
            QAction* refreshAction = menu.addAction("Refresh display list");
            QAction* copyName = menu.addAction("Copy display name");
            QAction* selected = menu.exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
            if (selected == refreshAction) refreshDisplays();
            if (selected == copyName) QApplication::clipboard()->setText(display.name);
        });

        m_widgets.push_back(widgets);
        m_listLayout->addWidget(card);
    }
    m_listLayout->addStretch();
}

void MainWindow::setDisplayBrightness(int index, int value) {
    if (index < 0 || index >= m_displays.size()) return;
    QString error;
    if (BrightnessBackend::setBrightness(m_displays[index].id, value, &error)) {
        m_status->setText(QString("%1 brightness set to %2%.").arg(m_displays[index].name).arg(value));
    } else {
        m_status->setText("Brightness could not be changed.");
        QMessageBox::warning(this, "DisplayBuddy", error);
    }
}

void MainWindow::applyPreset(const QString& name, int value) {
    if (m_displays.isEmpty()) {
        m_status->setText("No displays available for this preset.");
        return;
    }
    int success = 0;
    for (int i = 0; i < m_displays.size(); ++i) {
        if (!m_displays[i].controllable) continue;
        if (i < m_widgets.size() && m_widgets[i].slider) {
            m_widgets[i].slider->setValue(value);
        }
        QString error;
        if (BrightnessBackend::setBrightness(m_displays[i].id, value, &error))
            ++success;
    }
    m_status->setText(QString("%1 preset applied to %2 display(s).").arg(name).arg(success));
}

void MainWindow::saveCustomPreset() {
    if (m_displays.isEmpty()) {
        QMessageBox::information(this, "Custom preset", "Connect a controllable display first.");
        return;
    }

    bool ok = false;
    const QString name = QInputDialog::getText(
        this, "Save custom preset", "Preset name:", QLineEdit::Normal, QString(), &ok
    ).trimmed();
    if (!ok || name.isEmpty()) return;

    int value = 70;
    for (const auto& display : m_displays) {
        if (display.controllable) {
            value = display.brightness;
            break;
        }
    }

    // If the user has moved a slider but not released it, use that live value.
    for (const auto& widget : m_widgets) {
        if (widget.slider && widget.slider->isEnabled()) {
            value = widget.slider->value();
            break;
        }
    }

    m_customPresets.insert(name, value);
    QStringList names = m_customPresets.keys();
    QSettings settings("DisplayBuddy", "DisplayBuddy");
    settings.setValue("customPresetNames", names);
    settings.setValue("customPresets/" + name, value);

    rebuildCustomPresetButtons();
    m_status->setText(QString("Custom preset “%1” saved at %2%.").arg(name).arg(value));
}

void MainWindow::removeCustomPreset(const QString& name) {
    m_customPresets.remove(name);
    QSettings settings("DisplayBuddy", "DisplayBuddy");
    settings.remove("customPresets/" + name);
    settings.setValue("customPresetNames", m_customPresets.keys());
    rebuildCustomPresetButtons();
    m_status->setText("Custom preset removed.");
}
