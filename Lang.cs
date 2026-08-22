using System.Collections.Generic;

namespace DhcpScanner
{
    /// <summary>
    /// 多语言文本管理（简体中文 / English / 繁体中文·台湾 / 繁体中文·香港）
    /// </summary>
    public static class Lang
    {
        /// <summary>
        /// 当前界面语言
        /// </summary>
        public static AppLanguage Current { get; set; } = AppLanguage.Chinese;

        /// <summary>
        /// 根据键获取当前语言的文本
        /// </summary>
        public static string Get(string key)
        {
            switch (Current)
            {
                case AppLanguage.English:
                    return En.TryGetValue(key, out var en) ? en : key;
                case AppLanguage.TraditionalChinese:
                    return ZhTw.TryGetValue(key, out var tw) ? tw : key;
                case AppLanguage.TraditionalChineseHk:
                    return ZhHk.TryGetValue(key, out var hk) ? hk : key;
                default:
                    return Zh.TryGetValue(key, out var zh) ? zh : key;
            }
        }

        private static readonly Dictionary<string, string> Zh = new()
        {
            // 主窗口
            ["FormTitle"] = "局域网设备扫描工具",
            ["ScanRangeTitle"] = "搜索范围设置",
            ["StartIp"] = "起始IP:",
            ["To"] = "至",
            ["EndIp"] = "结束IP:",
            ["StartScan"] = "开始扫描",
            ["StopScan"] = "停止扫描",
            ["ClearResults"] = "清空结果",
            ["ExportResults"] = "导出结果",
            ["Settings"] = "设置",
            ["History"] = "历史",
            ["Ready"] = "就绪",
            ["Tip"] = "提示",
            ["Error"] = "错误",
            ["Success"] = "成功",
            ["Confirm"] = "确认",
            ["Ok"] = "确定",
            ["Cancel"] = "取消",

            // 状态栏
            ["StatusCountInit"] = "发现 0 个DHCP服务器",
            ["StatusCountScanning"] = "发现 0 个设备",
            ["StatusCountDone"] = "在线: {0}，无设备: {1}，DHCP服务器: {2}",
            ["ScanningRange"] = "正在扫描 {0} ~ {1}...",
            ["ScanProgressPercent"] = "正在扫描... {0}%",
            ["ScanStopped"] = "扫描已停止",
            ["OrganizingResults"] = "正在整理结果...",
            ["ScanCompletedStatus"] = "扫描完成",
            ["ScanErrorStatus"] = "扫描出错: {0}",
            ["ScanErrorDialog"] = "扫描过程中发生错误:\n{0}",

            // 扫描提示
            ["ScanningInProgress"] = "扫描正在进行中，请等待完成或停止扫描。",
            ["InputIpRequired"] = "请输入起始IP和结束IP！",
            ["PrivateIpOnly"] = "只能扫描内网地址！\n\n允许的范围：\n10.0.0.0 ~ 10.255.255.255\n172.16.0.0 ~ 172.31.255.255\n192.168.0.0 ~ 192.168.255.255",
            ["ExitWhileScanning"] = "扫描正在进行中，确定要退出吗？",
            ["SubnetTab"] = "网段 {0}",
            ["ScanSummary"] = "扫描完成！\n\n共扫描 {0} 个IP\n在线设备: {1}\n无设备: {2}\nDHCP服务器: {3}",
            ["Completed"] = "完成",
            ["MaxSubnets"] = "最多测100个网段",
            ["TooManySubnetsSub"] = "当前有 {0} 个网段\n什么鬼，谁家网段那么多",

            // 导出
            ["NoDataToExport"] = "没有可导出的数据！",
            ["ExportFilter"] = "CSV文件 (*.csv)|*.csv|文本文件 (*.txt)|*.txt|所有文件 (*.*)|*.*",
            ["ExportFileName"] = "DHCP扫描结果",
            ["ExportSuccess"] = "数据已成功导出到:\n{0}",
            ["ExportFailed"] = "导出失败: {0}",
            ["CsvHeader"] = "网段,IP地址,MAC地址,主机名,延迟(ms),DHCP服务器,状态",

            // 结果表格
            ["ColIp"] = "IP地址",
            ["ColMac"] = "MAC地址",
            ["ColHost"] = "主机名",
            ["ColPing"] = "延迟(ms)",
            ["ColDhcp"] = "DHCP服务器",
            ["ColStatus"] = "状态",
            ["Online"] = "在线",
            ["NoDevice"] = "无设备",
            ["Yes"] = "是",
            ["No"] = "否",

            // 设备详情
            ["DeviceDetail"] = "设备详情 - {0}",
            ["FieldIp"] = "IP 地 址",
            ["FieldMac"] = "MAC 地址",
            ["FieldHost"] = "主 机 名",
            ["FieldPing"] = "延迟",
            ["AccessAdmin"] = "访问后台",
            ["IeAccess"] = "IE访问",
            ["Close"] = "关闭",
            ["IeNotRegistered"] = "IE COM 组件未注册，请确认已启用 Internet Explorer 11 功能。",
            ["IeLaunchFailed"] = "无法启动 IE 浏览器：{0}\n请确认已启用 Internet Explorer 11 功能。",

            // IP分布图
            ["IpDistribution"] = "IP 地址分布图",
            ["NotScanned"] = "未扫描",

            // 设置窗口
            ["SettingsTitle"] = "设置",
            ["LanguageLabel"] = "界面语言:",
            ["ThreadsLabel"] = "扫描线程数:",
            ["ThreadsHint"] = "并发扫描的线程数（1-100），线程越多扫描越快，但会占用更多系统资源",

            // 数据保存配置
            ["SaveMethodGroup"] = "数据保存方式",
            ["SaveByTime"] = "按时间保存",
            ["SaveByCount"] = "按数量保存",
            ["SaveRangeGroup"] = "保存范围",
            ["Range14Days"] = "近14天",
            ["RangeHalfMonth"] = "半个月",
            ["RangeOneMonth"] = "一个月",
            ["RangeOneYear"] = "一年",
            ["RangeNever"] = "永不清除",
            ["RangeCustom"] = "自定义天数",
            ["Range30"] = "近30个",
            ["Range60"] = "近60个",
            ["Range90"] = "近90个",
            ["Range100"] = "近100个",
            ["SaveSettings"] = "保存设置",
            ["SaveConfigSuccess"] = "保存配置已应用！",

            // 关于
            ["About"] = "关于",
            ["AboutTitle"] = "关于",
            ["AboutVersion"] = "版本号: {0}",
            ["AboutCreator"] = "创作者: {0}",
            ["AboutFeatures"] = "扫描局域网在线设备|检测DHCP服务器|多网段自动分组|导出扫描结果|多语言界面支持",

            // 扫描历史
            ["HistoryTitle"] = "扫描历史",
            ["HistoryHint"] = "双击历史记录可重新加载扫描结果",
            ["HistoryDateFormat"] = "yyyy年MM月dd日",
            ["HistoryRangeFormat"] = "{0}~{1}（IP）",
            ["ColHistoryTime"] = "扫描时间",
            ["ColHistoryRange"] = "扫描范围",
            ["ColHistoryTotal"] = "设备总数",
            ["ColHistoryOnline"] = "在线设备",
            ["ColHistoryDhcp"] = "DHCP服务器",
            ["HistoryView"] = "查看",
            ["HistoryDelete"] = "删除",
            ["HistoryClear"] = "清空",
            ["HistoryEmpty"] = "暂无扫描历史",
            ["HistorySelectFirst"] = "请先选择一条历史记录！",
            ["HistoryConfirmDelete"] = "确定删除这条历史记录吗？",
            ["HistoryConfirmClear"] = "确定清空所有历史记录吗？",
            ["HistoryLoaded"] = "已加载历史记录：{0} ~ {1}",
        };

