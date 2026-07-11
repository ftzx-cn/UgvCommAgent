#pragma once

#ifndef UGVCOMMAGENT_CONFIGSOURCE_H
#define UGVCOMMAGENT_CONFIGSOURCE_H

#include "../3rdparty/nlohmann/json.hpp"

#include <QFile>
#include <QJsonObject>
#include <QMetaObject>
#include <QMetaType>
#include <QObject>
#include <QSaveFile>
#include <regex>

class ConfigSource : public QObject {
    Q_OBJECT
    // ---------- PLC 配置属性 ----------
    Q_PROPERTY(QString plcIp MEMBER m_plcIp WRITE setPlcIp NOTIFY srcValueChanged)
    Q_PROPERTY(int plcPort MEMBER m_plcPort WRITE setPlcPort NOTIFY srcValueChanged)
    Q_PROPERTY(int plcSlaveId MEMBER m_plcSlaveId WRITE setPlcSlaveId NOTIFY srcValueChanged)
    Q_PROPERTY(
        int plcResponseTimeoutMs MEMBER m_plcResponseTimeoutMs WRITE setPlcResponseTimeoutMs NOTIFY srcValueChanged)
    // Q_PROPERTY(int plcByteTimeoutMs MEMBER m_plcByteTimeoutMs WRITE setPlcByteTimeoutMs NOTIFY srcValueChanged)
    Q_PROPERTY(
        int plcConnectRetryCount MEMBER m_plcConnectRetryCount WRITE setPlcConnectRetryCount NOTIFY srcValueChanged)
    Q_PROPERTY(int plcConnectRetryIntervalMs MEMBER m_plcConnectRetryIntervalMs WRITE setPlcConnectRetryIntervalMs
                   NOTIFY srcValueChanged)
    Q_PROPERTY(int plcReadRetryCount MEMBER m_plcReadRetryCount WRITE setPlcReadRetryCount NOTIFY srcValueChanged)
    Q_PROPERTY(int plcReadRetryIntervalMs MEMBER m_plcReadRetryIntervalMs WRITE setPlcReadRetryIntervalMs NOTIFY
                   srcValueChanged)
    Q_PROPERTY(
        int plcPollingIntervalMs MEMBER m_plcPollingIntervalMs WRITE setPlcPollingIntervalMs NOTIFY srcValueChanged)
    Q_PROPERTY(int plcEndianMode MEMBER m_plcEndianMode WRITE setPlcEndianMode NOTIFY srcValueChanged)
    // ---------- 设备配置属性 ----------
    Q_PROPERTY(int deviceRemoteModeFuncCode MEMBER m_deviceRemoteModeFuncCode WRITE setDeviceRemoteModeFuncCode NOTIFY
                   srcValueChanged)
    Q_PROPERTY(int deviceRemoteModeAddress MEMBER m_deviceRemoteModeAddress WRITE setDeviceRemoteModeAddress NOTIFY
                   srcValueChanged)
    Q_PROPERTY(int devicePumpStatusFuncCode MEMBER m_devicePumpStatusFuncCode WRITE setDevicePumpStatusFuncCode NOTIFY
                   srcValueChanged)
    Q_PROPERTY(int devicePumpStatusAddress MEMBER m_devicePumpStatusAddress WRITE setDevicePumpStatusAddress NOTIFY
                   srcValueChanged)
    Q_PROPERTY(int deviceBarrelLevelFuncCode MEMBER m_deviceBarrelLevelFuncCode WRITE setDeviceBarrelLevelFuncCode
                   NOTIFY srcValueChanged)
    Q_PROPERTY(int deviceBarrelLevelAddress MEMBER m_deviceBarrelLevelAddress WRITE setDeviceBarrelLevelAddress NOTIFY
                   srcValueChanged)
    Q_PROPERTY(int devicePumpCtrlFuncCode MEMBER m_devicePumpCtrlFuncCode WRITE setDevicePumpCtrlFuncCode NOTIFY
                   srcValueChanged)
    Q_PROPERTY(
        int devicePumpCtrlAddress MEMBER m_devicePumpCtrlAddress WRITE setDevicePumpCtrlAddress NOTIFY srcValueChanged)
    Q_PROPERTY(
        QString deviceAiScaleRange MEMBER m_deviceAiScaleRange WRITE setDeviceAiScaleRange NOTIFY srcValueChanged)
    Q_PROPERTY(
        QString deviceTankLevelRange MEMBER m_deviceTankLevelRange WRITE setDeviceTankLevelRange NOTIFY srcValueChanged)
    Q_PROPERTY(QString wsServerUrl MEMBER m_wsServerUrl WRITE setWsServerUrl NOTIFY srcValueChanged)

