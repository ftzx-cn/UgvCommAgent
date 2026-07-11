#pragma once

#ifndef UGVCOMMAGENT_CONFIGMODEL_H
#define UGVCOMMAGENT_CONFIGMODEL_H

#include ".././3rdparty/qtpropertybrowser/qttreepropertybrowser_p.h"
#include ".././3rdparty/qtpropertybrowser/qtvariantproperty_p.h"

#include "../3rdparty/nlohmann/json.hpp"
#include <spdlog/spdlog.h>

#include "configsource.h"

#include <QLineEdit>
#include <QMap>
#include <QMetaType>
#include <QObject>
#include <QPointer>
#include <QRegularExpression>
#include <QTreeView>

class ConfigModel : public QObject {
    Q_OBJECT

  public:
    explicit ConfigModel(QObject *parent = nullptr);
    ~ConfigModel() override = default;

    void setPorpertyManager(QtVariantPropertyManager *propertyManger) { m_propertyManager = propertyManger; }
    void setConfigSource(ConfigSource *configSource) { m_configSource = configSource; }

    void buildPropertyBinding();

    void loadPropetyItems();
    void savePropetyItems();

    bool equalValues(QtVariantProperty *property) const;

    void onSrcValueChanged(const QString &propName, const QVariant &propValue) const;

    QList<QtVariantProperty *> properties() { return m_propertyList; }

    void updateSourceProperty(QtVariantProperty *property, const QVariant &value) const;

  signals:

  private:
    QtVariantProperty *createProperty(const QString &srcPropName, const nlohmann::json &propInfo);
    QtVariantProperty *groupPropertyByPath(const QString &groupName);
    static bool isValidItem(const QtVariantProperty *property, const QVariant &value);

    QtVariantPropertyManager *m_propertyManager{nullptr};
    ConfigSource *m_configSource{nullptr};

    QList<QtVariantProperty *> m_propertyList{};
    QHash<QString, QtVariantProperty *> m_nameToProperty{};
    QHash<QtVariantProperty *, QString> m_propertyToName{};
};
#endif // UGVCOMMAGENT_CONFIGMODEL_H
