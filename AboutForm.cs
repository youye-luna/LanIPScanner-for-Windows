using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace DhcpScanner
{
    /// <summary>
    /// 关于窗口（白色背景 + 居中路由器放大镜 Logo）
    /// </summary>
    public class AboutForm : Form
    {
        public AboutForm()
        {
            Text = Lang.Get("AboutTitle");
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            StartPosition = FormStartPosition.CenterParent;
            ClientSize = new Size(500, 360);
            BackColor = Color.White;
            DoubleBuffered = true;
            Font = new Font("Microsoft YaHei", 9F);

            try
            {
                var asm = System.Reflection.Assembly.GetExecutingAssembly();
                using var stream = asm.GetManifestResourceStream("DhcpScanner.app.ico");
                if (stream != null)
                    Icon = new Icon(stream);
            }
            catch { }
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            var g = e.Graphics;
            g.SmoothingMode = SmoothingMode.AntiAlias;
            g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;

            int cx = ClientSize.Width / 2;

            // === 左侧：图标（直接显示，无背景框） ===
            int logoSize = 110;
            int logoX = 38;
            int logoY = 30;

            // 绘制嵌入的图标图片（等比例缩放居中显示）
            using (var logo = LoadLogo())
            {
                if (logo != null)
                {
                    float ratio = Math.Min((float)logoSize / logo.Width, (float)logoSize / logo.Height);
                    int w = (int)(logo.Width * ratio);
                    int h = (int)(logo.Height * ratio);
                    var dest = new Rectangle(
                        logoX + (logoSize - w) / 2,
                        logoY + (logoSize - h) / 2,
                        w, h);
                    g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                    g.DrawImage(logo, dest);
                }
            }

            // === 右侧：文字信息 ===
            int textX = logoX + logoSize + 30;
            int textWidth = ClientSize.Width - textX - 40;

            // LanIPScanner（英文名）
            using var nameFont = new Font("Segoe UI Semibold", 17F, FontStyle.Bold);
            using var nameBrush = new SolidBrush(Color.FromArgb(50, 50, 50));
            g.DrawString("LanIPScanner", nameFont, nameBrush, textX, logoY + 6);

            // 局域网设备扫描工具（中文名）
            using var cnFont = new Font("Microsoft YaHei", 11F);
            using var cnBrush = new SolidBrush(Color.FromArgb(130, 130, 130));
            g.DrawString(Lang.Get("FormTitle"), cnFont, cnBrush, textX, logoY + 42);

            // 分隔线
            using var linePen = new Pen(Color.FromArgb(230, 232, 235), 1);
            g.DrawLine(linePen, textX, logoY + 76, textX + textWidth, logoY + 76);

            // 版本号
            using var verFont = new Font("Microsoft YaHei", 9.5F);
            using var verBrush = new SolidBrush(Color.FromArgb(150, 150, 150));
            string verText = string.Format(Lang.Get("AboutVersion"), "1.4");
            g.DrawString(verText, verFont, verBrush, textX, logoY + 90);

            // 功能简介
            using var descFont = new Font("Microsoft YaHei", 8.5F);
            using var descBrush = new SolidBrush(Color.FromArgb(100, 100, 100));
            int descY = logoY + 120;
            string[] features = Lang.Get("AboutFeatures").Split('|');
            foreach (var feat in features)
            {
                g.DrawString($"  {feat}", descFont, descBrush, textX, descY);
                descY += 18;
            }

            // 版权（底部居中）
            using var copyFont = new Font("Microsoft YaHei", 8.5F);
            using var copyBrush = new SolidBrush(Color.FromArgb(190, 190, 190));
            string copyText = $"Copyright © {DateTime.Now.Year}";
            var copySize = g.MeasureString(copyText, copyFont);
            g.DrawString(copyText, copyFont, copyBrush, cx - copySize.Width / 2, 320);
        }

        /// <summary>
        /// 加载嵌入的图标图片（图标.png）
        /// </summary>
        private static Image? LoadLogo()
        {
            try
            {
                var asm = System.Reflection.Assembly.GetExecutingAssembly();
                using var stream = asm.GetManifestResourceStream("DhcpScanner.AboutLogo.png");
                if (stream != null)
                    return Image.FromStream(stream);
            }
            catch { }
            return null;
        }
    }
}
