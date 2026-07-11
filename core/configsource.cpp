#include "configsource.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaProperty>
#include <QRegularExpression>

#include "spdlog/spdlog.h"

ConfigSource::ConfigSource(QObject *parent) : QObject(parent) {
    m_nameToInfoHash.insert("plcIp", m_plcIpInfo);
    m_nameToInfoHash.insert("plcPort", m_plcPortInfo);
    m_nameToInfoHash.insert("plcSlaveId", m_plcSlaveIdInfo);
    m_nameToInfoHash.insert("plcResponseTimeoutMs", m_plcResponseTimeoutMsInfo);
    // m_nameToInfoHash.insert("plcByteTimeoutMs", m_plcByteTimeoutMsInfo);
    m_nameToInfoHash.insert("plcConnectRetryCount", m_plcConnectRetryCountInfo);
    m_nameToInfoHash.insert("plcConnectRetryIntervalMs", m_plcConnectRetryIntervalInfo);
    m_nameToInfoHash.insert("plcReadRetryCount", m_plcReadRetryCountInfo);
    m_nameToInfoHash.insert("plcReadRetryIntervalMs", m_plcReadRetryIntervalMsInfo);
    m_nameToInfoHash.insert("plcPollingIntervalMs", m_plcPollingIntervalMsInfo);
    m_nameToInfoHash.insert("plcEndianMode", m_plcEndianModeInfo);
    m_nameToInfoHash.insert("deviceRemoteModeFuncCode", m_deviceRemoteModeFuncCodeInfo);
    m_nameToInfoHash.insert("deviceRemoteModeAddress", m_deviceRemoteModeAddressInfo);
    m_nameToInfoHash.insert("devicePumpStatusFuncCode", m_devicePumpStatusFuncCodeInfo);
    m_nameToInfoHash.insert("devicePumpStatusAddress", m_devicePumpStatusAddressInfo);
    m_nameToInfoHash.insert("deviceBarrelLevelFuncCode", m_deviceBarrelLevelFuncCodeInfo);
    m_nameToInfoHash.insert("deviceBarrelLevelAddress", m_deviceBarrelLevelAddressInfo);
    m_nameToInfoHash.insert("devicePumpCtrlFuncCode", m_devicePumpCtrlFuncCodeInfo);
    m_nameToInfoHash.insert("devicePumpCtrlAddress", m_devicePumpCtrlAddressInfo);
    m_nameToInfoHash.insert("deviceAiScaleRange", m_deviceAiScaleRangeInfo);
    m_nameToInfoHash.insert("deviceTankLevelRange", m_deviceTankLevelRangeInfo);
    m_nameToInfoHash.insert("wsServerUrl", m_wsServerUrlInfo);
}

void ConfigSource::sortValidProperty() {
    m_sortedValidPropNames.clear();
    const QMetaObject *mo = metaObject();
    QHash<QString, int> nameToId;
    const int propCount = mo->propertyCount() - mo->propertyOffset();
    nameToId.reserve(propCount);
    m_sortedValidPropNames.reserve(propCount);
    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        const QMetaProperty prop = mo->property(i);
        const QString propName = prop.name();
        auto it = m_nameToInfoHash.constFind(propName);
        if (it == m_nameToInfoHash.constEnd()) {
            continue;
        }
        const QString &jsonStr = it.value();
        if (jsonStr.isEmpty()) {
            continue;
        }
        try {
            nlohmann::json propJson = nlohmann::json::parse(jsonStr.toStdString());
            if (propJson.contains("id") && propJson["id"].is_number_integer() && propJson.contains("uiName") &&
                propJson["uiName"].is_string() && propJson.contains("type") && propJson["type"].is_string()) {
                QString uiNameStr = QString::fromStdString(propJson["uiName"].get<std::string>()).trimmed();
                QString typeStr = QString::fromStdString(propJson["type"].get<std::string>()).trimmed();
                if (uiNameStr.isEmpty() || typeStr.isEmpty()) {
                    continue;
                }
                if ((prop.typeId() == QMetaType::QString && propJson["type"] == "string") ||
                    (prop.typeId() == QMetaType::Int && propJson["type"] == "int") ||
                    (prop.typeId() == QMetaType::Int && propJson["type"] == "enum")) {
                    int id = propJson["id"].get<int>();
                    nameToId.insert(propName, id);
                    m_sortedValidPropNames.append(propName);
                }
            }
        } catch (const nlohmann::json::exception &e) {
            spdlog::critical("JSON parse error for " + propName.toStdString() + " : " + e.what());
        }
    }
    std::sort(m_sortedValidPropNames.begin(), m_sortedValidPropNames.end(),
              [&nameToId](const QString &a, const QString &b) { return nameToId[a] < nameToId[b]; });
}