        private static readonly Dictionary<string, string> En = new()
        {
            // Main window
            ["FormTitle"] = "LAN Device Scanner",
            ["ScanRangeTitle"] = "Scan Range Settings",
            ["StartIp"] = "Start IP:",
            ["To"] = "to",
            ["EndIp"] = "End IP:",
            ["StartScan"] = "Start Scan",
            ["StopScan"] = "Stop Scan",
            ["ClearResults"] = "Clear Results",
            ["ExportResults"] = "Export Results",
            ["Settings"] = "Settings",
            ["History"] = "History",
            ["Ready"] = "Ready",
            ["Tip"] = "Notice",
            ["Error"] = "Error",
            ["Success"] = "Success",
            ["Confirm"] = "Confirm",
            ["Ok"] = "OK",
            ["Cancel"] = "Cancel",

            // Status bar
            ["StatusCountInit"] = "Found 0 DHCP servers",
            ["StatusCountScanning"] = "Found 0 devices",
            ["StatusCountDone"] = "Online: {0}, No device: {1}, DHCP servers: {2}",
            ["ScanningRange"] = "Scanning {0} ~ {1}...",
            ["ScanProgressPercent"] = "Scanning... {0}%",
            ["ScanStopped"] = "Scan stopped",
            ["OrganizingResults"] = "Organizing results...",
            ["ScanCompletedStatus"] = "Scan completed",
            ["ScanErrorStatus"] = "Scan error: {0}",
            ["ScanErrorDialog"] = "An error occurred during the scan:\n{0}",

            // Scan prompts
            ["ScanningInProgress"] = "A scan is in progress. Please wait for it to finish or stop the scan.",
            ["InputIpRequired"] = "Please enter the start IP and end IP!",
            ["PrivateIpOnly"] = "Only private (LAN) addresses can be scanned!\n\nAllowed ranges:\n10.0.0.0 ~ 10.255.255.255\n172.16.0.0 ~ 172.31.255.255\n192.168.0.0 ~ 192.168.255.255",
            ["ExitWhileScanning"] = "A scan is in progress. Are you sure you want to exit?",
            ["SubnetTab"] = "Subnet {0}",
            ["ScanSummary"] = "Scan completed!\n\nScanned {0} IPs\nOnline devices: {1}\nNo device: {2}\nDHCP servers: {3}",
            ["Completed"] = "Completed",
            ["MaxSubnets"] = "Up to 100 subnets allowed",
            ["TooManySubnetsSub"] = "Found {0} subnets\nWhoa, that's a lot of subnets",

            // Export
            ["NoDataToExport"] = "No data to export!",
            ["ExportFilter"] = "CSV files (*.csv)|*.csv|Text files (*.txt)|*.txt|All files (*.*)|*.*",
            ["ExportFileName"] = "DHCP_Scan_Result",
            ["ExportSuccess"] = "Data has been exported to:\n{0}",
            ["ExportFailed"] = "Export failed: {0}",
            ["CsvHeader"] = "Subnet,IP Address,MAC Address,Host Name,Latency(ms),DHCP Server,Status",

            // Result grid
            ["ColIp"] = "IP Address",
            ["ColMac"] = "MAC Address",
            ["ColHost"] = "Host Name",
            ["ColPing"] = "Latency(ms)",
            ["ColDhcp"] = "DHCP Server",
            ["ColStatus"] = "Status",
            ["Online"] = "Online",
            ["NoDevice"] = "No device",
            ["Yes"] = "Yes",
            ["No"] = "No",

            // Device details
            ["DeviceDetail"] = "Device Details - {0}",
            ["FieldIp"] = "IP Address",
            ["FieldMac"] = "MAC Address",
            ["FieldHost"] = "Host Name",
            ["FieldPing"] = "Latency",
            ["AccessAdmin"] = "Open Admin Page",
            ["IeAccess"] = "Open in IE",
            ["Close"] = "Close",
            ["IeNotRegistered"] = "IE COM component is not registered. Please make sure the Internet Explorer 11 feature is enabled.",
            ["IeLaunchFailed"] = "Failed to launch Internet Explorer: {0}\nPlease make sure the Internet Explorer 11 feature is enabled.",

            // IP map
            ["IpDistribution"] = "IP Address Map",
            ["NotScanned"] = "Not scanned",

            // Settings form
            ["SettingsTitle"] = "Settings",
            ["LanguageLabel"] = "Language:",
            ["ThreadsLabel"] = "Scan threads:",
            ["ThreadsHint"] = "Number of concurrent scan threads (1-100). More threads speed up the scan but use more system resources",

            // Data save configuration
            ["SaveMethodGroup"] = "Data Save Method",
            ["SaveByTime"] = "Save by time",
            ["SaveByCount"] = "Save by count",
            ["SaveRangeGroup"] = "Save Range",
            ["Range14Days"] = "Last 14 days",
            ["RangeHalfMonth"] = "Half month",
            ["RangeOneMonth"] = "One month",
            ["RangeOneYear"] = "One year",
            ["RangeNever"] = "Never clear",
            ["RangeCustom"] = "Custom days",
            ["Range30"] = "Last 30 records",
            ["Range60"] = "Last 60 records",
            ["Range90"] = "Last 90 records",
            ["Range100"] = "Last 100 records",
            ["SaveSettings"] = "Save Settings",
            ["SaveConfigSuccess"] = "Save configuration applied!",

            // About
            ["About"] = "About",
            ["AboutTitle"] = "About",
            ["AboutVersion"] = "Version: {0}",
            ["AboutCreator"] = "Creator: {0}",
            ["AboutFeatures"] = "Scan LAN online devices|Detect DHCP servers|Auto-group by subnet|Export scan results|Multi-language UI support",

            // Scan history
            ["HistoryTitle"] = "Scan History",
            ["HistoryHint"] = "Double-click a record to reload the scan results",
            ["HistoryDateFormat"] = "yyyy-MM-dd",
            ["HistoryRangeFormat"] = "{0}~{1} (IP)",
            ["ColHistoryTime"] = "Scan Time",
            ["ColHistoryRange"] = "Scan Range",
            ["ColHistoryTotal"] = "Total",
            ["ColHistoryOnline"] = "Online",
            ["ColHistoryDhcp"] = "DHCP Servers",
            ["HistoryView"] = "View",
            ["HistoryDelete"] = "Delete",
            ["HistoryClear"] = "Clear All",
            ["HistoryEmpty"] = "No scan history yet",
            ["HistorySelectFirst"] = "Please select a history record first!",
            ["HistoryConfirmDelete"] = "Delete this history record?",
            ["HistoryConfirmClear"] = "Clear all history records?",
            ["HistoryLoaded"] = "History loaded: {0} ~ {1}",
        };

