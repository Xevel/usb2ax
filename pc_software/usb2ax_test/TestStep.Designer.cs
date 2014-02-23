namespace USB2AX_Test {
    partial class TestStep {
        /// <summary> 
        /// Variable nécessaire au concepteur.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Nettoyage des ressources utilisées.
        /// </summary>
        /// <param name="disposing">true si les ressources managées doivent être supprimées ; sinon, false.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Code généré par le Concepteur de composants

        /// <summary> 
        /// Méthode requise pour la prise en charge du concepteur - ne modifiez pas 
        /// le contenu de cette méthode avec l'éditeur de code.
        /// </summary>
        private void InitializeComponent() {
            this.lError = new System.Windows.Forms.Label();
            this.lOK = new System.Windows.Forms.Label();
            this.lDescription = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // lError
            // 
            this.lError.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lError.AutoSize = true;
            this.lError.BackColor = System.Drawing.Color.Red;
            this.lError.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lError.Location = new System.Drawing.Point(388, 0);
            this.lError.Name = "lError";
            this.lError.Size = new System.Drawing.Size(52, 24);
            this.lError.TabIndex = 23;
            this.lError.Text = "Error";
            // 
            // lOK
            // 
            this.lOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.lOK.AutoSize = true;
            this.lOK.BackColor = System.Drawing.Color.LimeGreen;
            this.lOK.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lOK.Location = new System.Drawing.Point(345, 0);
            this.lOK.Name = "lOK";
            this.lOK.Size = new System.Drawing.Size(37, 24);
            this.lOK.TabIndex = 22;
            this.lOK.Text = "OK";
            // 
            // lDescription
            // 
            this.lDescription.AutoSize = true;
            this.lDescription.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lDescription.Location = new System.Drawing.Point(3, 0);
            this.lDescription.Name = "lDescription";
            this.lDescription.Size = new System.Drawing.Size(145, 24);
            this.lDescription.TabIndex = 21;
            this.lDescription.Text = "Step description";
            // 
            // StepView
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.lError);
            this.Controls.Add(this.lOK);
            this.Controls.Add(this.lDescription);
            this.Name = "StepView";
            this.Size = new System.Drawing.Size(442, 24);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lError;
        private System.Windows.Forms.Label lOK;
        private System.Windows.Forms.Label lDescription;
    }
}
