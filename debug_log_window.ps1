Add-Type -AssemblyName System.Windows.Forms
$form = New-Object System.Windows.Forms.Form
$form.Text = "Game Debug Log"
$form.Size = New-Object System.Drawing.Size(800, 600)
$form.StartPosition = "Manual"
$form.Location = New-Object System.Drawing.Point(100, 100)

$textBox = New-Object System.Windows.Forms.TextBox
$textBox.Multiline = $true
$textBox.ScrollBars = "Vertical"
$textBox.Dock = "Fill"
$textBox.Font = New-Object System.Drawing.Font("Consolas", 10)
$textBox.ReadOnly = $true
$form.Controls.Add($textBox)

$timer = New-Object System.Windows.Forms.Timer
$timer.Interval = 1000
$logFile = "C:\Users\nakam\Desktop\test\Re-buttlerX\game_debug.log"
$lastPosition = 0

$timer.Add_Tick({
    if (Test-Path $logFile) {
        $content = Get-Content $logFile -Raw -ErrorAction SilentlyContinue
        if ($content -and $content.Length -gt $lastPosition) {
            $newText = $content.Substring($lastPosition)
            $script:textBox.AppendText($newText)
            $script:lastPosition = $content.Length
        }
    }
})
$timer.Start()

$form.Add_Shown({$form.Activate()})
[void]$form.ShowDialog()