        private static readonly Dictionary<string, string> ZhTw = new()
        {
            // 主視窗
            ["FormTitle"] = "區域網路裝置掃描工具",
            ["ScanRangeTitle"] = "掃描範圍設定",
            ["StartIp"] = "起始IP:",
            ["To"] = "至",
            ["EndIp"] = "結束IP:",
            ["StartScan"] = "開始掃描",
            ["StopScan"] = "停止掃描",
            ["ClearResults"] = "清除結果",
            ["ExportResults"] = "匯出結果",
            ["Settings"] = "設定",
            ["History"] = "歷史",
            ["Ready"] = "就緒",
            ["Tip"] = "提示",
            ["Error"] = "錯誤",
            ["Success"] = "成功",
            ["Confirm"] = "確認",
            ["Ok"] = "確定",
            ["Cancel"] = "取消",

            // 狀態列
            ["StatusCountInit"] = "發現 0 個DHCP伺服器",
            ["StatusCountScanning"] = "發現 0 個裝置",
            ["StatusCountDone"] = "線上: {0}，無裝置: {1}，DHCP伺服器: {2}",
            ["ScanningRange"] = "正在掃描 {0} ~ {1}...",
            ["ScanProgressPercent"] = "正在掃描... {0}%",
            ["ScanStopped"] = "掃描已停止",
            ["OrganizingResults"] = "正在整理結果...",
            ["ScanCompletedStatus"] = "掃描完成",
            ["ScanErrorStatus"] = "掃描出錯: {0}",
            ["ScanErrorDialog"] = "掃描過程中發生錯誤:\n{0}",

            // 掃描提示
            ["ScanningInProgress"] = "掃描正在進行中，請等待完成或停止掃描。",
            ["InputIpRequired"] = "請輸入起始IP和結束IP！",
            ["PrivateIpOnly"] = "只能掃描內網地址！\n\n允許的範圍：\n10.0.0.0 ~ 10.255.255.255\n172.16.0.0 ~ 172.31.255.255\n192.168.0.0 ~ 192.168.255.255",
            ["ExitWhileScanning"] = "掃描正在進行中，確定要退出嗎？",
            ["SubnetTab"] = "網段 {0}",
            ["ScanSummary"] = "掃描完成！\n\n共掃描 {0} 個IP\n線上裝置: {1}\n無裝置: {2}\nDHCP伺服器: {3}",
            ["Completed"] = "完成",
            ["MaxSubnets"] = "最多測100個網段",
            ["TooManySubnetsSub"] = "目前有 {0} 個網段\n這也太多網段了吧",

            // 匯出
            ["NoDataToExport"] = "沒有可匯出的資料！",
            ["ExportFilter"] = "CSV檔案 (*.csv)|*.csv|文字檔案 (*.txt)|*.txt|所有檔案 (*.*)|*.*",
            ["ExportFileName"] = "DHCP掃描結果",
            ["ExportSuccess"] = "資料已成功匯出到:\n{0}",
            ["ExportFailed"] = "匯出失敗: {0}",
            ["CsvHeader"] = "網段,IP位址,MAC位址,主機名稱,延遲(ms),DHCP伺服器,狀態",

            // 結果表格
            ["ColIp"] = "IP位址",
            ["ColMac"] = "MAC位址",
            ["ColHost"] = "主機名稱",
            ["ColPing"] = "延遲(ms)",
            ["ColDhcp"] = "DHCP伺服器",
            ["ColStatus"] = "狀態",
            ["Online"] = "線上",
            ["NoDevice"] = "無裝置",
            ["Yes"] = "是",
            ["No"] = "否",

            // 裝置詳情
            ["DeviceDetail"] = "裝置詳情 - {0}",
            ["FieldIp"] = "IP 位 址",
            ["FieldMac"] = "MAC 位址",
            ["FieldHost"] = "主 機 名",
            ["FieldPing"] = "延遲",
            ["AccessAdmin"] = "存取後台",
            ["IeAccess"] = "IE存取",
            ["Close"] = "關閉",
            ["IeNotRegistered"] = "IE COM 元件未註冊，請確認已啟用 Internet Explorer 11 功能。",
            ["IeLaunchFailed"] = "無法啟動 IE 瀏覽器：{0}\n請確認已啟用 Internet Explorer 11 功能。",

            // IP分佈圖
            ["IpDistribution"] = "IP 位址分佈圖",
            ["NotScanned"] = "未掃描",

            // 設定視窗
            ["SettingsTitle"] = "設定",
            ["LanguageLabel"] = "介面語言:",
            ["ThreadsLabel"] = "掃描執行緒數:",
            ["ThreadsHint"] = "並行掃描的執行緒數（1-100），執行緒越多掃描越快，但會占用更多系統資源",

            // 資料保存設定
            ["SaveMethodGroup"] = "資料保存方式",
            ["SaveByTime"] = "按時間保存",
            ["SaveByCount"] = "按數量保存",
            ["SaveRangeGroup"] = "保存範圍",
            ["Range14Days"] = "近14天",
            ["RangeHalfMonth"] = "半個月",
            ["RangeOneMonth"] = "一個月",
            ["RangeOneYear"] = "一年",
            ["RangeNever"] = "永不清除",
            ["RangeCustom"] = "自訂天數",
            ["Range30"] = "近30個",
            ["Range60"] = "近60個",
            ["Range90"] = "近90個",
            ["Range100"] = "近100個",
            ["SaveSettings"] = "儲存設定",
            ["SaveConfigSuccess"] = "儲存設定已套用！",

            // 關於
            ["About"] = "關於",
            ["AboutTitle"] = "關於",
            ["AboutVersion"] = "版本號: {0}",
            ["AboutCreator"] = "創作者: {0}",
            ["AboutFeatures"] = "掃描區域網路線上裝置|偵測DHCP伺服器|多網段自動分組|匯出掃描結果|多語言介面支援",

            // 掃描歷史
            ["HistoryTitle"] = "掃描歷史",
            ["HistoryHint"] = "雙擊歷史記錄可重新載入掃描結果",
            ["HistoryDateFormat"] = "yyyy年MM月dd日",
            ["HistoryRangeFormat"] = "{0}~{1}（IP）",
            ["ColHistoryTime"] = "掃描時間",
            ["ColHistoryRange"] = "掃描範圍",
            ["ColHistoryTotal"] = "裝置總數",
            ["ColHistoryOnline"] = "線上裝置",
            ["ColHistoryDhcp"] = "DHCP伺服器",
            ["HistoryView"] = "檢視",
            ["HistoryDelete"] = "刪除",
            ["HistoryClear"] = "清空",
            ["HistoryEmpty"] = "尚無掃描歷史",
            ["HistorySelectFirst"] = "請先選擇一筆歷史記錄！",
            ["HistoryConfirmDelete"] = "確定刪除這筆歷史記錄嗎？",
            ["HistoryConfirmClear"] = "確定清空所有歷史記錄嗎？",
            ["HistoryLoaded"] = "已載入掃描歷史：{0} ~ {1}",
        };

