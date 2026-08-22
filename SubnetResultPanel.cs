using System;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;

namespace DhcpScanner
{
    /// <summary>
    /// 单个网段的结果面板（DataGridView + IPGridPanel）
    /// </summary>
    public class SubnetResultPanel : UserControl
    {
        private readonly DataGridView _grid;
        private readonly IPGridPanel _ipGrid;
        private List<DhcpServerInfo> _results = new();

        public SubnetResultPanel()
        {
            // 表格
            _grid = new DataGridView
            {
                Dock = DockStyle.Fill,
                AllowUserToAddRows = false,
                AllowUserToDeleteRows = false,
                ReadOnly = true,
                SelectionMode = DataGridViewSelectionMode.FullRowSelect,
                AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.None,
                BackgroundColor = Color.White,
                BorderStyle = BorderStyle.None,
                RowHeadersVisible = false,
                Font = new Font("Microsoft YaHei", 10F),
                ColumnHeadersHeight = 35,
                EnableHeadersVisualStyles = false,
            };
            _grid.ColumnHeadersDefaultCellStyle.BackColor = Color.FromArgb(0, 120, 215);
            _grid.ColumnHeadersDefaultCellStyle.ForeColor = Color.White;
            _grid.ColumnHeadersDefaultCellStyle.Font = new Font("Microsoft YaHei", 10F, FontStyle.Bold);
            _grid.DefaultCellStyle.SelectionBackColor = Color.FromArgb(200, 220, 240);
            _grid.DefaultCellStyle.SelectionForeColor = Color.Black;
            _grid.AlternatingRowsDefaultCellStyle.BackColor = Color.FromArgb(245, 248, 250);

            // 不显示"网段"列（每个标签页已经按网段分组了）
            _grid.Columns.Add("IpAddress", Lang.Get("ColIp"));
            _grid.Columns.Add("MacAddress", Lang.Get("ColMac"));
            _grid.Columns.Add("HostName", Lang.Get("ColHost"));
            _grid.Columns.Add("PingMs", Lang.Get("ColPing"));
            _grid.Columns.Add("IsRouter", Lang.Get("ColDhcp"));
            _grid.Columns.Add("Status", Lang.Get("ColStatus"));

            _grid.Columns["IpAddress"].Width = 140;
            _grid.Columns["MacAddress"].Width = 160;
            _grid.Columns["HostName"].Width = 150;
            _grid.Columns["PingMs"].Width = 80;
            _grid.Columns["IsRouter"].Width = 110;
            _grid.Columns["Status"].Width = 80;

            _grid.CellDoubleClick += Grid_CellDoubleClick;

            // IP网格
            _ipGrid = new IPGridPanel
            {
                Dock = DockStyle.Right,
                Width = 480,
                BackColor = Color.White,
                BorderStyle = BorderStyle.FixedSingle,
            };
            _ipGrid.CellClicked += IpGrid_CellClicked;
            _ipGrid.CellDoubleClicked += IpGrid_CellDoubleClicked;

            // 添加控件（先加右再加左，保证Dock顺序正确）
            Controls.Add(_grid);
            Controls.Add(_ipGrid);
        }

        /// <summary>
        /// 填充数据
        /// </summary>
        public void PopulateData(List<DhcpServerInfo> results)
        {
            _results = results;
            _grid.Rows.Clear();
            _ipGrid.ResetAllColors();

            foreach (var info in results)
            {
                string statusText = info.IsActive ? Lang.Get("Online") : Lang.Get("NoDevice");
                string pingText = info.IsActive ? info.PingMs.ToString() : "-";

                int rowIndex = _grid.Rows.Add(
                    info.IpAddress.ToString(),
                    info.IsActive ? info.MacAddress : "-",
                    info.IsActive ? info.HostName : "-",
                    pingText,
                    info.IsDhcpServer ? Lang.Get("Yes") : Lang.Get("No"),
                    statusText
                );

                var row = _grid.Rows[rowIndex];
                row.Tag = info;

                if (info.IsDhcpServer)
                {
                    row.Cells["IsRouter"].Style.ForeColor = Color.Red;
                    row.Cells["IsRouter"].Style.Font = new Font(_grid.Font, FontStyle.Bold);
                    row.Cells["IpAddress"].Style.ForeColor = Color.FromArgb(0, 102, 204);
                    row.Cells["IpAddress"].Style.Font = new Font(_grid.Font, FontStyle.Underline);
                    row.Cells["IpAddress"].Tag = "dhcp";
                }

                if (info.IsActive)
                {
                    row.Cells["Status"].Style.ForeColor = Color.Green;
                    row.Cells["Status"].Style.Font = new Font(_grid.Font, FontStyle.Bold);
                }
                else
                {
                    row.Cells["Status"].Style.ForeColor = Color.Gray;
                    row.DefaultCellStyle.BackColor = Color.FromArgb(245, 245, 245);
                }

                // 更新IP网格颜色
                var parts = info.IpAddress.ToString().Split('.');
                if (parts.Length == 4 && int.TryParse(parts[3], out int ipLast))
                {
                    int index = ipLast - 1;
                    if (index >= 0 && index < 255)
                    {
                        if (info.IsDhcpServer)
                            _ipGrid.SetIpColor(index, Color.FromArgb(255, 138, 128));
                        else if (info.IsActive)
                            _ipGrid.SetIpColor(index, Color.FromArgb(33, 150, 243));
                        else
                            _ipGrid.SetIpColor(index, Color.FromArgb(76, 175, 80));
                    }
                }
            }
        }

