# PowerShell script to permanently add MSBuild to PATH
Write-Host "Adding MSBuild to system PATH..." -ForegroundColor Green

# Find the most recent MSBuild installation
$msbuildPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin"
)

$msbuildPath = $null
foreach ($path in $msbuildPaths) {
    if (Test-Path "$path\MSBuild.exe") {
        $msbuildPath = $path
        Write-Host "Found MSBuild at: $path" -ForegroundColor Yellow
        break
    }
}

if (-not $msbuildPath) {
    Write-Host "MSBuild not found! Please install Visual Studio or Build Tools." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}

# Get current system PATH
$currentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")

# Check if MSBuild is already in PATH
if ($currentPath -like "*$msbuildPath*") {
    Write-Host "MSBuild is already in system PATH!" -ForegroundColor Green
} else {
    try {
        # Add MSBuild to system PATH
        $newPath = "$currentPath;$msbuildPath"
        [Environment]::SetEnvironmentVariable("Path", $newPath, "Machine")
        Write-Host "Successfully added MSBuild to system PATH!" -ForegroundColor Green
        Write-Host "You may need to restart your command prompt for changes to take effect." -ForegroundColor Yellow
    } catch {
        Write-Host "Failed to add to system PATH. You may need to run as Administrator." -ForegroundColor Red
        Write-Host "Error: $_" -ForegroundColor Red
        
        # Fallback: add to user PATH
        try {
            $userPath = [Environment]::GetEnvironmentVariable("Path", "User")
            if ($userPath -notlike "*$msbuildPath*") {
                $newUserPath = "$userPath;$msbuildPath"
                [Environment]::SetEnvironmentVariable("Path", $newUserPath, "User")
                Write-Host "Added MSBuild to user PATH instead." -ForegroundColor Yellow
            }
        } catch {
            Write-Host "Failed to add to user PATH as well." -ForegroundColor Red
        }
    }
}

# Test MSBuild
Write-Host "`nTesting MSBuild..." -ForegroundColor Cyan
try {
    & "$msbuildPath\MSBuild.exe" /version
    Write-Host "MSBuild is working correctly!" -ForegroundColor Green
} catch {
    Write-Host "MSBuild test failed: $_" -ForegroundColor Red
}

Read-Host "`nPress Enter to exit"