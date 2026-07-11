#include "configmodel.h"

ConfigModel::ConfigModel(QObject *parent) : QObject{parent} {
    connect(m_configSource, &ConfigSource::srcValueChanged, this, &ConfigModel::onSrcValueChanged);
}

void ConfigModel::buildPropertyBinding() {
    if (!m_configSource) {
        return;
    }
    QList<QString> propNames = m_configSource->sortedValidPropNames();
    for (const auto &proName : propNames) {
        std::optional<QString> infoStr = m_configSource->propInfoByName(proName);
        if (!infoStr.has_value()) {
            continue;
        }
        std::string jsonStr = infoStr.value().trimmed().toStdString();
        if (nlohmann::json::accept(jsonStr)) {
            nlohmann::json propJson = nlohmann::json::parse(jsonStr);
            const auto property = createProperty(proName, propJson);
            if (property) {
                m_propertyList.append(property);
                m_nameToProperty.insert(proName, property);
                m_propertyToName.insert(property, proName);
                property->setValue(m_configSource->property(proName.toStdString().data()));
                property->setModified(false);
            }
        }
    }
}

void ConfigModel::updateSourceProperty(QtVariantProperty *property, const QVariant &value) const {
    if (!m_configSource) {
        return;
    }
    if (isValidItem(property, property->value())) {
        m_configSource->setProperty(m_propertyToName.value(property).toStdString().data(), property->value());
        property->setModified(false);
    }
}

QtVariantProperty *ConfigModel::createProperty(const QString &srcPropName, const nlohmann::json &propInfo) {
    if (!m_propertyManager) {
        return nullptr;
    }
    if (srcPropName.isEmpty()) {
        return nullptr;
    }
    if (!propInfo.is_object()) {
        return nullptr;
    }
    if (!propInfo.contains("uiName") || !propInfo["uiName"].is_string() ||
        propInfo["uiName"].get<std::string>().empty() || !propInfo.contains("type") || !propInfo["type"].is_string()) {
        return nullptr;
    }
    QString groupPath{""};
    if (propInfo.contains("groupPath") && propInfo["groupPath"].is_string()) {
        groupPath = QString::fromStdString(propInfo["groupPath"].get<std::string>()).trimmed();
    }
    const QString uiName = QString::fromStdString(propInfo["uiName"].get<std::string>()).trimmed();
    const QString type = QString::fromStdString(propInfo["type"].get<std::string>()).trimmed();
    // 根据类型创建绑定
    QtVariantProperty *property{nullptr};
    if (type == "string") {
        QRegularExpression regExp{};
        if (propInfo.contains("validator") && propInfo["validator"].is_object()) {
            const auto &validator = propInfo["validator"];
            if (validator.contains("regExp") && validator["regExp"].is_string()) {
                regExp.setPattern(QString::fromStdString(validator["regExp"].get<std::string>()).trimmed());
            }
        }
        const auto group = groupPropertyByPath(groupPath);
        property = m_propertyManager->addProperty(QMetaType::QString, uiName);
        property->setAttribute("groupPath", groupPath);
        if (regExp.isValid()) {
            property->setAttribute("regExp", regExp);
        }
        if (group) {
            group->addSubProperty(property);
        }
    } else if (type == "int") {
        const auto group = groupPropertyByPath(groupPath);
        property = m_propertyManager->addProperty(QMetaType::Int, uiName);
        property->setAttribute("groupPath", groupPath);
        int minVal{0};
        int maxVal{0};
        bool hasMin = false;
        bool hasMax = false;
        if (propInfo.contains("validator") && propInfo["validator"].is_object()) {
            const auto &validator = propInfo["validator"];
            hasMin = validator.contains("min") && validator["min"].is_number_integer();
            hasMax = validator.contains("max") && validator["max"].is_number_integer();

            if (hasMin)
                minVal = validator["min"].get<int>();
            if (hasMax)
                maxVal = validator["max"].get<int>();

            if (hasMin && hasMax && maxVal < minVal) {
                hasMin = false;
                hasMax = false;
            }
        }
        if (hasMin)
            property->setAttribute("minimum", minVal);
        if (hasMax)
            property->setAttribute("maximum", maxVal);
        property->setModified(false);
        if (group) {
            group->addSubProperty(property);
        }
    } else if (type == "enum") {
        QStringList enumNames;
        if (propInfo.contains("validator") && propInfo["validator"].is_object()) {
            const auto &validator = propInfo["validator"];
            if (validator.contains("enumNames") && validator["enumNames"].is_array()) {
                for (const auto &name : validator["enumNames"]) {
                    if (name.is_string()) {
                        const QString &trimedName = QString::fromStdString(name.get<std::string>()).trimmed();
                        if (!trimedName.isEmpty()) {
                            enumNames.append(trimedName);
                        }
                    }
                }
            }
        }
        if (!enumNames.empty()) {
            const auto group = groupPropertyByPath(groupPath);
            property = m_propertyManager->addProperty(QtVariantPropertyManager::enumTypeId(), uiName);
            property->setAttribute("groupPath", groupPath);
            property->setAttribute("enumNames", enumNames);
            property->setAttribute("minimum", 0);
            property->setAttribute("maximum", enumNames.size() - 1);
            if (group) {
                group->addSubProperty(property);
            }
        }
    }
    return property;
}

