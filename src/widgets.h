#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <QWidget>

// Slider that
//  - jumps to the clicked position,
//  - emits committed(value) when the mouse is released (no Apply button needed),
//  - also commits (debounced) after keyboard / wheel changes.
class ClickSlider : public QSlider {
    Q_OBJECT
public:
    explicit ClickSlider(QWidget* parent = nullptr) : QSlider(Qt::Horizontal, parent) {
        setRange(0, 100);
        setSingleStep(1);
        setPageStep(5);
        setCursor(Qt::PointingHandCursor);
        m_debounce.setSingleShot(true);
        m_debounce.setInterval(220);
        connect(&m_debounce, &QTimer::timeout, this, [this] { emit committed(value()); });
        connect(this, &QSlider::valueChanged, this, [this] {
            if (!m_pressed) m_debounce.start();
        });
    }

    // Change the value without emitting valueChanged/committed.
    void setValueQuiet(int v) {
        const QSignalBlocker blocker(this);
        setValue(v);
    }

signals:
    void committed(int value);

protected:
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() != Qt::LeftButton) { QSlider::mousePressEvent(e); return; }
        m_pressed = true;
        m_debounce.stop();
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        const QRect handle = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        if (handle.contains(e->position().toPoint())) {
            QSlider::mousePressEvent(e);   // normal drag
        } else {
            m_jumping = true;
            setValue(valueAt(e->position().toPoint().x(), handle.width()));
            e->accept();
        }
    }
    void mouseMoveEvent(QMouseEvent* e) override {
        if (m_jumping) {
            QStyleOptionSlider opt;
            initStyleOption(&opt);
            const QRect handle = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
            setValue(valueAt(e->position().toPoint().x(), handle.width()));
            e->accept();
        } else {
            QSlider::mouseMoveEvent(e);
        }
    }
    void mouseReleaseEvent(QMouseEvent* e) override {
        const bool wasPressed = m_pressed;
        if (!m_jumping) QSlider::mouseReleaseEvent(e);
        m_jumping = false;
        m_pressed = false;
        if (wasPressed && e->button() == Qt::LeftButton) emit committed(value());
    }

private:
    int valueAt(int x, int handleWidth) const {
        return QStyle::sliderValueFromPosition(minimum(), maximum(),
                                               x - handleWidth / 2, qMax(1, width() - handleWidth));
    }

    QTimer m_debounce;
    bool m_pressed = false;
    bool m_jumping = false;
};

// Clickable preset "card": name on top, brightness below.
class PresetCard : public QPushButton {
    Q_OBJECT
public:
    PresetCard(const QString& name, int value, bool custom, QWidget* parent = nullptr)
        : QPushButton(parent), m_name(name), m_value(value), m_custom(custom) {
        setObjectName(QStringLiteral("presetCard"));
        setCursor(Qt::PointingHandCursor);
        setToolTip(custom ? tr("%1 – %2% brightness (right-click to delete)").arg(name).arg(value)
                          : tr("%1 – %2% brightness").arg(name).arg(value));
        auto* lay = new QVBoxLayout(this);
        lay->setContentsMargins(14, 9, 14, 9);
        lay->setSpacing(1);
        auto* nameLabel = new QLabel(name);
        nameLabel->setObjectName(QStringLiteral("presetName"));
        auto* valueLabel = new QLabel(QStringLiteral("%1%").arg(value));
        valueLabel->setObjectName(QStringLiteral("presetValue"));
        for (QLabel* l : {nameLabel, valueLabel})
            l->setAttribute(Qt::WA_TransparentForMouseEvents);
        lay->addWidget(nameLabel);
        lay->addWidget(valueLabel);
    }

    QString presetName() const { return m_name; }
    int presetValue() const { return m_value; }
    bool isCustom() const { return m_custom; }

    void setActive(bool active) {
        if (property("active").toBool() == active) return;
        setProperty("active", active);
        style()->unpolish(this);
        style()->polish(this);
    }

    QSize sizeHint() const override {
        return layout()->sizeHint().expandedTo(QSize(104, 58)).boundedTo(QSize(220, 200));
    }
    QSize minimumSizeHint() const override { return sizeHint(); }

private:
    QString m_name;
    int m_value;
    bool m_custom;
};

// Lays child cards out in 1..N equal columns depending on the available width.
class ResponsiveGrid : public QWidget {
    Q_OBJECT
public:
    explicit ResponsiveGrid(QWidget* parent = nullptr) : QWidget(parent) {
        m_grid = new QGridLayout(this);
        m_grid->setContentsMargins(0, 0, 0, 0);
        m_grid->setSpacing(12);
    }

    void setMinItemWidth(int w) { m_minItemWidth = w; relayout(true); }

    void setItems(const QVector<QWidget*>& items) {
        clearItems();
        m_items = items;
        relayout(true);
    }

    void clearItems() {
        while (QLayoutItem* it = m_grid->takeAt(0)) delete it;
        for (QWidget* w : std::as_const(m_items)) {
            w->hide();
            w->deleteLater();
        }
        m_items.clear();
    }

protected:
    void resizeEvent(QResizeEvent* e) override {
        QWidget::resizeEvent(e);
        relayout(false);
    }

private:
    void relayout(bool force) {
        const int spacing = m_grid->spacing();
        const int cols = qMax(1, (width() + spacing) / (m_minItemWidth + spacing));
        if (!force && cols == m_cols) return;
        m_cols = cols;

        while (QLayoutItem* it = m_grid->takeAt(0)) delete it;
        for (int c = 0; c < 8; ++c) m_grid->setColumnStretch(c, 0);
        for (int r = 0; r < 64; ++r) m_grid->setRowStretch(r, 0);

        int row = 0, col = 0;
        for (QWidget* w : std::as_const(m_items)) {
            m_grid->addWidget(w, row, col);
            w->show();
            if (++col == cols) { col = 0; ++row; }
        }
        for (int c = 0; c < cols; ++c) m_grid->setColumnStretch(c, 1);
        m_grid->setRowStretch(col == 0 ? row : row + 1, 1);   // push cards to the top
    }

    QGridLayout* m_grid = nullptr;
    QVector<QWidget*> m_items;
    int m_minItemWidth = 360;
    int m_cols = 0;
};
