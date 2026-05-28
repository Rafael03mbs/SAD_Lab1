namespace DADApp
{
    partial class Form1
{
    /// <summary>
    ///  Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    ///  Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
        if (disposing && (components != null))
        {
            components.Dispose();
        }
        base.Dispose(disposing);
    }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            grpConn = new GroupBox();
            lblPorts = new Label();
            cmbPorts = new ComboBox();
            btnRefresh = new Button();
            btnConnect = new Button();
            grpConfig = new GroupBox();
            lblP = new Label();
            numP = new NumericUpDown();
            lblN = new Label();
            numN = new NumericUpDown();
            chkAx = new CheckBox();
            chkAy = new CheckBox();
            chkSD0 = new CheckBox();
            chkSD1 = new CheckBox();
            chkD6 = new CheckBox();
            chkD7 = new CheckBox();
            chkDB = new CheckBox();
            chkDV = new CheckBox();
            lblXmlSensor = new Label();
            cmbXmlSensor = new ComboBox();
            grpI2CStatus = new GroupBox();
            lblI2CStatus = new Label();
            grpLogs = new GroupBox();
            txtLog = new RichTextBox();
            grpConn.SuspendLayout();
            grpConfig.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)numP).BeginInit();
            ((System.ComponentModel.ISupportInitialize)numN).BeginInit();
            grpI2CStatus.SuspendLayout();
            grpLogs.SuspendLayout();
            SuspendLayout();
            // 
            // grpConn
            // 
            grpConn.Controls.Add(lblPorts);
            grpConn.Controls.Add(cmbPorts);
            grpConn.Controls.Add(btnRefresh);
            grpConn.Controls.Add(btnConnect);
            grpConn.Location = new Point(17, 20);
            grpConn.Margin = new Padding(3, 4, 3, 4);
            grpConn.Name = "grpConn";
            grpConn.Padding = new Padding(3, 4, 3, 4);
            grpConn.Size = new Size(686, 87);
            grpConn.TabIndex = 0;
            grpConn.TabStop = false;
            grpConn.Text = "Ligação Série";
            // 
            // lblPorts
            // 
            lblPorts.AutoSize = true;
            lblPorts.Location = new Point(17, 37);
            lblPorts.Name = "lblPorts";
            lblPorts.Size = new Size(83, 20);
            lblPorts.TabIndex = 0;
            lblPorts.Text = "Porta COM:";
            // 
            // cmbPorts
            // 
            cmbPorts.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbPorts.FormattingEnabled = true;
            cmbPorts.Location = new Point(114, 33);
            cmbPorts.Margin = new Padding(3, 4, 3, 4);
            cmbPorts.Name = "cmbPorts";
            cmbPorts.Size = new Size(137, 28);
            cmbPorts.TabIndex = 1;
            // 
            // btnRefresh
            // 
            btnRefresh.Location = new Point(263, 32);
            btnRefresh.Margin = new Padding(3, 4, 3, 4);
            btnRefresh.Name = "btnRefresh";
            btnRefresh.Size = new Size(40, 37);
            btnRefresh.TabIndex = 2;
            btnRefresh.Text = "↻";
            btnRefresh.UseVisualStyleBackColor = true;
            btnRefresh.Click += btnRefresh_Click;
            // 
            // btnConnect
            // 
            btnConnect.Location = new Point(314, 32);
            btnConnect.Margin = new Padding(3, 4, 3, 4);
            btnConnect.Name = "btnConnect";
            btnConnect.Size = new Size(126, 37);
            btnConnect.TabIndex = 3;
            btnConnect.Text = "Conectar";
            btnConnect.UseVisualStyleBackColor = true;
            btnConnect.Click += BtnConnect_Click;
            // 
            // grpConfig
            // 
            grpConfig.Controls.Add(lblP);
            grpConfig.Controls.Add(numP);
            grpConfig.Controls.Add(lblN);
            grpConfig.Controls.Add(numN);
            grpConfig.Controls.Add(chkAx);
            grpConfig.Controls.Add(chkAy);
            grpConfig.Controls.Add(chkSD0);
            grpConfig.Controls.Add(chkSD1);
            grpConfig.Controls.Add(chkD6);
            grpConfig.Controls.Add(chkD7);
            grpConfig.Controls.Add(chkDB);
            grpConfig.Controls.Add(chkDV);
            grpConfig.Controls.Add(lblXmlSensor);
            grpConfig.Controls.Add(cmbXmlSensor);
            grpConfig.Location = new Point(17, 120);
            grpConfig.Margin = new Padding(3, 4, 3, 4);
            grpConfig.Name = "grpConfig";
            grpConfig.Padding = new Padding(3, 4, 3, 4);
            grpConfig.Size = new Size(686, 207);
            grpConfig.TabIndex = 1;
            grpConfig.TabStop = false;
            grpConfig.Text = "Configuração do DAD (PIC24)";
            // 
            // lblP
            // 
            lblP.AutoSize = true;
            lblP.Location = new Point(17, 37);
            lblP.Name = "lblP";
            lblP.Size = new Size(113, 20);
            lblP.TabIndex = 0;
            lblP.Text = "Período p (seg):";
            // 
            // numP
            // 
            numP.Location = new Point(154, 33);
            numP.Margin = new Padding(3, 4, 3, 4);
            numP.Name = "numP";
            numP.Size = new Size(57, 27);
            numP.TabIndex = 1;
            numP.Value = new decimal(new int[] { 1, 0, 0, 0 });
            // 
            // lblN
            // 
            lblN.AutoSize = true;
            lblN.Location = new Point(229, 37);
            lblN.Name = "lblN";
            lblN.Size = new Size(86, 20);
            lblN.TabIndex = 2;
            lblN.Text = "Amostras n:";
            // 
            // numN
            // 
            numN.Location = new Point(331, 33);
            numN.Margin = new Padding(3, 4, 3, 4);
            numN.Maximum = new decimal(new int[] { 50, 0, 0, 0 });
            numN.Minimum = new decimal(new int[] { 1, 0, 0, 0 });
            numN.Name = "numN";
            numN.Size = new Size(57, 27);
            numN.TabIndex = 3;
            numN.Value = new decimal(new int[] { 1, 0, 0, 0 });
            // 
            // chkAx
            // 
            chkAx.AutoSize = true;
            chkAx.Checked = true;
            chkAx.CheckState = CheckState.Checked;
            chkAx.Location = new Point(17, 80);
            chkAx.Margin = new Padding(3, 4, 3, 4);
            chkAx.Name = "chkAx";
            chkAx.Size = new Size(48, 24);
            chkAx.TabIndex = 4;
            chkAx.Text = "Ax";
            chkAx.UseVisualStyleBackColor = true;
            // 
            // chkAy
            // 
            chkAy.AutoSize = true;
            chkAy.Location = new Point(80, 80);
            chkAy.Margin = new Padding(3, 4, 3, 4);
            chkAy.Name = "chkAy";
            chkAy.Size = new Size(48, 24);
            chkAy.TabIndex = 5;
            chkAy.Text = "Ay";
            chkAy.UseVisualStyleBackColor = true;
            
            // 
            // chkSD0
            // 
            chkSD0.AutoSize = true;
            chkSD0.Checked = true;
            chkSD0.CheckState = CheckState.Checked;
            chkSD0.Location = new Point(206, 80);
            chkSD0.Margin = new Padding(3, 4, 3, 4);
            chkSD0.Name = "chkSD0";
            chkSD0.Size = new Size(58, 24);
            chkSD0.TabIndex = 7;
            chkSD0.Text = "SD0";
            chkSD0.UseVisualStyleBackColor = true;
            // 
            // chkSD1
            // 
            chkSD1.AutoSize = true;
            chkSD1.Checked = true;
            chkSD1.CheckState = CheckState.Checked;
            chkSD1.Location = new Point(274, 80);
            chkSD1.Margin = new Padding(3, 4, 3, 4);
            chkSD1.Name = "chkSD1";
            chkSD1.Size = new Size(58, 24);
            chkSD1.TabIndex = 8;
            chkSD1.Text = "SD1";
            chkSD1.UseVisualStyleBackColor = true;
            // 
            // chkD6
            // 
            chkD6.AutoSize = true;
            chkD6.Location = new Point(343, 80);
            chkD6.Margin = new Padding(3, 4, 3, 4);
            chkD6.Name = "chkD6";
            chkD6.Size = new Size(50, 24);
            chkD6.TabIndex = 9;
            chkD6.Text = "D6";
            chkD6.UseVisualStyleBackColor = true;
            // 
            // chkD7
            // 
            chkD7.AutoSize = true;
            chkD7.Location = new Point(406, 80);
            chkD7.Margin = new Padding(3, 4, 3, 4);
            chkD7.Name = "chkD7";
            chkD7.Size = new Size(50, 24);
            chkD7.TabIndex = 10;
            chkD7.Text = "D7";
            chkD7.UseVisualStyleBackColor = true;
            // 
            // chkDB
            // 
            chkDB.AutoSize = true;
            chkDB.Location = new Point(469, 80);
            chkDB.Margin = new Padding(3, 4, 3, 4);
            chkDB.Name = "chkDB";
            chkDB.Size = new Size(51, 24);
            chkDB.TabIndex = 11;
            chkDB.Text = "DB";
            chkDB.UseVisualStyleBackColor = true;
            // 
            // chkDV
            // 
            chkDV.AutoSize = true;
            chkDV.Location = new Point(531, 80);
            chkDV.Margin = new Padding(3, 4, 3, 4);
            chkDV.Name = "chkDV";
            chkDV.Size = new Size(51, 24);
            chkDV.TabIndex = 12;
            chkDV.Text = "DV";
            chkDV.UseVisualStyleBackColor = true;
            // 
            // lblXmlSensor
            // 
            lblXmlSensor.AutoSize = true;
            lblXmlSensor.Location = new Point(17, 124);
            lblXmlSensor.Name = "lblXmlSensor";
            lblXmlSensor.Size = new Size(130, 20);
            lblXmlSensor.TabIndex = 13;
            lblXmlSensor.Text = "Entrada para XML:";
            // 
            // cmbXmlSensor
            // 
            cmbXmlSensor.DropDownStyle = ComboBoxStyle.DropDownList;
            cmbXmlSensor.FormattingEnabled = true;
            cmbXmlSensor.Items.AddRange(new object[] { "Ax", "Ay", "SD0", "SD1", "D6", "D7", "DB", "DV", "Nenhum" });
            cmbXmlSensor.Location = new Point(154, 120);
            cmbXmlSensor.Margin = new Padding(3, 4, 3, 4);
            cmbXmlSensor.Name = "cmbXmlSensor";
            cmbXmlSensor.Size = new Size(97, 28);
            cmbXmlSensor.TabIndex = 14;
            cmbXmlSensor.SelectedIndexChanged += cmbXmlSensor_SelectedIndexChanged;
            // 
            // grpI2CStatus
            // 
            grpI2CStatus.Controls.Add(lblI2CStatus);
            grpI2CStatus.Location = new Point(17, 340);
            grpI2CStatus.Margin = new Padding(3, 4, 3, 4);
            grpI2CStatus.Name = "grpI2CStatus";
            grpI2CStatus.Padding = new Padding(3, 4, 3, 4);
            grpI2CStatus.Size = new Size(686, 60);
            grpI2CStatus.TabIndex = 2;
            grpI2CStatus.TabStop = false;
            grpI2CStatus.Text = "Estado I2C (Arduino)";
            // 
            // lblI2CStatus
            // 
            lblI2CStatus.AutoSize = true;
            lblI2CStatus.Location = new Point(17, 27);
            lblI2CStatus.Name = "lblI2CStatus";
            lblI2CStatus.Size = new Size(92, 20);
            lblI2CStatus.TabIndex = 0;
            lblI2CStatus.Text = "A aguardar...";
            // 
            // grpLogs
            // 
            grpLogs.Controls.Add(txtLog);
            grpLogs.Location = new Point(17, 413);
            grpLogs.Margin = new Padding(3, 4, 3, 4);
            grpLogs.Name = "grpLogs";
            grpLogs.Padding = new Padding(3, 4, 3, 4);
            grpLogs.Size = new Size(686, 360);
            grpLogs.TabIndex = 3;
            grpLogs.TabStop = false;
            grpLogs.Text = "Monitor de Dados Recebidos";
            // 
            // txtLog
            // 
            txtLog.Font = new Font("Consolas", 10F);
            txtLog.Location = new Point(17, 33);
            txtLog.Margin = new Padding(3, 4, 3, 4);
            txtLog.Name = "txtLog";
            txtLog.ReadOnly = true;
            txtLog.Size = new Size(651, 305);
            txtLog.TabIndex = 0;
            txtLog.Text = "";
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(8F, 20F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(725, 795);
            Controls.Add(grpLogs);
            Controls.Add(grpI2CStatus);
            Controls.Add(grpConfig);
            Controls.Add(grpConn);
            FormBorderStyle = FormBorderStyle.FixedSingle;
            Margin = new Padding(3, 4, 3, 4);
            MaximizeBox = false;
            Name = "Form1";
            Text = "App DAD - Monitorização";
            FormClosing += Form1_FormClosing;
            grpConn.ResumeLayout(false);
            grpConn.PerformLayout();
            grpConfig.ResumeLayout(false);
            grpConfig.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)numP).EndInit();
            ((System.ComponentModel.ISupportInitialize)numN).EndInit();
            grpI2CStatus.ResumeLayout(false);
            grpI2CStatus.PerformLayout();
            grpLogs.ResumeLayout(false);
            ResumeLayout(false);
        }

        #endregion

        private GroupBox grpConn;
    private Label lblPorts;
    private ComboBox cmbPorts;
    private Button btnRefresh;
    private Button btnConnect;
    private GroupBox grpConfig;
    private Label lblP;
    private NumericUpDown numP;
    private Label lblN;
    private NumericUpDown numN;
    private CheckBox chkAx;
    private CheckBox chkAy;
    private CheckBox chkSD0;
    private CheckBox chkSD1;
    private CheckBox chkD6;
    private CheckBox chkD7;
    private CheckBox chkDB;
    private CheckBox chkDV;
    private ComboBox cmbXmlSensor;
    private Label lblXmlSensor;
    private GroupBox grpI2CStatus;
    private Label lblI2CStatus;
    private GroupBox grpLogs;
    private RichTextBox txtLog;
}
}
