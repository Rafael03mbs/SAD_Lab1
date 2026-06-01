namespace DADApp
{
    static class Program
    {
        /// <summary>
        ///  Ponto de entrada principal da aplicação.
        /// </summary>
        [STAThread]
        static void Main()
        {
            // Inicializa a configuração da aplicação.
            ApplicationConfiguration.Initialize();
            Application.Run(new Form1());
        }    
    }
}