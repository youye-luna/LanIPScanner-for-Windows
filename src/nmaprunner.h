#pragma once

#include <functional>

#include <QString>
#include <QStringList>
#include <QVector>

struct ScanCancelToken;

/// nmap 主机发现的结果
struct NmapHost
{
    QString ipAddress;
    QString macAddress; ///< 仅在安装了 Npcap 时 nmap 才会返回 MAC
    QString hostName;
    qint64 rttMs = -1;
};

/// 官方 nmap 子进程封装：只用于主机发现（nmap -sn）
namespace NmapRunner
{
/// 定位 nmap.exe：优先程序目录下的 nmap\nmap.exe，其次 PATH；找不到返回空
QString findNmapExecutable();

/// 用 nmap -sn 对 ipList 做主机发现，只返回在线主机。
/// onProgress 收到 nmap 报告的 0-100 进度；token 用于取消。
/// 失败时返回空列表并写入 errorMessage（调用方据此区分「扫描失败」与「没有在线主机」）。
QVector<NmapHost> scanHosts(const QStringList &ipList,
                            const std::function<void(int)> &onProgress,
                            const ScanCancelToken *token,
                            QString *errorMessage);
} // namespace NmapRunner
