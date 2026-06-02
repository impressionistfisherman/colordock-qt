#pragma once
#include <QSettings>
#include <QVariant>

class AppSettings {
public:
    static AppSettings& instance() {
        static AppSettings inst;
        return inst;
    }
    QVariant get(const QString &key, const QVariant &def = {}) const {
        return m_s.value(key, def);
    }
    void set(const QString &key, const QVariant &val) {
        m_s.setValue(key, val);
        m_s.sync();
    }
private:
    AppSettings() : m_s("ColorDock", "ColorDock") {}
    QSettings m_s;
};
