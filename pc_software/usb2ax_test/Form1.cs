using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Management;
using System.IO.Ports;
using System.IO;
using System.Threading;
using System.Collections;

namespace USB2AX_Test
{
    

    public partial class Form1 : Form
    {
        string tester_DeviceId = "";
        string under_test_DeviceId = "";
        string under_test_port = "";
        SerialPort tester = new SerialPort();
        SerialPort under_test = new SerialPort();

        Int16 Atmel_VID = 0x03EB;
        Int16 ATmega32u2_DFU_PID = 0x2FF0;

        List<TestStep> steps = new List<TestStep>();


        public Form1(){
            InitializeComponent();

            // initialize test step list
            svAtmegaDetect.myFunc = DetectATmega32u2;
            steps.Add(svAtmegaDetect);
            svProgramming.myFunc = RunBatchisp;
            steps.Add(svProgramming);
            svUsb2axDetect.myFunc = DetectNewUSB2AX;
            steps.Add(svUsb2axDetect);
            svSerialOpen.myFunc = OpenNewUSB2AX;
            steps.Add(svSerialOpen);
            svSerialTest.myFunc = RunSerialTest;
            steps.Add(svSerialTest);
            svSerialClose.myFunc = CloseUsb2ax;
            steps.Add(svSerialClose);
            svUninstall.myFunc = RunDevcon;
            //steps.Add(svUninstall);

            start_logging();
        }

        private void Form1_Load(object sender, EventArgs e){
            tbDevconPath.Text = Properties.Settings.Default.DevconPath;
            tbBatchispPath.Text = Properties.Settings.Default.BatchispPath;
            tbFirmwarePath.Text = Properties.Settings.Default.FirmwarePath;
        }

        //******
        // logging
        //******

        string log_file_path = "usb2ax_test_log.txt";
                    
        public void start_logging() {
            DateTime now = DateTime.Now;
            log("USB2AX_Test started: " + now.ToLongDateString() + " @ " + now.ToLongTimeString());
        }

        public void log(string str) {
            try {
                File.AppendAllText(log_file_path, DateTime.Now.ToLongTimeString() + " " + str + Environment.NewLine);
                add_to_output(DateTime.Now.ToLongTimeString() + " " + str + Environment.NewLine);
            }
            catch (Exception) {
                // whatever
            }
        }

        public void err(string str) {
            log("ERROR: " + str);
        }
        public void warn(string str) {
            log("Warning: " + str);
        }


        /**********************************************************************
         * 
         * Test and setup
         * 
         * ********************************************************************/

        /// <summary>
        /// Run all tests and setup.
        /// </summary>
        /// <returns> true if all setup and tests are OK. </returns>
        public bool TestAll() {
            log("TEST All:");
            lUsb2axOk.Visible = SetupUsb2ax();
            //lDevconOk.Visible = TestDevcon(tbDevconPath.Text);
            lBatchispOk.Visible = TestBatchisp(tbBatchispPath.Text);
            lFirmwareOk.Visible = TestFirmware(tbFirmwarePath.Text);

            return lUsb2axOk.Visible /*&& lDevconOk.Visible */&& lBatchispOk.Visible && lFirmwareOk.Visible;
        }

