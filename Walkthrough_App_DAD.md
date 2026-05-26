# Documentação e Walkthrough: App DAD (C#)

Este documento descreve detalhadamente o funcionamento de cada módulo e função implementada na aplicação Windows Forms (C#) responsável por comunicar com o Dispositivo de Aquisição de Dados (PIC24).

---

## 1. Construtor e Inicialização da Interface
**Funções:** `Form1()` e `SetupUI()`

* **`Form1()`**: Ponto de entrada da interface gráfica. Chama `InitializeComponent()` (gerado pelo .NET) e de seguida invoca `SetupUI()` e `LoadAvailablePorts()`.
* **`SetupUI()`**: Responsável por desenhar programaticamente toda a interface gráfica.
  * **Zona 1 (Ligação Série):** Instancia a Label, ComboBox (para as portas) e Botões (Refresh e Conectar). Configura também as propriedades base do objeto `SerialPort` (BaudRate configurado para 115200) e associa o evento de receção assíncrona `DataReceived`.
  * **Zona 2 (Configuração do DAD):** Cria as caixas numéricas (`NumericUpDown`) para definir o período ($p$) e número de amostras ($n$), assim como as caixas de seleção (`CheckBoxes`) dinâmicas para habilitar ou desabilitar as diferentes entradas (Ax, Ay, Az, D6, D7).
  * **Zona 3 (Logs):** Instancia uma `RichTextBox` para imprimir os dados a chegar ou eventos enviados em tempo real.

---

## 2. Gestão da Porta Série (RS-232/USB)
**Funções:** `LoadAvailablePorts()` e `BtnConnect_Click()`

* **`LoadAvailablePorts()`**: É chamada ao arrancar a app e sempre que se clica no botão "↻". Utiliza `SerialPort.GetPortNames()` para ler os portos virtuais RS-232/USB ativos no Windows e preenche as opções da ComboBox.
* **`BtnConnect_Click()`**: 
  * Se a porta estiver fechada, tenta abrir a comunicação na porta selecionada. Em caso de sucesso, muda o texto do botão para "Desconectar".
  * Se a porta já estiver aberta, fecha a comunicação de forma segura, libertando a porta COM para o sistema operativo.

---

## 3. Envio do JSON de Configuração para o PIC24
**Função:** Evento `Click` do botão `btnSendConfig`

Sempre que o utilizador clica em "Atualizar Sensores", o código avalia o estado atual da Interface e gera uma string JSON correspondente aos comandos que o interpretador C do PIC24 (`process_json_config` no `main.c`) consegue interpretar.
1. Lê os inteiros introduzidos de $p$ e $n$.
2. Atribui valores numéricos binários (`1` ou `0`) consoante o estado ("Visto" / "Não Visto") das CheckBoxes.
3. Formata a string (ex: `{"p":1, "n":5, "Ax":1, "Ay":0, "Az":0, "D6":0, "D7":0}\n`).
4. Envia imediatamente para o PIC24 via `serialPort1.Write()`.

---

## 4. Receção, Armazenamento Local e "Parsing" JSON
**Função:** `SerialPort1_DataReceived()`

Esta é a rotina central do programa. Trata-se de um evento/interrupção que corre numa **Thread Secundária** do Windows. Isto garante que a Interface Gráfica principal não "encrava" enquanto processa as pesadas mensagens série.

Está dividida nas seguintes tarefas:

1. **Leitura Segura (`Invoke`):** 
   * Extrai a linha recebida usando `serialPort1.ReadLine()`. 
   * Para alterar e atualizar a caixa de texto dos Logs visualmente, tem de invocar `this.Invoke()`. Isto serve como uma ponte de comunicação segura para passar dados da thread secundária para a thread primária (a que detém os controlos visuais).
2. **Passo 1 do Guião (Armazenamento):** 
   * A string crua recebida é acompanhada de um *Timestamp* (Data/Hora) nativo do Windows.
   * Invoca `File.AppendAllText`, adicionando essa linha ao ficheiro local `dados_sensores.log` como forma de backup persistente.
3. **Passo 2 do Guião (Extração JSON para XML):** 
   * Utiliza a biblioteca `JsonDocument.Parse` (System.Text.Json) para transformar a string num objeto pesquisável.
   * Avalia todas as matrizes de amostras possíveis (Ax, Ay...). 
   * Para os sensores detetados, executa iterações sob os seus valores internos (`512, 513...`) e aloca-os em marcações de tags XML: `<valor>512</valor>`. 

---

## 5. Comunicação Externa (HTTP POST)
**Função:** `SendDataToServerAsync(string xmlPayload)`

Embora adormecida no código, esta rotina é chamada pela conversão de XML anterior e cumpre o Passo 3 exigido pelo guião.

1. Instancia uma carga útil (`StringContent`) baseada na string XML gerada. O cabeçalho da carga vai marcado como tipo `application/xml` em codificação UTF-8.
2. Faz o disparo (POST request) via instância global `httpClient.PostAsync()` ao endereço do servidor remoto estipulado: `http://193.136.120.133/~sad/`.
3. Aguarda resposta (Síncrono/Assíncrono via `await`).
4. Ao receber resposta (ex: Erro 404, ou Sucesso 200), invoca a informação de forma segura para o Log Visual do utilizador, certificando que os dados chegaram.
