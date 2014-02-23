using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace USB2AX_Test {
    public partial class TestStep : UserControl {
        public TestStep() {
            InitializeComponent();
        }

        public String Description { 
            get{
                return lDescription.Text;
            }
            set {
                lDescription.Text = value;
            }
        }

        private StepViewState state = StepViewState.InProgress;
        public StepViewState State {
            get { return state; }
            set {
                if (this.InvokeRequired) {
                    Invoke(new Action(() => State = value));
                }
                else {
                    state = value;
                    switch (state) {
                        case StepViewState.Hidden:
                            lDescription.Visible = false;
                            lOK.Visible = false;
                            lError.Visible = false;
                            break;
                        case StepViewState.InProgress:
                            lDescription.Visible = true;
                            lOK.Visible = false;
                            lError.Visible = false;
                            break;
                        case StepViewState.OK:
                            lDescription.Visible = true;
                            lOK.Visible = true;
                            lError.Visible = false;
                            break;
                        case StepViewState.Error:
                            lDescription.Visible = true;
                            lOK.Visible = false;
                            lError.Visible = true;
                            break;
                    }
                }
            }
        }

        public int PercentUponCompletion { get; set; }

        // TODO move it to a common interface
        public delegate bool func();
        public func myFunc;

        public bool Run() {
            State = StepViewState.InProgress;

            bool res = myFunc();
            if (res) {
                State = StepViewState.OK;
            } else {
                State = StepViewState.Error;
            }
            return res;
        }

        public void Reset() {
            State = StepViewState.Hidden;
        }

    }

    public enum StepViewState {
        Hidden,
        InProgress,
        OK,
        Error
    }

}
