using System.IO;
using System.IO.Ports;
using System.Net.Http;
using System.Text.Json;

namespace DADApp;

public partial class Form1 : Form
{
    private ComboBox cmbPorts;
    private Button btnConnect;
    private RichTextBox txtLog;
    private SerialPort serialPort1;
    
    // HttpClient estático preparado para o Passo 3
    private static readonly HttpClient httpClient = new HttpClient();

    public Form1()
    {
        InitializeComponent();
        SetupUI();
        LoadAvailablePorts();
    }

    private void SetupUI()
    {
        this.Text = "App DAD - Monitorização";
        this.Size = new Size(650, 550);
        this.FormBorderStyle = FormBorderStyle.FixedSingle; // Impede que a janela fique com aspeto estranho se for redimensionada
        this.MaximizeBox = false;

        // --- ZONA 1: Conexão ---
        GroupBox grpConn = new GroupBox() { Text = "Ligação Série", Location = new Point(15, 15), Size = new Size(600, 65) };
        this.Controls.Add(grpConn);

        Label lblPorts = new Label() { Text = "Porta COM:", Location = new Point(15, 28), AutoSize = true };
        grpConn.Controls.Add(lblPorts);

        cmbPorts = new ComboBox() { Location = new Point(100, 25), Size = new Size(120, 25), DropDownStyle = ComboBoxStyle.DropDownList };
        grpConn.Controls.Add(cmbPorts);

        Button btnRefresh = new Button() { Text = "↻", Location = new Point(230, 24), Size = new Size(35, 28) };
        btnRefresh.Click += (s, e) => LoadAvailablePorts();
        grpConn.Controls.Add(btnRefresh);

        btnConnect = new Button() { Text = "Conectar", Location = new Point(275, 24), Size = new Size(110, 28) };
        btnConnect.Click += BtnConnect_Click;
        grpConn.Controls.Add(btnConnect);

        // --- ZONA 2: Configuração do DAD (Passo 2) ---
        GroupBox grpConfig = new GroupBox() { Text = "Configuração do DAD (PIC24)", Location = new Point(15, 90), Size = new Size(600, 95) };
        this.Controls.Add(grpConfig);

        Label lblP = new Label() { Text = "Período p (seg):", Location = new Point(15, 28), AutoSize = true };
        grpConfig.Controls.Add(lblP);

        NumericUpDown numP = new NumericUpDown() { Location = new Point(135, 25), Size = new Size(50, 25), Minimum = 0, Maximum = 100, Value = 1 };
        grpConfig.Controls.Add(numP);

        Label lblN = new Label() { Text = "Amostras n:", Location = new Point(200, 28), AutoSize = true };
        grpConfig.Controls.Add(lblN);

        NumericUpDown numN = new NumericUpDown() { Location = new Point(290, 25), Size = new Size(50, 25), Minimum = 1, Maximum = 50, Value = 1 };
        grpConfig.Controls.Add(numN);

        // CheckBoxes para ativar/desativar cada sensor
        CheckBox chkAx = new CheckBox() { Text = "Ax", Location = new Point(15, 60), AutoSize = true, Checked = true };
        grpConfig.Controls.Add(chkAx);
        CheckBox chkAy = new CheckBox() { Text = "Ay", Location = new Point(80, 60), AutoSize = true };
        grpConfig.Controls.Add(chkAy);
        CheckBox chkAz = new CheckBox() { Text = "Az", Location = new Point(145, 60), AutoSize = true };
        grpConfig.Controls.Add(chkAz);
        CheckBox chkD6 = new CheckBox() { Text = "D6", Location = new Point(210, 60), AutoSize = true };
        grpConfig.Controls.Add(chkD6);
        CheckBox chkD7 = new CheckBox() { Text = "D7", Location = new Point(275, 60), AutoSize = true };
        grpConfig.Controls.Add(chkD7);

        Button btnSendConfig = new Button() { Text = "Atualizar Sensores", Location = new Point(360, 56), Size = new Size(160, 28) };
        btnSendConfig.Click += (s, e) => {
            if (serialPort1 != null && serialPort1.IsOpen) {
                // Prepara JSON suportando as flags do main.c
                int ax = chkAx.Checked ? 1 : 0;
                int ay = chkAy.Checked ? 1 : 0;
                int az = chkAz.Checked ? 1 : 0;
                int d6 = chkD6.Checked ? 1 : 0;
                int d7 = chkD7.Checked ? 1 : 0;

                string jsonCmd = $"{{\"p\":{numP.Value}, \"n\":{numN.Value}, \"Ax\":{ax}, \"Ay\":{ay}, \"Az\":{az}, \"D6\":{d6}, \"D7\":{d7}}}\n";
                serialPort1.Write(jsonCmd);
                LogMessage("Enviado: " + jsonCmd.Trim());
            } else {
                MessageBox.Show("Liga a porta série primeiro antes de enviar.", "Aviso", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        };
        grpConfig.Controls.Add(btnSendConfig);

        // --- ZONA 3: Receção de Dados ---
        // Empurrado para baixo (Y=195) devido ao aumento da Zona 2
        GroupBox grpLogs = new GroupBox() { Text = "Monitor de Dados Recebidos", Location = new Point(15, 195), Size = new Size(600, 300) };
        this.Controls.Add(grpLogs);

        txtLog = new RichTextBox() { Location = new Point(15, 25), Size = new Size(570, 260), ReadOnly = true, Font = new Font("Consolas", 10) };
        grpLogs.Controls.Add(txtLog);

        // Inicialização da Porta Série
        serialPort1 = new SerialPort();
        // NOTA: O BaudRate deve ser igual ao configurado no teu PIC24. (ex: 9600, 115200)
        serialPort1.BaudRate = 9600; 
        serialPort1.DataReceived += SerialPort1_DataReceived;
    }

    private void LoadAvailablePorts()
    {
        cmbPorts.Items.Clear();
        string[] ports = SerialPort.GetPortNames();
        cmbPorts.Items.AddRange(ports);
        if (ports.Length > 0)
        {
            cmbPorts.SelectedIndex = 0;
        }
    }

    private void BtnConnect_Click(object? sender, EventArgs e)
    {
        if (!serialPort1.IsOpen)
        {
            try
            {
                if (cmbPorts.SelectedItem == null)
                {
                    MessageBox.Show("Por favor, selecione uma porta COM.");
                    return;
                }

                serialPort1.PortName = cmbPorts.SelectedItem.ToString();
                serialPort1.Open();
                btnConnect.Text = "Desconectar";
                LogMessage("Ligado a " + serialPort1.PortName + " (BaudRate: " + serialPort1.BaudRate + ")");
            }
            catch (Exception ex)
            {
                MessageBox.Show("Erro ao abrir a porta: " + ex.Message, "Erro", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        else
        {
            serialPort1.Close();
            btnConnect.Text = "Conectar";
            LogMessage("Desligado.");
        }
    }

    private void SerialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
    {
        try
        {
            // Lê a string enviada pelo PIC24
            string data = serialPort1.ReadLine().TrimEnd();
            
            this.Invoke(new Action(() => {
                LogMessage("Rx: " + data);
            }));

            // ==========================================
            // PASSO 1: ARMAZENAMENTO LOCAL
            // ==========================================
            string filePath = "dados_sensores.log";
            // Grava a amostra num ficheiro de texto, adicionando a hora
            File.AppendAllText(filePath, $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] {data}\n");

            // ==========================================
            // PASSO 2: PARSING JSON E CONVERSÃO PARA XML
            // ==========================================
            // Tenta processar apenas se a mensagem for um JSON válido
            if (data.StartsWith("{") && data.EndsWith("}"))
            {
                using (var doc = JsonDocument.Parse(data))
                {
                    var root = doc.RootElement;
                    
                    // Constrói o início do XML
                    string xmlOutput = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<amostras>\n";

                    // Percorre todos os sensores que o PIC24 possa ter enviado
                    string[] sensoresPossiveis = { "Ax", "Ay", "Az", "SD0", "SD1", "D6", "D7" };
                    bool temDados = false;

                    foreach (var sensor in sensoresPossiveis)
                    {
                        if (root.TryGetProperty(sensor, out var arrayAmostras) && arrayAmostras.ValueKind == JsonValueKind.Array)
                        {
                            temDados = true;
                            xmlOutput += $"  <sensor nome=\"{sensor}\">\n";
                            foreach (var amostra in arrayAmostras.EnumerateArray())
                            {
                                xmlOutput += $"    <valor>{amostra}</valor>\n";
                            }
                            xmlOutput += $"  </sensor>\n";
                        }
                    }

                    xmlOutput += "</amostras>";

                    if (temDados)
                    {
                        this.Invoke(new Action(() => {
                            LogMessage("XML Gerado (Pronto para a rede):\n" + xmlOutput);
                        }));

                        // ==========================================
                        // PASSO 3: COMUNICAÇÃO DE REDE (HTTP POST)
                        // ==========================================
                        _ = SendDataToServerAsync(xmlOutput);
                    }
                }
            }
        }
        catch (Exception ex)
        {
            if (serialPort1.IsOpen) 
            {
                this.Invoke(new Action(() => {
                    LogMessage("Erro de processamento: " + ex.Message);
                }));
            }
        }
    }

    private async Task SendDataToServerAsync(string xmlPayload)
    {
        try
        {
            var content = new StringContent(xmlPayload, System.Text.Encoding.UTF8, "application/xml");
            
            // O IP do servidor do Professor indicado no guião
            string serverUrl = "http://193.136.120.133/~sad/";
            
            var response = await httpClient.PostAsync(serverUrl, content);
            
            this.Invoke(new Action(() => {
                LogMessage($"[HTTP POST] Resposta do Servidor: {(int)response.StatusCode} {response.ReasonPhrase}");
            }));
        }
        catch (Exception ex)
        {
            this.Invoke(new Action(() => {
                LogMessage($"[HTTP POST] Falha ao contactar servidor: {ex.Message}");
            }));
        }
    }

    private void LogMessage(string msg)
    {
        txtLog.AppendText($"[{DateTime.Now:HH:mm:ss}] {msg}\n");
        txtLog.ScrollToCaret(); // Faz scroll automático para a última mensagem
    }
}
