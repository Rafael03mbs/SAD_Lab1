using System.IO;
using System.IO.Ports;
using System.Net.Http;
using System.Text;
using System.Text.Json;

namespace DADApp
{
    public partial class Form1 : Form
    {
        private SerialPort serialPort1 = null!;
        private readonly object serialBufferLock = new();
        private readonly object appLogLock = new();
        private string serialBuffer = string.Empty;
        private string selectedXmlSensor = "Ax";
        private volatile bool isDisconnecting;
        private const int MaxSerialBufferLength = 4096;
        private readonly string appLogPath = Path.Combine(AppContext.BaseDirectory, "dados_sensores.log");

        // Contadores para diagnóstico I2C
        private int i2cOkCount;
        private int i2cErrCount;

        // HttpClient estático preparado para o Passo 3
        private static readonly HttpClient httpClient = new HttpClient();

        public Form1()
        {
            InitializeComponent();
            ConfigureSerialPort();
            selectedXmlSensor = cmbXmlSensor.SelectedItem?.ToString() ?? "Ax";
            LoadAvailablePorts();
            LogMessage("Log detalhado: " + appLogPath);

            // Bind control events for instant configuration updates
            numP.ValueChanged += (s, e) => SendConfiguration();
            numN.ValueChanged += (s, e) => SendConfiguration();
            chkAx.CheckedChanged += (s, e) => SendConfiguration();
            chkAy.CheckedChanged += (s, e) => SendConfiguration();
            chkSD0.CheckedChanged += (s, e) => SendConfiguration();
            chkSD1.CheckedChanged += (s, e) => SendConfiguration();
            chkD6.CheckedChanged += (s, e) => SendConfiguration();
            chkD7.CheckedChanged += (s, e) => SendConfiguration();
            chkDB.CheckedChanged += (s, e) => SendConfiguration();
            chkDV.CheckedChanged += (s, e) => SendConfiguration();
        }

        // =====================================================================
        //  Serial Port Setup
        // =====================================================================

        private void ConfigureSerialPort()
        {
            serialPort1 = new SerialPort();
            serialPort1.BaudRate = 9600;
            serialPort1.NewLine = "\n";
            serialPort1.ReadTimeout = 500;
            serialPort1.WriteTimeout = 500;
            serialPort1.DataReceived += SerialPort1_DataReceived;
        }

        // =====================================================================
        //  UI Event Handlers
        // =====================================================================

        private void btnRefresh_Click(object? sender, EventArgs e)
        {
            LoadAvailablePorts();
        }

        private void cmbXmlSensor_SelectedIndexChanged(object? sender, EventArgs e)
        {
            selectedXmlSensor = cmbXmlSensor.SelectedItem?.ToString() ?? "Ax";
        }

        private void SendConfiguration()
        {
            if (!serialPort1.IsOpen || isDisconnecting)
            {
                return;
            }

            int ax = chkAx.Checked ? 1 : 0;
            int ay = chkAy.Checked ? 1 : 0;
            int sd0 = chkSD0.Checked ? 1 : 0;
            int sd1 = chkSD1.Checked ? 1 : 0;
            int d6 = chkD6.Checked ? 1 : 0;
            int d7 = chkD7.Checked ? 1 : 0;
            int db = chkDB.Checked ? 1 : 0;
            int dv = chkDV.Checked ? 1 : 0;

            // Inclui todos os canais definidos no guião (Ax, Ay, Az, SD0, SD1, D6, D7, DB, DV)
            string jsonCmd = $"{{\"p\":{numP.Value}, \"n\":{numN.Value}, \"Ax\":{ax}, \"Ay\":{ay}, \"SD0\":{sd0}, \"SD1\":{sd1}, \"D6\":{d6}, \"D7\":{d7}, \"DB\":{db}, \"DV\":{dv}}}\n";

            try
            {
                serialPort1.Write(jsonCmd);
                AppendAppLog("[TX JSON] " + jsonCmd.Trim());
                LogMessage($"Configuração enviada: p={numP.Value}s, n={numN.Value}, XML={selectedXmlSensor}");
            }
            catch (Exception ex)
            {
                AppendAppLog("[ERRO TX] " + ex.Message);
                LogMessage("Erro ao enviar configuracao: " + ex.Message);
            }
        }

        // =====================================================================
        //  Serial Port Management
        // =====================================================================

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

        private async void BtnConnect_Click(object? sender, EventArgs e)
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