        private static readonly Dictionary<string, string> ZhHk = new()
        {
            // 主視窗（香港/澳門用詞：網絡、資料、軟件）
            ["FormTitle"] = "區域網絡裝置掃描工具",
            ["ScanRangeTitle"] = "掃描範圍設定",
            ["StartIp"] = "起始IP:",
            ["To"] = "至",
            ["EndIp"] = "結束IP:",
            ["StartScan"] = "開始掃描",
            ["StopScan"] = "停止掃描",
            ["ClearResults"] = "清除結果",
            ["ExportResults"] = "匯出結果",
            ["Settings"] = "設定",
            ["History"] = "歷史",
            ["Ready"] = "就緒",
            ["Tip"] = "提示",
            ["Error"] = "錯誤",
            ["Success"] = "成功",
            ["Confirm"] = "確認",
            ["Ok"] = "確定",
            ["Cancel"] = "取消",

            // 狀態列
            ["StatusCountInit"] = "發現 0 個DHCP伺服器",
            ["StatusCountScanning"] = "發現 0 個裝置",
            ["StatusCountDone"] = "線上: {0}，無裝置: {1}，DHCP伺服器: {2}",
            ["ScanningRange"] = "正在掃描 {0} ~ {1}...",
            ["ScanProgressPercent"] = "正在掃描... {0}%",
            ["ScanStopped"] = "掃描已停止",
            ["OrganizingResults"] = "正在整理結果...",
            ["ScanCompletedStatus"] = "掃描完成",
            ["ScanErrorStatus"] = "掃描出錯: {0}",
            ["ScanErrorDialog"] = "掃描過程中發生錯誤:\n{0}",

            // 掃描提示
            ["ScanningInProgress"] = "掃描正在進行中，請等待完成或停止掃描。",
            ["InputIpRequired"] = "請輸入起始IP和結束IP！",
            ["PrivateIpOnly"] = "只能掃描內網位址！\n\n允許的範圍：\n10.0.0.0 ~ 10.255.255.255\n172.16.0.0 ~ 172.31.255.255\n192.168.0.0 ~ 192.168.255.255",
            ["ExitWhileScanning"] = "掃描正在進行中，確定要退出嗎？",
            ["SubnetTab"] = "網段 {0}",
            ["ScanSummary"] = "掃描完成！\n\n共掃描 {0} 個IP\n線上裝置: {1}\n無裝置: {2}\nDHCP伺服器: {3}",
            ["Completed"] = "完成",
            ["MaxSubnets"] = "最多測100個網段",
            ["TooManySubnetsSub"] = "目前有 {0} 個網段\n這也太多網段了吧",

            // 匯出
            ["NoDataToExport"] = "沒有可匯出的資料！",
            ["ExportFilter"] = "CSV檔案 (*.csv)|*.csv|文字檔案 (*.txt)|*.txt|所有檔案 (*.*)|*.*",
            ["ExportFileName"] = "DHCP掃描結果",
            ["ExportSuccess"] = "資料已成功匯出到:\n{0}",
            ["ExportFailed"] = "匯出失敗: {0}",
            ["CsvHeader"] = "網段,IP位址,MAC位址,主機名稱,延遲(ms),DHCP伺服器,狀態",

            // 結果表格
            ["ColIp"] = "IP位址",
            ["ColMac"] = "MAC位址",
            ["ColHost"] = "主機名稱",
            ["ColPing"] = "延遲(ms)",
            ["ColDhcp"] = "DHCP伺服器",
            ["ColStatus"] = "狀態",
            ["Online"] = "線上",
            ["NoDevice"] = "無裝置",
            ["Yes"] = "是",
            ["No"] = "否",

            // 裝置詳情
            ["DeviceDetail"] = "裝置詳情 - {0}",
            ["FieldIp"] = "IP 位 址",
            ["FieldMac"] = "MAC 位址",
            ["FieldHost"] = "主 機 名",
            ["FieldPing"] = "延遲",
            ["AccessAdmin"] = "存取後台",
            ["IeAccess"] = "IE存取",
            ["Close"] = "關閉",
            ["IeNotRegistered"] = "IE COM 元件未註冊，請確認已啟用 Internet Explorer 11 功能。",
            ["IeLaunchFailed"] = "無法啟動 IE 瀏覽器：{0}\n請確認已啟用 Internet Explorer 11 功能。",

            // IP分佈圖
            ["IpDistribution"] = "IP 位址分佈圖",
            ["NotScanned"] = "未掃描",

            // 設定視窗
            ["SettingsTitle"] = "設定",
            ["LanguageLabel"] = "介面語言:",
            ["ThreadsLabel"] = "掃描執行緒數:",
            ["ThreadsHint"] = "並行掃描的執行緒數（1-100），執行緒越多掃描越快，但會占用更多系統資源",

            // 資料保存設定
            ["SaveMethodGroup"] = "資料保存方式",
            ["SaveByTime"] = "按時間保存",
            ["SaveByCount"] = "按數量保存",
            ["SaveRangeGroup"] = "保存範圍",
            ["Range14Days"] = "近14天",
            ["RangeHalfMonth"] = "半個月",
            ["RangeOneMonth"] = "一個月",
            ["RangeOneYear"] = "一年",
            ["RangeNever"] = "永不清除",
            ["RangeCustom"] = "自訂天數",
            ["Range30"] = "近30個",
            ["Range60"] = "近60個",
            ["Range90"] = "近90個",
            ["Range100"] = "近100個",
            ["SaveSettings"] = "儲存設定",
            ["SaveConfigSuccess"] = "儲存設定已套用！",

            // 關於
            ["About"] = "關於",
            ["AboutTitle"] = "關於",
            ["AboutVersion"] = "版本號: {0}",
            ["AboutCreator"] = "創作者: {0}",
            ["AboutFeatures"] = "掃描區域網絡線上裝置|偵測DHCP伺服器|多網段自動分組|匯出掃描結果|多語言介面支援",

            // 掃描歷史
            ["HistoryTitle"] = "掃描歷史",
            ["HistoryHint"] = "雙擊歷史記錄可重新載入掃描結果",
            ["HistoryDateFormat"] = "yyyy年MM月dd日",
            ["HistoryRangeFormat"] = "{0}~{1}（IP）",
            ["ColHistoryTime"] = "掃描時間",
            ["ColHistoryRange"] = "掃描範圍",
            ["ColHistoryTotal"] = "裝置總數",
            ["ColHistoryOnline"] = "線上裝置",
            ["ColHistoryDhcp"] = "DHCP伺服器",
            ["HistoryView"] = "檢視",
            ["HistoryDelete"] = "刪除",
            ["HistoryClear"] = "清空",
            ["HistoryEmpty"] = "尚無掃描歷史",
            ["HistorySelectFirst"] = "請先選擇一筆歷史記錄！",
            ["HistoryConfirmDelete"] = "確定刪除這筆歷史記錄嗎？",
            ["HistoryConfirmClear"] = "確定清空所有歷史記錄嗎？",
            ["HistoryLoaded"] = "已載入掃描歷史：{0} ~ {1}",
        };
    }
}
