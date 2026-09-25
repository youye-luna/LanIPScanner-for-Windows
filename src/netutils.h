#pragma once

#include <QHash>
#include <QString>
#include <QVector>

/// 底层网络工具（ICMP / TCP / ARP / 本机信息 / 反向 DNS）
namespace NetUtils
{
/// IPv4 字符串转 32 位数值，失败返回 -1
qint64 ipToLong(const QString &ip);

/// 32 位数值转 IPv4 字符串
QString longToIp(qint64 value);

/// 是否为内网地址（10.x / 172.16-31.x / 192.168.x）
bool isPrivateIp(const QString &ip);

/// ICMP 探测：成功返回 true 并写入往返毫秒数
bool icmpPing(const QString &ip, int timeoutMs, qint64 *roundTripMs);

/// 端口探测的确定性结果
struct PortProbeResult
{
    int openPort = -1;    ///< 探测到的第一个开放端口，无则 -1
    bool hostAlive = false; ///< 主机在线（连接被拒绝即认为在线）
};

/// 在总时间预算内按顺序探测端口，返回第一个确定性结果
PortProbeResult probePorts(const QString &ip, const QVector<int> &ports, int totalBudgetMs);

/// arp -a 全表，返回 IP -> MAC（AA-BB-CC-DD-EE-FF 大写）
QHash<QString, QString> readArpTable(int timeoutMs = 1500);

/// arp -a <ip> 单条查询，失败返回空
QString queryArpEntry(const QString &ip, int timeoutMs = 1000);

/// 本机物理网卡 MAC（AA-BB-CC-DD-EE-FF 大写），失败返回空
QString localMacAddress();

/// 默认网关 IPv4，失败返回空
QString defaultGatewayIp();

/// 本机第一个非回环 IPv4，失败返回空
QString localIpv4Address();

/// 带超时的反向 DNS 解析，失败/超时返回空
QString resolveHostName(const QString &ip, int timeoutMs);
} // namespace NetUtils