                    isDisconnecting = false;
                    serialPort1.DataReceived -= SerialPort1_DataReceived;
                    serialPort1.DataReceived += SerialPort1_DataReceived;
                    serialPort1.PortName = cmbPorts.SelectedItem.ToString();
                    serialPort1.Open();
                    try { serialPort1.DiscardInBuffer(); } catch (Exception) { }
                    ResetSerialBuffer();
                    i2cOkCount = 0;
                    i2cErrCount = 0;
                    btnConnect.Text = "Desconectar";
                    LogMessage("Ligado a " + serialPort1.PortName + " (BaudRate: " + serialPort1.BaudRate + ")");
                    SendConfiguration();
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Erro ao abrir a porta: " + ex.Message, "Erro", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            else
            {
                await DisconnectSerialPortAsync();
            }
        }

        // =====================================================================
        //  Serial Data Reception & Processing
        // =====================================================================

        private void SerialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                if (isDisconnecting)
                {
                    return;
                }

                var port = (SerialPort)sender;
                if (!port.IsOpen)
                {
                    return;
                }

                string incoming = port.ReadExisting();
                foreach (string data in ExtractJsonMessages(incoming))
                {
                    ProcessSerialJsonMessage(data);
                }
            }
            catch (InvalidOperationException)
            {
                // The port can be closed while DataReceived is still unwinding.
            }
            catch (IOException ex)
            {
                if (!isDisconnecting)
                {
                    RunOnUiThread(() =>
                    {
                        LogMessage("Erro de comunicacao serie: " + ex.Message);
                    });
                }
            }
            catch (Exception ex)
            {
                if (!isDisconnecting)
                {
                    RunOnUiThread(() =>
                    {
                        LogMessage("Erro de processamento: " + ex.Message);
                    });
                }
            }
        }

        private void ProcessSerialJsonMessage(string data)
        {
            if (string.IsNullOrWhiteSpace(data) || isDisconnecting)
            {
                return;
            }

            try
            {
                using var doc = JsonDocument.Parse(data);
                var root = doc.RootElement;
                AppendAppLog("[RX JSON] " + data);

                // Handle config acknowledgement
                if (root.TryGetProperty("status", out var status))
                {
                    RunOnUiThread(() =>
                    {
                        LogMessage("DAD confirmou configuração: " + status.GetString());
                    });
                    return;
                }

                // Handle alert messages
                if (root.TryGetProperty("alert", out var alertProp))
                {
                    string alertMsg = alertProp.GetString() ?? "unknown";
                    RunOnUiThread(() =>
                    {
                        LogMessage($"⚠ ALERTA do DAD: {alertMsg}");
                    });
                    return;
                }

                // Process sensor data arrays
                string[] sensoresPossiveis = { "Ax", "Ay", "SD0", "SD1", "D6", "D7", "DB", "DV" };
                List<string> summaries = BuildSensorSummaries(root, sensoresPossiveis);

                // Track I2C health for the status panel
                UpdateI2CStatus(root);

                if (summaries.Count > 0)
                {
                    RunOnUiThread(() =>
                    {
                        LogMessage("Amostras recebidas: " + string.Join(" | ", summaries));
                    });
                }

                string selectedSensor = selectedXmlSensor;
                bool anySensorEnabled = chkAx.Checked || chkAy.Checked || chkSD0.Checked || chkSD1.Checked || chkD6.Checked || chkD7.Checked || chkDB.Checked || chkDV.Checked;

                if (selectedSensor != "Nenhum" && anySensorEnabled && TryBuildXmlForSensor(root, selectedSensor, out string xmlOutput))
                {
                    AppendAppLog("[XML " + selectedSensor + "]\n" + xmlOutput);

                    _ = SendDataToServerAsync(xmlOutput);
                }
                else if (selectedSensor != "Nenhum" && anySensorEnabled && summaries.Count > 0)
                {
                    RunOnUiThread(() =>
                    {
                        LogMessage("Sem amostras de " + selectedSensor + " para enviar por XML.");
                    });
                }
            }
            catch (JsonException ex)
            {
                AppendAppLog("[JSON IGNORADO] " + ex.Message);
            }
        }

        // =====================================================================
        //  I2C Diagnostic Helpers
        // =====================================================================

        private void UpdateI2CStatus(JsonElement root)
        {
            bool hasSD = false;
            bool allError = true;

            foreach (string key in new[] { "SD0", "SD1" })
            {
                if (!root.TryGetProperty(key, out var arr) || arr.ValueKind != JsonValueKind.Array)
                    continue;

                hasSD = true;
                foreach (var val in arr.EnumerateArray())
                {
                    if (val.ToString() != "65535")
                    {
                        allError = false;
                        break;
                    }
                }
                if (!allError) break;
            }

            if (!hasSD) return;

            if (allError)
            {
                i2cErrCount++;
            }
            else
            {
                i2cOkCount++;
                i2cErrCount = 0; // reset consecutive error counter
            }

            RunOnUiThread(() =>
            {
                if (allError)
                {
                    lblI2CStatus.Text = $"❌ Erro I2C ({i2cErrCount} consecutivos) — verificar ligações SDA/SCL e pull-ups";
                    lblI2CStatus.ForeColor = Color.Red;
                }
                else
                {
                    lblI2CStatus.Text = $"✅ I2C OK — {i2cOkCount} leituras bem sucedidas";
                    lblI2CStatus.ForeColor = Color.Green;
                }
            });
        }

        // =====================================================================
        //  Sensor Data Formatting
        // =====================================================================

        private static List<string> BuildSensorSummaries(JsonElement root, string[] sensors)
        {
            var summaries = new List<string>();

            foreach (string sensor in sensors)
            {
                if (!root.TryGetProperty(sensor, out var samples) || samples.ValueKind != JsonValueKind.Array)
                {
                    continue;
                }

                var values = new List<string>();
                foreach (var sample in samples.EnumerateArray())
                {
                    values.Add(sample.ToString());
                }

                if (values.Count == 0)
                {
                    continue;
                }

                bool allI2cErrors = (sensor == "SD0" || sensor == "SD1");
                foreach (string value in values)
                {
                    if (value != "65535")
                    {
                        allI2cErrors = false;
                        break;
                    }
                }

                if (allI2cErrors)
                {
                    summaries.Add($"{sensor}: erro I2C ({values.Count} amostra(s))");
                }
                else
                {
                    summaries.Add($"{sensor}: {FormatValues(values)}");
                }
            }

            return summaries;
        }

        private static string FormatValues(List<string> values)
        {
            int maxVisibleValues = 8;
            int visibleCount = Math.Min(values.Count, maxVisibleValues);
            string text = string.Join(", ", values.GetRange(0, visibleCount));

            if (values.Count > maxVisibleValues)
            {
                text += ", ...";
            }

            return $"{values.Count} amostra(s) [{text}]";
        }

        // =====================================================================
        //  XML Builder
        // =====================================================================

        private static bool TryBuildXmlForSensor(JsonElement root, string sensor, out string xmlOutput)
        {
            xmlOutput = string.Empty;

            if (!root.TryGetProperty(sensor, out var samples) || samples.ValueKind != JsonValueKind.Array)
            {
                return false;
            }

            var builder = new StringBuilder();
            builder.Append("62966<amostras>");
            builder.AppendLine();
            builder.AppendLine($"  <sensor nome=\"{sensor}\">");

            foreach (var sample in samples.EnumerateArray())
            {
                builder.AppendLine($"    <valor>{sample}</valor>");
            }

            builder.AppendLine("  </sensor>");
            builder.Append("</amostras>");

            xmlOutput = builder.ToString();
            return true;
        }

        // =====================================================================
        //  HTTP Communication
        // =====================================================================

        private async Task SendDataToServerAsync(string xmlPayload)
        {
            try
            {
                var content = new StringContent(xmlPayload, System.Text.Encoding.UTF8, "application/xml");

                
                string serverUrl = "http://193.136.120.133/~sad/";

                var response = await httpClient.PostAsync(serverUrl, content);
                AppendAppLog($"[HTTP POST] {(int)response.StatusCode} {response.ReasonPhrase}");

                RunOnUiThread(() =>
                {
                    LogMessage($"[HTTP POST] Resposta do Servidor: {(int)response.StatusCode} {response.ReasonPhrase}");
                });
            }
            catch (Exception ex)
            {
                AppendAppLog("[HTTP POST ERRO] " + ex.Message);
                RunOnUiThread(() =>
                {
                    LogMessage($"[HTTP POST] Falha ao contactar servidor: {ex.Message}");
                });
            }
        }

        // =====================================================================
        //  Serial Port Disconnect
        // =====================================================================

        private async Task DisconnectSerialPortAsync()
        {
            if (isDisconnecting)
            {
                return;
            }

            isDisconnecting = true;
            btnConnect.Enabled = false;

            try
            {
                serialPort1.DataReceived -= SerialPort1_DataReceived;

                await Task.Run(() =>
                {
                    if (!serialPort1.IsOpen)
                    {
                        return;
                    }

                    try { serialPort1.DiscardInBuffer(); } catch (Exception) { }
                    try { serialPort1.DiscardOutBuffer(); } catch (Exception) { }
                    serialPort1.Close();
                });

                ResetSerialBuffer();
                LogMessage("Desligado.");
            }
            catch (Exception ex)
            {
                LogMessage("Erro ao desligar porta serie: " + ex.Message);
            }
            finally
            {
                bool stillOpen = false;
                try { stillOpen = serialPort1.IsOpen; } catch (Exception) { }

                if (stillOpen)
                {
                    isDisconnecting = false;
                }

                btnConnect.Text = stillOpen ? "Desconectar" : "Conectar";
                btnConnect.Enabled = true;
            }
        }

        private void Form1_FormClosing(object? sender, FormClosingEventArgs e)
        {
            isDisconnecting = true;

            try
            {
                serialPort1.DataReceived -= SerialPort1_DataReceived;
                if (serialPort1.IsOpen)
                {
                    serialPort1.Close();
                }
                serialPort1.Dispose();
            }
            catch (Exception)
            {
                // Ignore shutdown errors so closing the form never gets stuck here.
            }
        }

        // =====================================================================
        //  Thread-safe UI helpers
        // =====================================================================

        private void RunOnUiThread(Action action)
        {
            if (IsDisposed || Disposing)
            {
                return;
            }

            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new Action(() =>
                    {
                        try
                        {
                            if (!IsDisposed && !Disposing)
                            {
                                action();
                            }
                        }
                        catch (ObjectDisposedException)
                        {
                        }
                        catch (InvalidOperationException)
                        {
                        }
                    }));
                }
                else
                {
                    action();
                }
            }
            catch (ObjectDisposedException)
            {
                // The form may be closing while background work is finishing.
            }
            catch (InvalidOperationException)
            {
                // The form may be closing while background work is finishing.
            }
        }

        // =====================================================================
        //  Serial Buffer / JSON Extraction
        // =====================================================================

        private List<string> ExtractJsonMessages(string incoming)
        {
            var messages = new List<string>();
            if (string.IsNullOrEmpty(incoming))
            {
                return messages;
            }

            lock (serialBufferLock)
            {
                serialBuffer += incoming.Replace("\0", "");

                if (serialBuffer.Length > MaxSerialBufferLength)
                {
                    int lastStart = serialBuffer.LastIndexOf('{');
                    serialBuffer = lastStart >= 0 ? serialBuffer.Substring(lastStart) : string.Empty;
                }

                while (true)
                {
                    int start = serialBuffer.IndexOf('{');
                    if (start < 0)
                    {
                        serialBuffer = string.Empty;
                        break;
                    }

                    if (start > 0)
                    {
                        serialBuffer = serialBuffer.Substring(start);
                    }

                    int end = FindJsonObjectEnd(serialBuffer);
                    if (end < 0)
                    {
                        break;
                    }

                    messages.Add(serialBuffer.Substring(0, end + 1));
                    serialBuffer = serialBuffer.Substring(end + 1);
                }
            }

            return messages;
        }

        private static int FindJsonObjectEnd(string text)
        {
            int depth = 0;
            bool inString = false;
            bool escaping = false;

            for (int i = 0; i < text.Length; i++)
            {
                char c = text[i];

                if (inString)
                {
                    if (escaping)
                    {
                        escaping = false;
                    }
                    else if (c == '\\')
                    {
                        escaping = true;
                    }
                    else if (c == '"')
                    {
                        inString = false;
                    }

                    continue;
                }

                if (c == '"')
                {
                    inString = true;
                }
                else if (c == '{')
                {
                    depth++;
                }
                else if (c == '}')
                {
                    depth--;
                    if (depth == 0)
                    {
                        return i;
                    }

                    if (depth < 0)
                    {
                        return -1;
                    }
                }
            }

            return -1;
        }

        private void ResetSerialBuffer()
        {
            lock (serialBufferLock)
            {
                serialBuffer = string.Empty;
            }
        }

        // =====================================================================
        //  Logging
        // =====================================================================

        private void AppendAppLog(string msg)
        {
            try
            {
                lock (appLogLock)
                {
                    File.AppendAllText(appLogPath, $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] {msg}\n");
                }
            }
            catch (Exception ex)
            {
                RunOnUiThread(() =>
                {
                    LogMessage("Erro ao escrever log: " + ex.Message);
                });
            }
        }

        private void LogMessage(string msg)
        {
            txtLog.AppendText($"[{DateTime.Now:HH:mm:ss}] {msg}\n");
            txtLog.ScrollToCaret(); // Faz scroll automático para a última mensagem
        }

        private void chkAz_CheckedChanged(object sender, EventArgs e)
        {

        }
    }
}
