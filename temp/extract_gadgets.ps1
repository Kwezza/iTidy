$content = Get-Content "c:\Amiga\Programming\iTidy\Bin\Amiga\Rebuild\iTidy35.reb"
$gadgetTypes = @(5, 7, 8, 11, 14, 18, 35, 10, 30)
$results = [System.Collections.ArrayList]::new()
$lineNum = 0
$inBlock = $false
$ident = ''
$name = ''
$identLine = 0
$hints = [System.Collections.ArrayList]::new()
$blockType = -1

foreach ($line in $content) {
    $lineNum++
    $trimmed = $line.Trim()
    
    if ($trimmed -match '^TYPE:\s*(\d+)$') {
        if ($inBlock -and ($gadgetTypes -contains $blockType)) {
            $h = if ($hints.Count -gt 0) { $hints -join ' | ' } else { '(none)' }
            [void]$results.Add("$ident`t$name`t$identLine`t$blockType`t$h")
        }
        $blockType = [int]$Matches[1]
        $inBlock = $true
        $ident = ''
        $name = ''
        $identLine = 0
        $hints = [System.Collections.ArrayList]::new()
    }
    elseif ($inBlock) {
        if ($trimmed -match '^IDENT:\s*(.*)$') {
            $ident = $Matches[1]
            $identLine = $lineNum
        }
        elseif ($trimmed -match '^NAME:\s*(.*)$') {
            $name = $Matches[1]
        }
        elseif ($trimmed -match '^HINT:\s+(.+)$') {
            [void]$hints.Add($Matches[1])
        }
    }
}
if ($inBlock -and ($gadgetTypes -contains $blockType)) {
    $h = if ($hints.Count -gt 0) { $hints -join ' | ' } else { '(none)' }
    [void]$results.Add("$ident`t$name`t$identLine`t$blockType`t$h")
}

$typeNames = @{ 5='Button'; 7='Checkbox'; 8='Chooser'; 11='GetFile'; 14='Integer'; 18='ListBrowser'; 35='ListView'; 10='FuelGauge'; 30='Label' }

$output = [System.Collections.ArrayList]::new()
[void]$output.Add("IDENT`tNAME`tLine#`tType`tCurrent Hint(s)")
foreach ($r in $results) {
    $parts = $r -split "`t"
    $tn = if ($typeNames.ContainsKey([int]$parts[3])) { $typeNames[[int]$parts[3]] } else { "T$($parts[3])" }
    [void]$output.Add("$($parts[0])`t$($parts[1])`t$($parts[2])`t$tn`t$($parts[4])")
}

$output | Out-File -FilePath "c:\Amiga\Programming\iTidy\temp\gadget_extract.txt" -Encoding utf8
Write-Host "Done. Found $($results.Count) gadgets. Output in temp\gadget_extract.txt"
