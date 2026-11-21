$ErrorActionPreference = 'Stop'
$project = (Split-Path -Parent $PSScriptRoot)
$sandbox = Join-Path ([System.IO.Path]::GetTempPath()) ('scholardesk-smoke-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $sandbox | Out-Null
$data = Join-Path $sandbox 'data/students.sdb'
$exe = Join-Path $project 'bin/ScholarDeskConsole.exe'
function Run-App([string[]] $Lines) {
    $result = ($Lines | & $exe --no-color --data $data) -join "`n"
    if ($LASTEXITCODE -ne 0) { throw 'Application failed' }
    return $result
}
function Expect([bool] $Condition, [string] $Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
    Write-Host "PASS: $Message"
}
$output = Run-App @('2','CLI-1','Test Student','Computer Science','wrong','13','3','bad-email','student@example.com','11','8.75','101','92','y','','0')
Expect ($output.Contains('SUCCESS  Student added')) 'Add workflow and invalid-input recovery'
$output = Run-App @('3','CLI-1','','0')
Expect ($output.Contains('Test Student') -and $output.Contains('92.0%')) 'Search persists across launches'
$output = Run-App @('4','CLI-1','Updated Student','','','','','70','y','','6','','0')
Expect ($output.Contains('SUCCESS  Changes saved.') -and $output.Contains('Needs support')) 'Update, retained fields and academic status'
$output = Run-App @('7','smoke-export','','0')
Expect ($output.Contains('SUCCESS  Export created.') -and (Test-Path (Join-Path $sandbox 'exports/smoke-export.csv'))) 'CSV menu workflow'
$output = Run-App @('5','CLI-1','','','3','CLI-1','','0')
Expect ($output.Contains('Deletion cancelled.') -and $output.Contains('Updated Student')) 'Delete cancellation retains student'
$output = Run-App @('5','CLI-1','CLI-1','','0')
Expect ($output.Contains('SUCCESS  Student deleted.')) 'Confirmed delete workflow'
$output = Run-App @('2','/cancel','1','','0')
Expect ($output.Contains('Action cancelled.') -and $output.Contains('No students to display.')) 'Form cancellation and empty state'
$output = Run-App @('2','INCOMPLETE')
Expect ($output.Contains('Input closed.')) 'EOF during form exits cleanly'
Expect ((Get-Content $data).Count -eq 1) 'Cancelled and incomplete forms never save'
$paginate = Join-Path $sandbox 'data/pages.sdb'
$records = @('SCHOLARDESK 1')
1..12 | ForEach-Object { $records += ('"ID-' + $_ + '" "Student ' + $_ + '" "Course" 1 "student@example.com" 8 90') }
Set-Content -Path $paginate -Value $records -Encoding ascii
$output = (@('1','n','p','','','0') | & $exe --no-color --data $paginate) -join "`n"
Expect ($output.Contains('Page 2 of 2') -and $output.Contains('Page 1 of 2')) 'Directory forward and backward pagination'
$output | Set-Content (Join-Path $sandbox 'pagination-output.txt')
$output = Run-App @('9','6','','0')
Expect ($output.Contains('I like work: it fascinates me.') -and $output.Contains('Original: https://standardebooks.org/ebooks/jerome-k-jerome/three-men-in-a-boat/text/chapter-15')) 'Reading shelf shows exact passage and original source'
$output = Run-App @('9','99','','0')
Expect ($output.Contains('Choose a whole number from 1 to 12.')) 'Reading shelf rejects invalid selection safely'
Write-Host 'All 12 CLI smoke checks passed.'
