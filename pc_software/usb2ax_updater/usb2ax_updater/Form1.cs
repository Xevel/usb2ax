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

namespace usb2ax_updater {
    public partial class Form1 : Form {
        public Form1() {
            InitializeComponent();
        }

        /// <summary>
        /// Get a list of Plug'n'Play devices matching the pattern
        /// </summary>
        /// <param name="pattern"> Pattern contained in the device name. </param>
        /// <returns> Dictionnary of DeviceID, Device Name. </returns>
        public List<string> GetDevices(string pattern) {
            SelectQuery query = new SelectQuery("Win32_PnPEntity");
            ManagementObjectSearcher searcher = new ManagementObjectSearcher(query);

            List<string> list = new List<string>();
            foreach (ManagementObject res in searcher.Get()) {
                if (res != null && res["Name"] != null && res["Name"].ToString().Contains(pattern)) {
                    list.Add(res["Name"].ToString());
                }
            }
            return list;
        }

        public bool TestFirmware(string path) {
            try {
                File.Open(path, FileMode.Open).Close();
                return path.EndsWith(".hex");
            }
            catch (Exception) {
                return false;
            }
        }

        /// <summary>
        /// Detect if there is an ATmega32u2 DFU connected.
        /// </summary>
        /// <returns> true if a blank ATmega32u2 is successfully detected. </returns>
        public bool DetectATmega32u2() {
            return GetDevices("ATmega32U2").Count > 0;
        }

        public void program(string firmware_path) {
            if (
                RunDFUProgrammer("erase")
                && RunDFUProgrammer("flash \"" + firmware_path + "\"")
                && RunDFUProgrammer("start")) {
                MessageBox.Show(
                    "Programming successful.",
                    "Information",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information);
            }
        }

        public bool RunDFUProgrammer(string param) {
            string args = " atmega32u2 " + param ;

            //log("Running DFU-Programmer.");

            if (!File.Exists("dfu-programmer.exe")){
                MessageBox.Show(
                    "dfu-programmer.exe not found. It should be in the same folder as this executable!\n",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return false;
            }

            // run dfu-programmer to program the USB2AX
            Process p = new Process();
            p.StartInfo.FileName = "dfu-programmer.exe";
            p.StartInfo.Arguments = args;
            p.StartInfo.CreateNoWindow = true; // don't show any window
            p.StartInfo.UseShellExecute = false;   // needed to redirect any output
            p.StartInfo.RedirectStandardOutput = true; // redirect output
            p.StartInfo.RedirectStandardInput = true;
            p.StartInfo.RedirectStandardError = true;
            string output;
            try {
                p.Start();

                if (!p.WaitForExit(5000)) {
                    p.Kill();
                }
                output = p.StandardError.ReadToEnd();
                Console.WriteLine("***" + p.ExitCode + "***");
                //log("Exit code: " + p.ExitCode);
                Console.WriteLine(output);
            }
            catch (Exception e) {
                //err("");
                //log(e.ToString() + Environment.NewLine);
                MessageBox.Show(
                            "An error occured while trying to run dfu-programmer.\n",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;
            }

            if (p.ExitCode != 0) {
                //Console.WriteLine("...." + output + "....");
                //err("Not the expected output. Output :");
                //log(output + Environment.NewLine);
                MessageBox.Show(
                            "An unsuspected error occured while programming.\nOutput was:\n" + output,
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                return false;
            }
            else {
                //log("BatchISP OK.");
                return true;
            }
        }

        private void comboBox1_DropDown(object sender, EventArgs e) {
            comboBox1.Items.Clear();
            var ports = GetDevices("(COM");
            comboBox1.Items.AddRange(ports.ToArray());
        }

        private void button1_Click(object sender, EventArgs e) {
            int com_pos = comboBox1.Text.ToUpper().LastIndexOf("COM");
            if ( com_pos >= 0 ) {
                string number_str = comboBox1.Text.Substring(com_pos + 3);
                if (number_str.Length > 0){
                    foreach (var c in number_str) {
                        if ( !char.IsDigit(c)) {
                            number_str = number_str.Substring(0, number_str.IndexOf(c) );
                            break;
                        }
                    }
                    int com_number;
                    if (int.TryParse(number_str, out com_number)) {
                        string com_name = "COM" + com_number;

                        // if we get up to here, it means that we have a reasonably written COM port number
                        Console.WriteLine("Trying to open {0}...", com_name);

                        SerialPort ser = new SerialPort(com_name, 1200);
                        
                        try{
                            ser.Open();
                            byte[] bootload_message = {0xff, 0xff, 0xfd, 0x02, 0x08, 0xf8};
                            ser.Write(bootload_message, 0, bootload_message.Length);
                            //ser.DtrEnable = true; // DTR line is used since versions 04
                            
                        } catch (Exception){
                            MessageBox.Show(
                                "Could not open port " + com_name + ". Please check that it is available and not already in use.",
                                "Error",
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                        }
                        try {
                            ser.Close();
                        }
                        catch (Exception) {
                            // the usb2ax rebooted before we could close, no big deal.
                        }

                        return;
                    }
                }
            }

            MessageBox.Show(
                "\"" + comboBox1.Text + "\" is not a valid COM port name.",
                "Error",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);

        }

        private void button3_Click(object sender, EventArgs e) {
            openFileDialog1.FileName = Path.GetFileName(textBox1.Text);
            if (openFileDialog1.ShowDialog() == DialogResult.OK) {
                    textBox1.Text = openFileDialog1.FileName;
            }
        }

        private void button2_Click(object sender, EventArgs e) {
            if (TestFirmware(textBox1.Text)) {
                var atmegas = GetDevices("ATmega32U2");

                if (atmegas.Count != 0) {
                    // before installing the driver, the device appears as "ATmega32u2 DFU", and after driver is installed it appears as "ATmega32u2".
                    if ( ! atmegas[0].Contains("DFU") ){
                        
                        // TODO run dfu-programmer
                        program(textBox1.Text);

                    } else {
                        MessageBox.Show(
                            "ATmega32u2 DFU detected, but the driver for it is not installed. Please install the DFU driver manually.",
                            "Error",
                            MessageBoxButtons.OK,
                            MessageBoxIcon.Error);
                    }
                }
                else {
                    MessageBox.Show(
                        "No ATmega32u2 DFU detected. Please make sure that an USB2AX is present and running the DFU bootloader.",
                        "Error",
                        MessageBoxButtons.OK,
                        MessageBoxIcon.Error);
                }
            }
            else {
                MessageBox.Show(
                    "\"" + Path.GetFileName(textBox1.Text) + "\" is not a valid firmware (.hex) or could not be opened.",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e) {
            MessageBox.Show(
                "USB2AX Updater v1.1\nCopyright © 2017 Nicolas \"Xevel\" Saugnier\n\nwww.xevelabs.com\n\n"
                + "Permission is hereby granted, free of charge, to any person obtaining a copy "
                + "of this software and associated documentation files (the \"Software\"), to deal "
                + "in the Software without restriction, including without limitation the rights "
                + "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
                + "copies of the Software, and to permit persons to whom the Software is "
                + "furnished to do so, subject to the following conditions:\n\n"
                + "The above copyright notice and this permission notice shall be included in "
                + "all copies or substantial portions of the Software.\n\n"
                + "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
                + "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
                + "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
                + "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
                + "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
                + "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN "
                + "THE SOFTWARE.",
                "About...",
                MessageBoxButtons.OK,
                MessageBoxIcon.Asterisk);


        }

        private void openFileDialog1_FileOk(object sender, CancelEventArgs e)
        {

        }
    }
}