void ConfigSource::loadFromFile(const QString &filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        try {
            const QMetaObject *mo = metaObject();
            if (nlohmann::json::accept(data.toStdString())) {
                nlohmann::json root = nlohmann::json::parse(data.toStdString());
                for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
                    const QString propName = mo->property(i).name();
                    auto it = m_nameToInfoHash.constFind(propName);
                    if (it == m_nameToInfoHash.constEnd()) {
                        continue;
                    }
                    const QString &jsonStr = it.value();
                    if (jsonStr.isEmpty()) {
                        continue;
                    }
                    nlohmann::json defaultJson = nlohmann::json::parse(jsonStr.toStdString());
                    if (root.contains(propName.toStdString()) && root[propName.toStdString()].is_object()) {
                        nlohmann::json &propRoot = root[propName.toStdString()];
                        if (propRoot.contains("id") && propRoot["id"].is_number_integer()) {
                            defaultJson["id"] = propRoot["id"];
                        }
                        if (propRoot.contains("groupPath") && propRoot["groupPath"].is_string()) {
                            defaultJson["groupPath"] = QString::fromStdString(propRoot["groupPath"].get<std::string>())
                                                           .trimmed()
                                                           .toStdString();
                        }
                        if (propRoot.contains("uiName") && propRoot["uiName"].is_string()) {
                            QString nameStr = QString::fromStdString(propRoot["uiName"].get<std::string>()).trimmed();
                            if (!nameStr.isEmpty()) {
                                defaultJson["uiName"] = nameStr.toStdString();
                            }
                        }
                        if (propRoot.contains("type") && propRoot["type"].is_string()) {
                            QString tempStr = QString::fromStdString(propRoot["type"].get<std::string>());
                            const std::string &typeStr = tempStr.trimmed().toStdString();
                            int typeId = property(propName.toStdString().data()).typeId();
                            bool typeEqualed{false};
                            switch (typeId) {
                                case QMetaType::QString:
                                    if (typeStr == "string") {
                                        typeEqualed = true;
                                    }
                                    break;
                                case QMetaType::Int:
                                    if (typeStr == "int" || typeStr == "enum") {
                                        typeEqualed = true;
                                    }
                                    break;
                                default:
                                    break;
                            }
                            if (typeEqualed) {
                                defaultJson["type"] = typeStr;
                            } else {
                                continue;
                            }
                        }
                        if (propRoot.contains("value")) {
                            nlohmann::json::value_t jsonValueType = propRoot["value"].type();
                            const std::string &typeStr =
                                QString::fromStdString(propRoot["type"].get<std::string>()).trimmed().toStdString();
                            if (typeStr == "string" && jsonValueType == nlohmann::json::value_t::string) {
                                bool isRegexpValidator{false};
                                if (propRoot.contains("validator") && propRoot["validator"].is_object()) {
                                    nlohmann::json &validator = propRoot["validator"];
                                    if (validator.contains("regExp") && validator["regExp"].is_string()) {
                                        const QString &pattern =
                                            QString::fromStdString(validator["regExp"].get<std::string>()).trimmed();
                                        // 静态缓存正则，仅首次编译
                                        static QRegularExpression regExp;
                                        // 仅当正则文本变化时才重新赋值编译
                                        if (regExp.pattern() != pattern) {
                                            regExp.setPattern(pattern);
                                        }
                                        if (regExp.isValid()) {
                                            if (regExp
                                                    .match(QString::fromStdString(propRoot["value"].get<std::string>()))
                                                    .hasMatch()) {
                                                defaultJson["value"] = propRoot["value"];
                                                defaultJson["validator"]["regExp"] = pattern.toStdString();
                                            }
                                        }
                                    }
                                }
                                if (!isRegexpValidator) {
                                    defaultJson["value"] = propRoot["value"];
                                    if (defaultJson["validator"].is_object()) {
                                        defaultJson.erase("validator");
                                    }
                                }
                            }
                            if (typeStr == "int" && (jsonValueType == nlohmann::json::value_t::number_integer ||
                                                     jsonValueType == nlohmann::json::value_t::number_unsigned)) {
                                if (propRoot.contains("validator") && propRoot["validator"].is_object()) {
                                    nlohmann::json &validator = propRoot["validator"];
                                    int minVal{0};
                                    int maxVal{0};
                                    bool hasMin = false;
                                    bool hasMax = false;
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
                                    int value{propRoot["value"].get<int>()};
                                    if (hasMin) {
                                        if (hasMax) {
                                            if (value <= maxVal && value >= minVal) {
                                                defaultJson["value"] = value;
                                                defaultJson["validator"]["min"] = minVal;
                                                defaultJson["validator"]["max"] = maxVal;
                                            }
                                        } else {
                                            if (value >= minVal) {
                                                defaultJson["value"] = value;
                                                defaultJson["validator"]["min"] = minVal;
                                            }
                                        }
                                    } else {
                                        if (hasMax) {
                                            if (value <= maxVal) {
                                                defaultJson["value"] = value;
                                                defaultJson["validator"]["max"] = maxVal;
                                            }
                                        } else {
                                            defaultJson["value"] = propRoot["value"];
                                        }
                                    }
                                } else {
                                    defaultJson["value"] = propRoot["value"];
                                    if (defaultJson["validator"].is_object()) {
                                        defaultJson.erase("validator");
                                    }
                                }
                            }
                            if (typeStr == "enum" && (jsonValueType == nlohmann::json::value_t::number_integer ||
                                                      jsonValueType == nlohmann::json::value_t::number_unsigned)) {
                                if (propRoot.contains("validator") && propRoot["validator"].is_object()) {
                                    nlohmann::json &validator = propRoot["validator"];
                                    if (validator.contains("enumNames") && validator["enumNames"].is_array()) {
                                        QList<std::string> enumNames{};
                                        for (const auto &name : validator["enumNames"]) {
                                            if (name.is_string()) {
                                                const QString &trimedName =
                                                    QString::fromStdString(name.get<std::string>()).trimmed();
                                                if (!trimedName.isEmpty()) {
                                                    enumNames.append(trimedName.toStdString());
                                                }
                                            }
                                        }
                                        if (!enumNames.isEmpty()) {
                                            if (propRoot["value"].get<int>() <= enumNames.size()) {
                                                defaultJson["validator"]["enumNames"].clear();
                                                for (const auto &name : enumNames) {
                                                    defaultJson["validator"]["enumNames"].push_back(name);
                                                }
                                                defaultJson["value"] = propRoot["value"].get<int>();
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        std::string newJson = defaultJson.dump();
                        m_nameToInfoHash[propName] = QString::fromStdString(newJson);
                    }
                }
            }
        } catch (const nlohmann::json::exception &e) {
            spdlog::critical(static_cast<std::string>("JSON parse error for : ") + e.what());
        }
    }
    const QMetaObject *mo = metaObject();
    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        const QMetaProperty prop = mo->property(i);
        const QString propName = mo->property(i).name();
        auto it = m_nameToInfoHash.constFind(propName);
        if (it == m_nameToInfoHash.constEnd()) {
            continue;
        }
        const QString &jsonStr = it.value();
        if (jsonStr.isEmpty()) {
            continue;
        }
        try {
            nlohmann::json propInfoJson = nlohmann::json::parse(jsonStr.toStdString());
            if (propInfoJson.contains("value")) {
                if (prop.typeId() == QMetaType::QString && propInfoJson["value"].is_string()) {
                    setProperty(propName.toStdString().data(),
                                QString::fromStdString(propInfoJson["value"].get<std::string>()));
                }
                if (prop.typeId() == QMetaType::Int && propInfoJson["value"].is_number_integer()) {
                    setProperty(propName.toStdString().data(), propInfoJson["value"].get<int>());
                }
            }

        } catch (const nlohmann::json::exception &e) {
            qWarning() << e.what();
            spdlog::critical(static_cast<std::string>("JSON parse error for : ") + e.what());
        }
    }
    sortValidProperty();
}

void ConfigSource::updatePropInfoValue(const QString &propName, const QVariant &propValue) {
    const auto it = m_nameToInfoHash.constFind(propName);
    if (it == m_nameToInfoHash.constEnd()) {
        return;
    }
    const QString &jsonStr = it.value();
    if (jsonStr.isEmpty()) {
        return;
    }
    if (!nlohmann::json::accept(jsonStr.toStdString())) {
        return;
    }
    nlohmann::json propJson = nlohmann::json::parse(jsonStr.toStdString());
    if (propJson.contains("value")) {
        if (propValue.typeId() == QMetaType::QString && propJson["value"].is_string()) {
            propJson["value"] = propValue.toString().toStdString();
        }
        if (propValue.typeId() == QMetaType::Int && propJson["value"].is_number_integer()) {
            propJson["value"] = propValue.toInt();
        }
    }
    const std::string newJson = propJson.dump();
    m_nameToInfoHash[propName] = QString::fromStdString(newJson);
}

QList<QString> ConfigSource::sortedValidPropNames() {
    sortValidProperty();
    return m_sortedValidPropNames;
}

std::optional<QString> ConfigSource::propInfoByName(const QString &propName) const {
    if (m_nameToInfoHash.contains(propName)) {
        return  std::make_optional(m_nameToInfoHash.value(propName));
    }
    return std::nullopt;
}

void ConfigSource::saveToFile(const QString &filePath) const {
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    nlohmann::json root;
    const QMetaObject *mo = metaObject();
    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        const QMetaProperty prop = mo->property(i);
        const QString propName = prop.name();
        auto it = m_nameToInfoHash.constFind(propName);
        if (it == m_nameToInfoHash.constEnd()) {
            continue;
        }
        const QString &jsonStr = it.value();
        if (jsonStr.isEmpty()) {
            continue;
        }
        try {
            const nlohmann::json propInfoJson = nlohmann::json::parse(jsonStr.toStdString());
            root[propName.toStdString()] = propInfoJson;
        } catch (const nlohmann::json::exception &e) {
            spdlog::critical(static_cast<std::string>("JSON parse error for : ") + e.what());
        }
    }
    const std::string jsonStr = root.dump(4);
    if (file.write(jsonStr.data(), static_cast<qint64>(jsonStr.size())) != jsonStr.size()) {
        return;
    }
    if (file.commit()) {
        spdlog::info("所有设置已成功保存到文件!");
    } else {
        spdlog::warn("保存设置文件失败!");
    }
}

// ---------- Setter 实现（每个 setter
// 都检查变化，若变化则更新并发出信号）----------
void ConfigSource::setPlcIp(const QString &ip) {
    if (m_plcIp == ip) {
        return;
    }
    m_plcIp = ip;
    updatePropInfoValue("plcIp", ip);
    emit srcValueChanged("plcIp", ip);
}

void ConfigSource::setPlcPort(const int port) {
    if (m_plcPort == port) {
        return;
    }
    m_plcPort = port;
    updatePropInfoValue("plcPort", port);
    emit srcValueChanged("plcPort", port);
}

void ConfigSource::setPlcSlaveId(const int id) {
    if (m_plcSlaveId == id) {
        return;
    }
    m_plcSlaveId = id;
    updatePropInfoValue("plcSlaveId", id);
    emit srcValueChanged("plcSlaveId", id);
}

void ConfigSource::setPlcResponseTimeoutMs(const int ms) {
    if (m_plcResponseTimeoutMs == ms) {
        return;
    }
    m_plcResponseTimeoutMs = ms;
    updatePropInfoValue("plcResponseTimeoutMs", ms);
    emit srcValueChanged("plcResponseTimeoutMs", ms);
}

// void ConfigSource::setPlcByteTimeoutMs(const int ms) {
//     if (m_plcByteTimeoutMs == ms) {
//         return;
//     }
//     m_plcByteTimeoutMs = ms;
//     updatePropInfoValue("plcByteTimeoutMs", ms);
//     emit srcValueChanged("plcByteTimeoutMs", ms);
// }

void ConfigSource::setPlcConnectRetryCount(const int count) {
    if (m_plcConnectRetryCount == count) {
        return;
    }
    m_plcConnectRetryCount = count;
    updatePropInfoValue("plcConnectRetryCount", count);
    emit srcValueChanged("plcConnectRetryCount", count);
}

void ConfigSource::setPlcConnectRetryIntervalMs(const int ms) {
    if (m_plcConnectRetryIntervalMs == ms) {
        return;
    }
    m_plcConnectRetryIntervalMs = ms;
    updatePropInfoValue("plcConnectRetryIntervalMs", ms);
    emit srcValueChanged("plcConnectRetryIntervalMs", ms);
}

void ConfigSource::setPlcReadRetryCount(const int count) {
    if (m_plcReadRetryCount == count) {
        return;
    }
    m_plcReadRetryCount = count;
    updatePropInfoValue("plcReadRetryCount", count);
    emit srcValueChanged("plcReadRetryCount", count);
}

void ConfigSource::setPlcReadRetryIntervalMs(const int ms) {
    if (m_plcReadRetryIntervalMs == ms) {
        return;
    }
    m_plcReadRetryIntervalMs = ms;
    updatePropInfoValue("plcReadRetryIntervalMs", ms);
    emit srcValueChanged("plcReadRetryIntervalMs", ms);
}

void ConfigSource::setPlcPollingIntervalMs(const int ms) {
    if (m_plcPollingIntervalMs == ms) {
        return;
    }
    m_plcPollingIntervalMs = ms;
    updatePropInfoValue("plcPollingIntervalMs", ms);
    emit srcValueChanged("plcPollingIntervalMs", ms);
}

void ConfigSource::setPlcEndianMode(const int mode) {
    if (m_plcEndianMode == mode) {
        return;
    }
    m_plcEndianMode = mode;
    updatePropInfoValue("plcEndianMode", mode);
    emit srcValueChanged("plcEndianMode", mode);
}

void ConfigSource::setDeviceRemoteModeFuncCode(const int code) {
    if (m_deviceRemoteModeFuncCode == code) {
        return;
    }
    m_deviceRemoteModeFuncCode = code;
    updatePropInfoValue("deviceRemoteModeFuncCode", code);
    emit srcValueChanged("deviceRemoteModeFuncCode", code);
}

void ConfigSource::setDeviceRemoteModeAddress(const int addr) {
    if (m_deviceRemoteModeAddress == addr) {
        return;
    }
    m_deviceRemoteModeAddress = addr;
    updatePropInfoValue("deviceRemoteModeAddress", addr);
    emit srcValueChanged("deviceRemoteModeAddress", addr);
}

void ConfigSource::setDevicePumpStatusFuncCode(const int code) {
    if (m_devicePumpStatusFuncCode == code) {
        return;
    }
    m_devicePumpStatusFuncCode = code;
    updatePropInfoValue("devicePumpStatusFuncCode", code);
    emit srcValueChanged("devicePumpStatusFuncCode", code);
}

void ConfigSource::setDevicePumpStatusAddress(const int addr) {
    if (m_devicePumpStatusAddress == addr) {
        return;
    }
    m_devicePumpStatusAddress = addr;
    updatePropInfoValue("devicePumpStatusAddress", addr);
    emit srcValueChanged("devicePumpStatusAddress", addr);
}

void ConfigSource::setDeviceBarrelLevelFuncCode(const int code) {
    if (m_deviceBarrelLevelFuncCode == code) {
        return;
    }
    m_deviceBarrelLevelFuncCode = code;
    updatePropInfoValue("deviceBarrelLevelFuncCode", code);
    emit srcValueChanged("deviceBarrelLevelFuncCode", code);
}

void ConfigSource::setDeviceBarrelLevelAddress(const int addr) {
    if (m_deviceBarrelLevelAddress == addr) {
        return;
    }
    m_deviceBarrelLevelAddress = addr;
    updatePropInfoValue("deviceBarrelLevelAddress", addr);
    emit srcValueChanged("deviceBarrelLevelAddress", addr);
}

void ConfigSource::setDevicePumpCtrlFuncCode(const int code) {
    if (m_devicePumpCtrlFuncCode == code) {
        return;
    }
    m_devicePumpCtrlFuncCode = code;
    updatePropInfoValue("devicePumpCtrlFuncCode", code);
    emit srcValueChanged("devicePumpCtrlFuncCode", code);
}

void ConfigSource::setDevicePumpCtrlAddress(const int addr) {
    if (m_devicePumpCtrlAddress == addr) {
        return;
    }
    m_devicePumpCtrlAddress = addr;
    updatePropInfoValue("devicePumpCtrlAddress", addr);
    emit srcValueChanged("devicePumpCtrlAddress", addr);
}

void ConfigSource::setDeviceAiScaleRange(const QString &range) {
    if (m_deviceAiScaleRange == range) {
        return;
    }
    QStringList parts = range.trimmed().split(':', Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        emit srcValueChanged("deviceAiScaleRange", m_deviceAiScaleRange);
        return;
    }
    bool ok1, ok2;
    const int out1 = parts[0].trimmed().toInt(&ok1);
    const int out2 = parts[1].trimmed().toInt(&ok2);
    if (!ok1 || !ok2 || out1 > out2) {
        emit srcValueChanged("deviceAiScaleRange", m_deviceAiScaleRange);
        return;
    }
    const QString newRangeStr = QString("%1:%2").arg(out1).arg(out2);
    m_deviceAiScaleRange = newRangeStr;
    updatePropInfoValue("deviceAiScaleRange", newRangeStr);
    emit srcValueChanged("deviceAiScaleRange", newRangeStr);
}

void ConfigSource::setDeviceTankLevelRange(const QString &range) {
    if (m_deviceTankLevelRange == range) {
        return;
    }
    QStringList parts = range.trimmed().split(':', Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        emit srcValueChanged("deviceTankLevelRange", m_deviceTankLevelRange);
        return;
    }
    bool ok1, ok2;
    const double out1 = parts[0].trimmed().toDouble(&ok1);
    const double out2 = parts[1].trimmed().toDouble(&ok2);
    if (!ok1 || !ok2 || out1 > out2) {
        emit srcValueChanged("deviceTankLevelRange", m_deviceTankLevelRange);
        return;
    }
    const QString newRangeStr = QString("%1:%2").arg(out1, 0, 'f', 2).arg(out2, 0, 'f', 2);
    m_deviceTankLevelRange = newRangeStr;
    updatePropInfoValue("deviceTankLevelRange", newRangeStr);
    emit srcValueChanged("deviceTankLevelRange", newRangeStr);
}
void ConfigSource::setWsServerUrl(const QString &url) {
    if (m_wsServerUrl == url) return;
    m_wsServerUrl = url;
    updatePropInfoValue("wsServerUrl", url);
    emit srcValueChanged("wsServerUrl", url);
}