        /// <summary>
        /// 获取当前表格所有行数据（用于导出）
        /// </summary>
        public IReadOnlyList<DataGridViewRow> GetRows()
        {
            return _grid.Rows.Cast<DataGridViewRow>().ToList().AsReadOnly();
        }

        /// <summary>
        /// 切换语言后刷新列头和单元格文本
        /// </summary>
        public void RefreshLanguage()
        {
            _grid.Columns["IpAddress"].HeaderText = Lang.Get("ColIp");
            _grid.Columns["MacAddress"].HeaderText = Lang.Get("ColMac");
            _grid.Columns["HostName"].HeaderText = Lang.Get("ColHost");
            _grid.Columns["PingMs"].HeaderText = Lang.Get("ColPing");
            _grid.Columns["IsRouter"].HeaderText = Lang.Get("ColDhcp");
            _grid.Columns["Status"].HeaderText = Lang.Get("ColStatus");

            // 刷新单元格中的状态文本（DHCP 是/否、在线/无设备）
            foreach (DataGridViewRow row in _grid.Rows)
            {
                if (row.Tag is not DhcpServerInfo info) continue;
                row.Cells["IsRouter"].Value = info.IsDhcpServer ? Lang.Get("Yes") : Lang.Get("No");
                row.Cells["Status"].Value = info.IsActive ? Lang.Get("Online") : Lang.Get("NoDevice");
            }

            _ipGrid.Invalidate();
        }

        /// <summary>
        /// IP分布图格子单击：跳转到表格中对应IP行并高亮
        /// </summary>
        private void IpGrid_CellClicked(int ipIndex)
        {
            int ipLast = ipIndex + 1;
            string suffix = "." + ipLast;

            foreach (DataGridViewRow row in _grid.Rows)
            {
                string ip = row.Cells["IpAddress"].Value?.ToString() ?? "";
                if (!ip.EndsWith(suffix, StringComparison.Ordinal)) continue;

                _grid.ClearSelection();
                _grid.CurrentCell = row.Cells["IpAddress"];
                row.Selected = true;
                _grid.FirstDisplayedScrollingRowIndex = row.Index;
                return;
            }
        }

        /// <summary>
        /// IP分布图格子双击：跳转并弹出详情
        /// </summary>
        private void IpGrid_CellDoubleClicked(int ipIndex)
        {
            int ipLast = ipIndex + 1;
            string suffix = "." + ipLast;

            foreach (DataGridViewRow row in _grid.Rows)
            {
                string ip = row.Cells["IpAddress"].Value?.ToString() ?? "";
                if (!ip.EndsWith(suffix, StringComparison.Ordinal)) continue;

                _grid.ClearSelection();
                _grid.CurrentCell = row.Cells["IpAddress"];
                row.Selected = true;
                _grid.FirstDisplayedScrollingRowIndex = row.Index;
                ShowDetailDialog(row);
                return;
            }
        }

        private void Grid_CellDoubleClick(object? sender, DataGridViewCellEventArgs e)
        {
            if (e.RowIndex < 0) return;
            var row = _grid.Rows[e.RowIndex];
            ShowDetailDialog(row);
        }