  public:
    explicit ConfigSource(QObject *parent = nullptr);

    QList<QString> sortedValidPropNames();
    [[nodiscard]] std::optional<QString> propInfoByName(const QString &propName) const;
    void saveToFile(const QString &filePath = "config.json") const;
    void loadFromFile(const QString &filePath);

    void setPlcIp(const QString &ip);
    void setPlcPort(int port);
    void setPlcSlaveId(int id);
    void setPlcResponseTimeoutMs(int ms);
    // void setPlcByteTimeoutMs(int ms);
    void setPlcConnectRetryCount(int count);
    void setPlcConnectRetryIntervalMs(int ms);
    void setPlcReadRetryCount(int count);
    void setPlcReadRetryIntervalMs(int ms);
    void setPlcPollingIntervalMs(int ms);
    void setPlcEndianMode(int mode);

    void setDeviceRemoteModeFuncCode(int code);
    void setDeviceRemoteModeAddress(int addr);
    void setDevicePumpStatusFuncCode(int code);
    void setDevicePumpStatusAddress(int addr);
    void setDeviceBarrelLevelFuncCode(int code);
    void setDeviceBarrelLevelAddress(int addr);
    void setDevicePumpCtrlFuncCode(int code);
    void setDevicePumpCtrlAddress(int addr);
    void setDeviceAiScaleRange(const QString &range);
    void setDeviceTankLevelRange(const QString &range);
    void setWsServerUrl(const QString &url);

  signals:
    void srcValueChanged(const QString &proName, const QVariant &proValue); // 任意属性变化时发出

  private:
    void sortValidProperty();

    // PLC参数
    QString m_plcIp{};
    int m_plcPort{};
    int m_plcSlaveId{};
    int m_plcResponseTimeoutMs{};
    int m_plcByteTimeoutMs{};
    int m_plcConnectRetryCount{};
    int m_plcConnectRetryIntervalMs{};
    int m_plcReadRetryCount{};
    int m_plcReadRetryIntervalMs{};
    int m_plcPollingIntervalMs{};
    int m_plcEndianMode{};

    QString m_plcIpInfo{
        R"({"id": 1, "groupPath": "PLC连接配置", "uiName": "IP地址", "type": "string", "value": "127.0.0.1", "validator": {"regExp":"^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"}})"};
    QString m_plcPortInfo{
        R"({"id": 2, "groupPath": "PLC连接配置", "uiName": "TCP端口", "type": "int", "value": 502, "validator": {"min": 0, "max": 65535}})"};
    QString m_plcSlaveIdInfo{
        R"({"id": 3, "groupPath": "PLC连接配置", "uiName": "从站地址", "type": "int", "value": 1, "validator": {"min": 1, "max": 247}})"};
    QString m_plcResponseTimeoutMsInfo{
        R"({"id": 4, "groupPath": "PLC连接配置", "uiName": "报文响应超时", "type": "int", "value": 1000, "validator": {"min": 1}})"};
    // QString m_plcByteTimeoutMsInfo{
    //     R"({"id": 5, "groupPath": "PLC连接配置", "uiName": "字节间隔超时", "type": "int", "value": 1000, "validator":
    //     {"min": 1}})"};
    QString m_plcConnectRetryCountInfo{
        R"({"id": 6, "groupPath": "PLC连接配置", "uiName": "连接重试次数", "type": "int", "value": 3, "validator": {"min": 1}})"};
    QString m_plcConnectRetryIntervalInfo{
        R"({"id": 7, "groupPath": "PLC连接配置", "uiName": "失败重连间隔", "type": "int", "value": 1000, "validator": {"min": 1}})"};
    QString m_plcReadRetryCountInfo{
        R"({"id": 8, "groupPath": "PLC连接配置", "uiName": "读寄存器失败重试次数", "type": "int", "value": 3, "validator": {"min": 1}})"};
    QString m_plcReadRetryIntervalMsInfo{
        R"({"id": 9, "groupPath": "PLC连接配置", "uiName": "读寄存器失败重试间隔", "type": "int", "value": 50, "validator": {"min": 1}})"};
    QString m_plcPollingIntervalMsInfo{
        R"({"id": 10, "groupPath": "PLC连接配置", "uiName": "数据轮询间隔", "type": "int", "value": 50, "validator": {"min": 1}})"};
    QString m_plcEndianModeInfo{
        R"({"id": 11, "groupPath": "PLC连接配置", "uiName": "大小端模式", "type": "enum", "value": 0, "validator": {"enumNames": ["ABCD", "CDAB", "BADC", "DCBA"]}})"};
    //   点位参数
    int m_deviceRemoteModeFuncCode{};
    int m_deviceRemoteModeAddress{};
    int m_devicePumpStatusFuncCode{};
    int m_devicePumpStatusAddress{};
    int m_deviceBarrelLevelFuncCode{};
    int m_deviceBarrelLevelAddress{};
    int m_devicePumpCtrlFuncCode{};
    int m_devicePumpCtrlAddress{};
    QString m_deviceAiScaleRange;
    QString m_deviceTankLevelRange;

