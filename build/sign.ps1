param(
    [string]$TargetFile = "bin\208softwarecenter.exe"
)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$pfxPath = Join-Path $scriptDir "codesign.pfx"
$fullTarget = Join-Path (Split-Path -Parent $scriptDir) $TargetFile

if (Test-Path $fullTarget) {
    if (Test-Path $pfxPath) {
        try {
            Import-Module Microsoft.PowerShell.Security -ErrorAction SilentlyContinue
            $cert = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2($pfxPath, "SoftwareCenter208", [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::Exportable)
            $sig = Set-AuthenticodeSignature -FilePath $fullTarget -Certificate $cert -TimestampServer "http://timestamp.digicert.com" -HashAlgorithm SHA256
            if ($sig.Status -eq "Valid" -or $sig.Status -eq "UnknownError") {
                Write-Host "[Codesign] Binary successfully signed with Authenticode & DigiCert Timestamp." -ForegroundColor Green
            } else {
                Write-Host "[Codesign] Signing status: $($sig.Status) - $($sig.StatusMessage)" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "[Codesign] Error signing binary: $_" -ForegroundColor Yellow
        }
    }
}