void ConfigModel::loadPropetyItems() {
    for (const auto &propName : m_propertyToName) {
        std::string pName = propName.toStdString();
        onSrcValueChanged(propName, m_configSource->property(pName.data()));
    }
}

void ConfigModel::onSrcValueChanged(const QString &propName, const QVariant &propValue) const {
    if (!m_configSource) {
        return;
    }
    if (!m_nameToProperty.contains(propName)) {
        return;
    }
    const auto property = m_nameToProperty.value(propName);
    if (isValidItem(property, propValue)) {
        if (property->value() != propValue) {
            property->setValue(propValue);
            property->setModified(false);
        }
    } else {
        if (isValidItem(property, property->value())) {
            m_configSource->setProperty(propName.toStdString().data(), property->value());
            property->setModified(false);
        } else {
            spdlog::critical(propName.toStdString() + "属性同步错误!");
        }
    }
}

void ConfigModel::savePropetyItems() {
    if (!m_configSource) {
        return;
    }
    for (const auto &property : m_nameToProperty) {
        if (isValidItem(property, property->value())) {
            m_configSource->setProperty(m_propertyToName.value(property).toStdString().data(), property->value());
            property->setModified(false);
        } else {
            if (isValidItem(property,
                            m_configSource->property(m_propertyToName.value(property).toStdString().data()))) {
                property->setValue(m_configSource->property(m_propertyToName.value(property).toStdString().data()));
                property->setModified(false);
            } else {
                spdlog::critical(m_propertyToName.value(property).toStdString() + "属性同步错误!");
            }
        }
    }
}

bool ConfigModel::equalValues(QtVariantProperty *property) const {
    if (!m_configSource) {
        return false;
    }
    if (!property || !m_propertyToName.contains(property)) {
        return false;
    }
    return property->value() == m_configSource->property(m_propertyToName.value(property).toStdString().data());
}

QtVariantProperty *ConfigModel::groupPropertyByPath(const QString &groupName) {
    if (!m_propertyManager) {
        return nullptr;
    }
    if (groupName.isEmpty()) {
        return nullptr;
    }

    static QHash<QString, QtVariantProperty *> cacheHash;

    QStringList parts = groupName.split('/', Qt::SkipEmptyParts);
    QtVariantProperty *parent = nullptr;
    QString path;

    for (const auto &part : parts) {
        path = parent ? path + "/" + part : part;
        if (auto *existing = cacheHash.value(path)) {
            parent = existing;
        } else {
            auto *group = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), part);
            group->setAttribute("groupPath", path);
            if (parent) {
                parent->addSubProperty(group);
            }
            m_propertyList.append(group);
            cacheHash[path] = group;
            parent = group;
        }
    }
    return parent;
}

bool ConfigModel::isValidItem(const QtVariantProperty *property, const QVariant &value) {
    if (!property || !value.isValid())
        return false;
    const auto propertyMetaType = property->valueType();
    if (value.typeId() != propertyMetaType) {
        return false;
    }
    switch (propertyMetaType) {
        case QMetaType::QString:
            {
                const auto regExpVar = property->attributeValue("regExp");
                if (!regExpVar.isValid() || regExpVar.typeId() != QMetaType::QString) {
                    return true;
                }
                const QRegularExpression regExp(regExpVar.toString());
                if (!regExp.isValid() || regExp.pattern().isEmpty())
                    return true;
                return regExp.match(value.toString()).hasMatch();
            }
        case QMetaType::Int:
            {
                const auto val = value.toInt();
                const auto minVar = property->attributeValue("minimum");
                const auto maxVar = property->attributeValue("maximum");
                bool ok = true;
                if (minVar.isValid() && minVar.typeId() == QMetaType::Int)
                    ok &= (val >= minVar.toInt());
                if (maxVar.isValid() && maxVar.typeId() == QMetaType::Int)
                    ok &= (val <= maxVar.toInt());
                return ok;
            }
        default:
            return false;
    }
}