    QString m_deviceRemoteModeFuncCodeInfo{
        R"({"id": 12, "groupPath": "设备监控配置/读点位设置", "uiName": "远程/本地状态 功能码", "type": "enum", "value": 0, "validator": {"enumNames": ["01 (0x01) 读线圈", "02 (0x02) 读离散输入", "03 (0x03) 读保持寄存器", "04 (0x04) 读输入寄存器"]}})"};
    QString m_deviceRemoteModeAddressInfo{
        R"({"id": 13, "groupPath": "设备监控配置/读点位设置", "uiName": "远程/本地状态 数据地址", "type": "int", "value": 1000, "validator": {"min": 0}})"};
    QString m_devicePumpStatusFuncCodeInfo{
        R"({"id": 14, "groupPath": "设备监控配置/读点位设置", "uiName": "泵启停状态 功能码", "type": "enum", "value": 0, "validator": {"enumNames": ["01 (0x01) 读线圈", "02 (0x02) 读离散输入", "03 (0x03) 读保持寄存器", "04 (0x04) 读输入寄存器"]}})"};
    QString m_devicePumpStatusAddressInfo{
        R"({"id": 15, "groupPath": "设备监控配置/读点位设置", "uiName": "泵启停状态 数据地址", "type": "int", "value": 1000, "validator": {"min": 0}})"};
    QString m_deviceBarrelLevelFuncCodeInfo{
        R"({"id": 16, "groupPath": "设备监控配置/读点位设置", "uiName": "桶液位 功能码", "type": "enum", "value": 0, "validator": {"enumNames": ["01 (0x01) 读线圈", "02 (0x02) 读离散输入", "03 (0x03) 读保持寄存器", "04 (0x04) 读输入寄存器"]}})"};
    QString m_deviceBarrelLevelAddressInfo{
        R"({"id": 17, "groupPath": "设备监控配置/读点位设置", "uiName": "桶液位 数据地址", "type": "int", "value": 1000, "validator": {"min": 0}})"};
    QString m_devicePumpCtrlFuncCodeInfo{
        R"({"id": 18, "groupPath": "设备监控配置/写点位设置", "uiName": "泵启停控制 功能码", "type": "enum", "value": 0, "validator": {"enumNames": ["05 (0x05) 写单个线圈", "06 (0x06) 写单个寄存器"]}})"};
    QString m_devicePumpCtrlAddressInfo{
        R"({"id": 19, "groupPath": "设备监控配置/写点位设置", "uiName": "泵启停控制 数据地址", "type": "int", "value": 1000, "validator": {"min": 0}})"};
    QString m_deviceAiScaleRangeInfo{
        R"({"id": 20, "groupPath": "设备监控配置/其他设置", "uiName": "模拟量标度范围", "type": "string", "value": "4000:20000", "validator": {"regExp": "^(\\d+):(\\d+)$"}})"};
    QString m_deviceTankLevelRangeInfo{
        R"({"id": 21, "groupPath": "设备监控配置/其他设置", "uiName": "桶液位工程量范围", "type": "string", "value": "0.00:100.00", "validator": {"regExp": "^(\\d+(?:\\.\\d+)?):(\\d+(?:\\.\\d+)?)$"}})"};

    QString m_wsServerUrl{};
    QString m_wsServerUrlInfo{
        R"({"id": 22, "groupPath": "WebSocket配置", "uiName": "服务器地址", "type": "string", "value": "ws:\/\/127.0.0.1:8080", "validator": {"regExp": "^wss?:\/\/.*"}})"};

    // 辅助函数
    QHash<QString, QString> m_nameToInfoHash{};
    QList<QString> m_sortedValidPropNames{};
    void updatePropInfoValue(const QString &propName, const QVariant &propValue);
};

#endif // UGVCOMMAGENT_CONFIGSOURCE_H