        /// <summary>
        /// Test if DevCon can be ran successfully.
        /// </summary>
        /// <param name="path"> Path to devcon.exe </param>
        /// <returns> true if DevCon can be ran successfully. </returns>
        public bool TestDevcon(string path) {
            log("TEST Devcon");
            Process p = new Process();
            p.StartInfo.FileName = path;
            p.StartInfo.Arguments = "";
            p.StartInfo.CreateNoWindow = true; // don't show any window
            p.StartInfo.UseShellExecute = false; // needed to redirect any output
            p.StartInfo.RedirectStandardError = true; // redirect error
            string output;
            try {
                p.Start();
                if (!p.WaitForExit(1000)) {
                    p.Kill();
                }
                output = p.StandardError.ReadToEnd();
                Console.WriteLine(output);
            }
            catch (Exception e) {
                err("Failed running Devcon. Exception was:");
                log(e.ToString() + Environment.NewLine);
                MessageBox.Show(
                            "An error occured while trying to run DevCon.\nPlease verify the path to devcon.exe and that it can be run.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;  
            }
            if (!output.StartsWith("devcon.exe")) {
                err("Devcon ran, but did not answer as expected. Answer was:");
                log(output + Environment.NewLine);
                MessageBox.Show(
                            "DevCon did not answer as expected. Please verify the path to devcon.exe and that it can be run.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;
            }
            log("Test Devcon OK");
            return true;
        }

        /// <summary>
        /// Tests if BatchISP can be ran successfully.
        /// </summary>
        /// <param name="path"> Path to batchisp.exe </param>
        /// <returns> true if BatchISP can be ran successfully. </returns>
        public bool TestBatchisp(string path) {
            log("TEST BatchISP");
            Process p = new Process();
            p.StartInfo.FileName = path;
            p.StartInfo.Arguments = "-version";
            p.StartInfo.CreateNoWindow = true; // don't show any window
            p.StartInfo.UseShellExecute = false;   // needed to redirect ant output
            p.StartInfo.RedirectStandardOutput = true; // redirect output
            string output;
            try {
                p.Start();
                if (!p.WaitForExit(5000)) {
                    p.Kill();
                }
                output = p.StandardOutput.ReadToEnd();
                Console.WriteLine(output);
            }
            catch (Exception e) {
                err("Failed running BatchISP. Exception was:");
                log(e.ToString() + Environment.NewLine);
                MessageBox.Show(
                            "An error occured while trying to run BatchISP.\nPlease verify the path to batchisp.exe and that it can be run.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;
            }
            if (!output.StartsWith("batchisp")) {
                err("BatchISP ran, but did not answer as expected. Answer was:");
                log(output + Environment.NewLine);
                MessageBox.Show(
                            "BatchISP did not answer as expected. Please verify the path to batchisp.exe and that it can be run.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;
            }
            log("Test BatchISP OK");
            return true;
        }

        /// <summary>
        /// Tests if the firmware is OK.
        /// </summary>
        /// <param name="path"> Path to the firmware. </param>
        /// <returns> true if the firmware is OK (exists, is a file, and is a .hex). </returns>
        public bool TestFirmware(string path) {
            log("TEST Firmware");
            try {
                File.Open(path, FileMode.Open).Close();
                if (path.EndsWith(".hex")) {
                    log("Test Firmware OK. File is : " + path);
                    return true;
                }
                else {
                    err("Firmware is not a .hex... Selected file was: " + path);
                    MessageBox.Show(
                            "The Firmware is not a .hex file.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);

                    return false;
                }
            }catch(Exception e){
                err("Firmware could not be opened... Selected file was: " + path);
                log(e.ToString() + Environment.NewLine);
                MessageBox.Show(
                            "The Firmware file could not be opened.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;
            }
        }

        /// <summary>
        /// Set up the tester USB2AX.
        /// </summary>
        /// <returns> true if the USB2AX has been correctly detected and set up. </returns>
        public bool SetupUsb2ax() {
            log("TEST USB2AX");
            if (tester.IsOpen) {
                log("USB2AX already opened");
                return true;
            }
            
            var usb2axs = GetDevices("USB2AX (");
            switch (usb2axs.Count){
                case 0:
                    err("No USB2AX pugged in.");
                    MessageBox.Show(
                            "No USB2AX plugged-in, please plug one in.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                    break;
                case 1:
                    tester_DeviceId = usb2axs.Keys.ElementAt(0);

                    string port = "NOT YET DEFINED" ;
                    // open serial port, get it from usb2axs[usb2ax_tester]
                    try{
                        port = usb2axs[tester_DeviceId].Substring(8).Trim(')');
                        tester.PortName = port;
                        tester.BaudRate = 1000000;
                        tester.ReadTimeout = 100;
                        tester.Open();
                        lComPortName.Text = port;
                    } catch (Exception e){
                        err("Found an USB2AX but could not open it. Exception was:");
                        log(e.ToString() + Environment.NewLine);
                        MessageBox.Show(
                            "Found an USB2AX on port \"" + port + "\", but could not open it.\nMaybe it's already in use?",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                        return false;
                    }
                    return true;
                default:
                    err("Too many USB2AX: " + usb2axs.Count + " have been detected.");
                    MessageBox.Show(
                            "Too many USB2AX detected (" + usb2axs.Count + "), please plug only one in.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                    break;
            }
            return false;
        }

        /// <summary>
        /// Get a list of Plug'n'Play devices matching the pattern
        /// </summary>
        /// <param name="pattern"> Pattern contained in the device name. </param>
        /// <returns> Dictionnary of DeviceID, Device Name. </returns>
        public Dictionary<string,string> GetDevices(string pattern){
            SelectQuery query = new SelectQuery("Select * from Win32_PnPEntity");
            ManagementObjectSearcher searcher = new ManagementObjectSearcher(query);

            Dictionary<string,string> list = new Dictionary<string,string>();
            foreach (ManagementObject res in searcher.Get())
            {
                if (res["Name"].ToString().Contains(pattern))
                {
                    list.Add(res["DeviceID"].ToString(),res["Name"].ToString());
                }
            }
            return list;
        }

        /// <summary>
        /// Detect if there is an ATmega32u2 DFU connected.
        /// </summary>
        /// <returns> true if a blank ATmega32u2 is successfully detected. </returns>
        public bool DetectATmega32u2() {
            return GetDevices("ATmega32U2").Count > 0;
        }

        /// <summary>
        /// Program the ATmega32u2 with the firmware using BatchISP.
        /// </summary>
        /// <returns> true if the ATmega32u2 has been successfully programmed. </returns>
        public bool RunBatchisp() {
            string args = "-device atmega32u2 -hardware usb "
                + "-operation erase f memory flash blankcheck loadbuffer "
                + "\"" + tbFirmwarePath.Text + "\"" + " program verify start reset 0";

            log("Running BatchISP.");

            // run batchisp to program the USB2AX
            Process p = new Process();
            p.StartInfo.FileName = tbBatchispPath.Text;
            p.StartInfo.Arguments = args;
            p.StartInfo.CreateNoWindow = true; // don't show any window
            p.StartInfo.UseShellExecute = false;   // needed to redirect any output
            p.StartInfo.RedirectStandardOutput = true; // redirect output
            p.StartInfo.RedirectStandardInput = true;
            string output;
            try {
                p.Start();

                if (!p.WaitForExit(5000)) {
                    p.Kill();
                }
                output = p.StandardOutput.ReadToEnd();
                //Console.WriteLine("***" + p.ExitCode + "***");
                //log("Exit code: " + p.ExitCode);
                //Console.WriteLine(output);
            }
            catch (Exception e) {
                err("");
                log(e.ToString() + Environment.NewLine);
                MessageBox.Show(
                            "An error occured while trying to run BatchISP.\n",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;
            }
            if (!output.Trim().EndsWith("Summary:  Total 11   Passed 11   Failed 0")) {
                //Console.WriteLine("...." + output + "....");
                err("Not the expected output. Output :");
                log(output + Environment.NewLine);
                MessageBox.Show(
                            "En error occured while programming.\nTODO manage this error.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;
            }
            else {
                log("BatchISP OK.");
                return true;
            }
        }

        /// <summary>
        /// Detect the new USB2AX, and wait for its driver to be installed.
        /// </summary>
        /// <returns> true if a new USB2AX has been successfully detected. </returns>
        public bool DetectNewUSB2AX() {
            DateTime stop_time = DateTime.Now + new TimeSpan(0, 0, 60);
            log("Detecting new USB2AX...");
            do {
                var usb2axs = GetDevices("USB2AX (");
                switch (usb2axs.Count) {
                    case 0:
                        err("Detect USB2AX : no more USB2AX connected !!!");
                        // Error No USB2AX anymore
                        MessageBox.Show(
                                "No USB2AX Plugged anymore!\nPlease restart the application...",
                                "Error",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                        return false;
                    case 1:
                        // That's OK, Waiting to detect the new USB2AX...
                        break;
                    case 2:
                        under_test_DeviceId = usb2axs.Keys.ElementAt(0);
                        if (under_test_DeviceId == tester_DeviceId) {
                            under_test_DeviceId = usb2axs.Keys.ElementAt(1);
                        }

                        // save serial port, get it from usb2axs[usb2ax_tester]
                        under_test_port = usb2axs[under_test_DeviceId].Substring(8).Trim(')');
                        log("Detected new USB2AX : " + under_test_port);
                        return true;
                    default:
                        err("Too many USB2AX detected: " + usb2axs.Count);
                        // Error too many USB2AXs at once!
                        MessageBox.Show(
                                "Found too many USB2AXs...\n",
                                "Error",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                        return false;
                }
                Thread.Sleep(250);
            } while (DateTime.Now < stop_time);
            err("Timeout.");
            return false; //Timeout
        }

        /// <summary>
        /// Open the newly detected USB2AX.
        /// </summary>
        /// <returns> true if the serial port has been opened successfully. </returns>
        public bool OpenNewUSB2AX() {
            log("Opening new USB2AX : " + under_test_port);
            // open serial port, get it from usb2axs[usb2ax_tester]
            try {
                under_test.PortName = under_test_port;
                under_test.BaudRate = 1000000;
                under_test.ReadTimeout = 100;
                under_test.Open();
            }
            catch (Exception e) {
                err("could not open. Exception was :");
                log(e.ToString() + Environment.NewLine);
                MessageBox.Show(
                    "Found a new USB2AX on port \"" + under_test_port + "\", but could not open it.\nMaybe it's already in use?",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return false;
            }
            log("Opened successfully.");
            return true;
        }

        /// <summary>
        ///  Test serial communications.
        /// </summary>
        /// <returns> true if serial communication is OK. </returns>
        public bool RunSerialTest() {
            // send and receive stuff to test that it works. Retry if needed.

            DialogResult res = DialogResult.None;
            do {
                log("Testing serial port...");
                try {
                    tester.DiscardOutBuffer();
                    tester.DiscardInBuffer();
                    under_test.DiscardInBuffer();
                    under_test.DiscardOutBuffer();

                    string message_1 = "Yo Wasup?";
                    string message_2 = "Not much. Watching the game, having a test.";
                    string message_3 = "True.";

                    tester.WriteLine(message_1);
                    Thread.Sleep(10);
                    if (under_test.ReadLine().Trim() != message_1) {
                        err("Failed at message 1");
                        return false;
                    }

                    under_test.WriteLine(message_2);
                    Thread.Sleep(10);
                    if (tester.ReadLine().Trim() != message_2) {
                        err("Failed at message 2");
                        return false;
                    }

                    tester.WriteLine(message_3);
                    Thread.Sleep(10);
                    if (under_test.ReadLine().Trim() != message_3) {
                        err("Failed at message 3");
                        return false;
                    }
                    return true;
                }
                catch (TimeoutException) {
                    err("Timeout...");
                    res = MessageBox.Show(
                                    "Timeout when trying to communicate between boards\nPlease verify if the cable is plugged and if it is not damaged.",
                                    "Error",
                                    MessageBoxButtons.RetryCancel,
                                    MessageBoxIcon.Error);
                }
                catch (InvalidOperationException) {
                    err("Serial port not opened anymore.");
                    MessageBox.Show(
                                    "The serial port is not opened anymore...\nPlease report this error and restart the program.",
                                    "Error",
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                    return false;
                }
                catch (Exception e) {
                    err("Failed. Exception was:");
                    log(e.ToString() + Environment.NewLine);
                    MessageBox.Show(
                                    "An error occured :"+ e.Message +"\n\nPlease report this error and restart the program.",
                                    "Error",
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                    return false;
                }
            } while (res == DialogResult.Retry);
            return false;
        }

        /// <summary>
        /// Close the Serial port of the USB2AX under test.
        /// </summary>
        /// <returns> Always return true. Errors are silently ignored. </returns>
        public bool CloseUsb2ax() {
            try {
                under_test.Close();
            } catch(Exception e){
                warn("Could not close properly. Exception was :");
                log(e.ToString() + Environment.NewLine);
               // silently ignore errors, they should not matter for the rest of the test procedure.
            }
            return true;
        }



        /// <summary>
        ///  Uninstall the device under test.
        /// </summary>
        /// <returns> true if device successfully uninstalled. </returns>
        public bool RunDevcon(){
            string args = " remove @\"" + under_test_DeviceId + "\"" ;

            log("Running DevCon with args: " + args);

            Process p = new Process();
            p.StartInfo.FileName = tbDevconPath.Text;
            p.StartInfo.Arguments = args;
            p.StartInfo.CreateNoWindow = true; // don't show any window
            p.StartInfo.UseShellExecute = false; // needed to redirect any output
            p.StartInfo.RedirectStandardError = true; // redirect error
            p.StartInfo.RedirectStandardOutput = true; // redirect output
            string output;
            try {
                p.Start();
                if (!p.WaitForExit(5000)) {
                    p.Kill();
                }
                output = p.StandardOutput.ReadToEnd();
                Console.Write(output);
            }
            catch (Exception e) {
                err("Failed to run Devcon. Exception was:");
                log(e.ToString() + Environment.NewLine);
                MessageBox.Show(
                            "An error occured while trying to run DevCon.\nPlease verify the path to devcon.exe and that it can be run.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;  
            }
            if (!output.Trim().EndsWith("1 device(s) were removed.")) {
                err("Ran devcon, but the output is not what was expected. Output was :");
                log(output + Environment.NewLine);
                MessageBox.Show(
                            "DevCon did not answer as expected. Please verify the path to devcon.exe and that it can be run.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;
            }
            log("DevCon OK");
            return true;
        }



        private void backgroundWorker1_DoWork(object sender, DoWorkEventArgs e) {
            EnableSteps(true);

            while (!backgroundWorker1.CancellationPending) {

                InitSteps();

                foreach (var s in steps) {
                    if (backgroundWorker1.CancellationPending) {
                        return;
                    }
                    if (!s.Run()) {
                        break;
                    }
                    SetProgress(s.PercentUponCompletion);
                }
                if (pbTesting.Value == 100) {
                    log("----------------  TEST OK ------------------");
                    Thread.Sleep(4000);
                }
            }
            EnableSteps(false);
        }



        /********************************************************************
         * 
         * Menus and GUI stuff
         * 
         * ******************************************************************/

        public void add_to_output(string str) {
            if (tbOutput.InvokeRequired) {
                Invoke(new Action(()=>add_to_output(str)));
            }
            else {
                tbOutput.AppendText(str);
                tbOutput.SelectionStart = tbOutput.Text.Length;
                tbOutput.ScrollToCaret();
                tbOutput.Refresh();
            }
        }

        private void InitSteps() {
            foreach (var s in steps) {
                s.Reset();
            }
            SetProgress(0);
        }
        
        public void EnableSteps(bool value) {
            if (gbSteps.InvokeRequired) {
                Invoke(new Action(() => EnableSteps(value)));
            }
            else {
                gbSteps.Enabled = value;
            }
        }

        protected void SetProgress(int percentage) {
            if (lProgressPercent.InvokeRequired) {
                Invoke(new Action(() => SetProgress(percentage)));
            }
            else {
                lProgressPercent.Text = percentage.ToString() + "%";
                pbTesting.Value = percentage;
            }
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e){
            MessageBox.Show("USB2AX Automated Test Application\nv2.2\nby Nicolas Saugnier aka Xevel\ne-mail: xevel@xevelabs.com", "About...", MessageBoxButtons.OK ,MessageBoxIcon.Information);
        }

        private void bStartTesting_Click(object sender, EventArgs e) {
            if (panelTest.Enabled) {
                tabControl1.SelectedTab = tabPageTest;
            }
            else {
                // Save paths, it's annoying to have to configure stuff over and over again.
                Properties.Settings.Default.DevconPath = tbDevconPath.Text;
                Properties.Settings.Default.BatchispPath = tbBatchispPath.Text;
                Properties.Settings.Default.FirmwarePath = tbFirmwarePath.Text;
                Properties.Settings.Default.Save();

                // Check if everything is setup properly
                if (TestAll()) {
                    panelTest.Enabled = true;
                    panelManualTest.Enabled = true;
                    tabControl1.SelectedTab = tabPageTest;
                    panelSetup.Enabled = false;
                    bStartTesting.Text = "Resume Testing";
                }
            }
        }

        private void bSearch_Click(object sender, EventArgs e) {
            // Try to search some files
            switch (openFileDialog1.ShowDialog()) {
                case DialogResult.OK:
                    if (sender == bSearchDevcon) {
                        tbDevconPath.Text = openFileDialog1.FileName;
                    }
                    else if (sender == bSearchBatchisp) {
                        tbBatchispPath.Text = openFileDialog1.FileName;
                    }
                    else if (sender == bSearchFirmware) {
                        tbFirmwarePath.Text = openFileDialog1.FileName;
                    }
                    break;
                default:
                    break;
            }
        }

        private void bRun_Click(object sender, EventArgs e) {
            backgroundWorker1.RunWorkerAsync();
        }

        private void bStop_Click(object sender, EventArgs e) {
            backgroundWorker1.CancelAsync();
        }

        private void bUpload_Click(object sender, EventArgs e) {
            
        }

        private void bErase_Click(object sender, EventArgs e) {

        }



    }
}