        /// <summary>
        /// 弹出设备详情对话框
        /// </summary>
        private void ShowDetailDialog(DataGridViewRow row)
        {
            string ip = row.Cells["IpAddress"].Value?.ToString() ?? "";
            string mac = row.Cells["MacAddress"].Value?.ToString() ?? "";
            string host = row.Cells["HostName"].Value?.ToString() ?? "";
            string ping = row.Cells["PingMs"].Value?.ToString() ?? "";
            string dhcp = row.Cells["IsRouter"].Value?.ToString() ?? "";
            var info = row.Tag as DhcpServerInfo;
            bool isActive = info?.IsActive ?? false;
            bool isDhcp = info?.IsDhcpServer ?? false;

            var statusColor = isDhcp ? Color.FromArgb(255, 138, 128) : isActive ? Color.FromArgb(33, 150, 243) : Color.FromArgb(158, 158, 158);

            using var form = new Form
            {
                Text = string.Format(Lang.Get("DeviceDetail"), ip),
                ClientSize = new Size(480, 420),
                FormBorderStyle = FormBorderStyle.FixedDialog,
                StartPosition = FormStartPosition.CenterParent,
                MaximizeBox = false,
                MinimizeBox = false,
                BackColor = Color.White,
                KeyPreview = true,
            };

            // 状态颜色条（顶部）
            var colorBar = new Panel
            {
                Dock = DockStyle.Top,
                Height = 6,
                BackColor = statusColor,
            };
            form.Controls.Add(colorBar);

            // 标题
            var lblTitle = new Label
            {
                Text = ip,
                Font = new Font("Microsoft YaHei", 14F, FontStyle.Bold),
                ForeColor = Color.FromArgb(40, 40, 40),
                AutoSize = true,
                Location = new Point(20, 20),
            };
            form.Controls.Add(lblTitle);

            // 状态标签
            var lblStatusTag = new Label
            {
                Text = isDhcp ? $"{Lang.Get("Online")}  |  {Lang.Get("ColDhcp")}" : isActive ? Lang.Get("Online") : Lang.Get("NoDevice"),
                Font = new Font("Microsoft YaHei", 9F, FontStyle.Bold),
                ForeColor = Color.White,
                BackColor = statusColor,
                AutoSize = true,
                Padding = new Padding(6, 2, 6, 2),
                Location = new Point(20, 48),
            };
            form.Controls.Add(lblStatusTag);

            int y = 86;
            var labelFont = new Font("Microsoft YaHei", 9.5F);
            var valueFont = new Font("Microsoft YaHei", 9.5F, FontStyle.Bold);

            string pingText = long.TryParse(ping, out long pingMs) && pingMs >= 0 ? $"{pingMs} ms" : "-";
            var fields = new (string label, string value, Color color)[]
            {
                (Lang.Get("FieldIp"), ip, Color.FromArgb(40, 40, 40)),
                (Lang.Get("FieldMac"), mac, Color.FromArgb(40, 40, 40)),
                (Lang.Get("FieldHost"), host, Color.FromArgb(40, 40, 40)),
                (Lang.Get("FieldPing"), pingText, pingText != "-" ? Color.FromArgb(46, 125, 50) : Color.Gray),
                (Lang.Get("ColDhcp"), dhcp, isDhcp ? Color.Red : Color.FromArgb(40, 40, 40)),
            };

            var infoTable = new TableLayoutPanel
            {
                Location = new Point(20, y),
                Size = new Size(form.ClientSize.Width - 40, fields.Length * 34),
                ColumnCount = 2,
                RowCount = fields.Length,
                CellBorderStyle = TableLayoutPanelCellBorderStyle.Single,
                BackColor = Color.FromArgb(245, 247, 249),
                Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right,
            };
            infoTable.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 105));
            infoTable.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            for (int i = 0; i < fields.Length; i++)
            {
                var (label, value, color) = fields[i];
                infoTable.RowStyles.Add(new RowStyle(SizeType.Absolute, 34));
                infoTable.Controls.Add(new Label
                {
                    Text = label,
                    Font = labelFont,
                    ForeColor = Color.FromArgb(100, 100, 100),
                    TextAlign = ContentAlignment.MiddleLeft,
                    Dock = DockStyle.Fill,
                    Padding = new Padding(10, 0, 4, 0),
                }, 0, i);
                infoTable.Controls.Add(new TextBox
                {
                    Text = value,
                    Font = valueFont,
                    ForeColor = color,
                    BorderStyle = BorderStyle.None,
                    ReadOnly = true,
                    BackColor = Color.FromArgb(245, 247, 249),
                    Dock = DockStyle.Fill,
                    Margin = new Padding(8, 7, 8, 4),
                    TabStop = true,
                }, 1, i);
            }
            form.Controls.Add(infoTable);
            y += infoTable.Height + 16;

            string copyDetailsText = Lang.Current switch
            {
                AppLanguage.English => "Copy details",
                AppLanguage.TraditionalChinese => "複製資訊",
                AppLanguage.TraditionalChineseHk => "複製資料",
                _ => "复制详情",
            };
            string copiedText = Lang.Current switch
            {
                AppLanguage.English => "Copied",
                AppLanguage.TraditionalChinese or AppLanguage.TraditionalChineseHk => "已複製",
                _ => "已复制",
            };
            var btnCopy = new Button
            {
                Text = copyDetailsText,
                FlatStyle = FlatStyle.System,
                Size = new Size(120, 35),
                Font = new Font("Microsoft YaHei", 9F),
                Cursor = Cursors.Hand,
            };
            btnCopy.Click += (_, _) =>
            {
                try
                {
                    Clipboard.SetText(string.Join(Environment.NewLine, fields.Select(f => $"{f.label}: {f.value}")));
                    btnCopy.Text = copiedText;
                    var timer = new System.Windows.Forms.Timer { Interval = 1400 };
                    timer.Tick += (_, _) => { timer.Stop(); timer.Dispose(); btnCopy.Text = copyDetailsText; };
                    timer.Start();
                }
                catch { }
            };

            // Ping按钮
            var btnPing = new Button
            {
                Text = "Ping",
                FlatStyle = FlatStyle.System,
                Size = new Size(120, 35),
                Font = new Font("Microsoft YaHei", 9F, FontStyle.Bold),
                Cursor = Cursors.Hand,
            };
            btnPing.Click += (_, _) =>
            {
                try
                {
                    System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
                    {
                        FileName = "cmd.exe",
                        Arguments = $"/k ping {ip} -t",
                        UseShellExecute = true,
                    });
                }
                catch { }
            };

            // 访问后台按钮（仅DHCP服务器显示）
            var btnWeb = new Button
            {
                Text = Lang.Get("AccessAdmin"),
                FlatStyle = FlatStyle.System,
                Size = new Size(120, 35),
                Font = new Font("Microsoft YaHei", 9F, FontStyle.Bold),
                Cursor = Cursors.Hand,
                Visible = isDhcp,
            };
            btnWeb.Click += (_, _) =>
            {
                try
                {
                    var psi = new System.Diagnostics.ProcessStartInfo
                    {
                        FileName = "explorer.exe",
                        Arguments = $"\"http://{ip}\"",
                        UseShellExecute = false,
                        CreateNoWindow = true
                    };
                    System.Diagnostics.Process.Start(psi);
                }
                catch { }
            };

            // IE访问后台按钮（仅DHCP服务器显示）
            var btnIe = new Button
            {
                Text = Lang.Get("IeAccess"),
                FlatStyle = FlatStyle.System,
                Size = new Size(120, 35),
                Font = new Font("Microsoft YaHei", 9F, FontStyle.Bold),
                Cursor = Cursors.Hand,
                Visible = isDhcp,
            };
            btnIe.Click += (_, _) =>
            {
                try
                {
                    // 完全复刻 VBScript 调用方式
                    var ieType = Type.GetTypeFromProgID("InternetExplorer.Application");
                    if (ieType == null)
                    {
                        MessageBox.Show(Lang.Get("IeNotRegistered"),
                            Lang.Get("Tip"), MessageBoxButtons.OK, MessageBoxIcon.Information);
                        return;
                    }
                    dynamic ie = Activator.CreateInstance(ieType)!;
                    ie!.Navigate("about:blank");
                    ie.Visible = 1;
                    // 延迟跳转，等待 IE 窗口显示
                    var timer = new System.Windows.Forms.Timer { Interval = 500 };
                    timer.Tick += (_, _) =>
                    {
                        timer.Stop();
                        try { ie.Navigate($"http://{ip}"); } catch { }
                        try { System.Runtime.InteropServices.Marshal.ReleaseComObject(ie); } catch { }
                    };
                    timer.Start();
                }
                catch (Exception ex)
                {
                    MessageBox.Show(string.Format(Lang.Get("IeLaunchFailed"), ex.Message),
                        Lang.Get("Tip"), MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            };

            // 按钮排列（两行）
            var btnClose = new Button
            {
                Text = Lang.Get("Close"),
                FlatStyle = FlatStyle.System,
                Size = new Size(120, 35),
                Font = new Font("Microsoft YaHei", 9F, FontStyle.Bold),
                Cursor = Cursors.Hand,
                DialogResult = DialogResult.Cancel,
            };

            int btnW = 120, btnH = 35, gap = 10;
            var allBtns = new List<Button> { btnCopy, btnPing, btnWeb, btnIe, btnClose };
            var visibleBtns = allBtns.Where(b => b.Visible).ToList();
            int cols = Math.Min(3, visibleBtns.Count);
            int rows = (visibleBtns.Count + cols - 1) / cols;
            int btnY = y;
            form.ClientSize = new Size(form.ClientSize.Width, btnY + rows * btnH + (rows - 1) * gap + 20);

            for (int i = 0; i < visibleBtns.Count; i++)
            {
                int r = i / cols;
                int c = i % cols;
                int rowCount = Math.Min(cols, visibleBtns.Count - r * cols);
                int rowWidth = rowCount * btnW + (rowCount - 1) * gap;
                int rowStartX = (form.ClientSize.Width - rowWidth) / 2;
                visibleBtns[i].Location = new Point(rowStartX + c * (btnW + gap), btnY + r * (btnH + gap));
                form.Controls.Add(visibleBtns[i]);
            }

            form.CancelButton = btnClose;
            form.ShowDialog(FindForm());
        }
    }
}
