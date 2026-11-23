# Builds assets\icon.ico from the source images in assets\icon-source\.
#
#   16 and 32 px   the hand-made images, used as they are
#   64, 128, 256   the large artwork (06_violet_chrome.png, a 64x76 pixel-art grid drawn at 4x), cropped to a square
#                  and scaled with nearest neighbour so the pixels stay sharp (it is an exact 1x / 2x / 4x of the grid)
#   48 px          resampled (48 is 0.75 of the 64 px grid, so it cannot be pixel-exact)
#
# Why: Explorer picks the image closest to the size it needs. An icon holding only 16 and 32 px looks tiny when icons are
# shown large, because there is nothing bigger to pick. Run:  powershell -File assets\make-icon.ps1
Add-Type -AssemblyName System.Drawing
$src = Join-Path $PSScriptRoot 'icon-source'
$out = Join-Path $PSScriptRoot 'icon.ico'

function ToPng([System.Drawing.Bitmap]$bmp) {
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    return , $ms.ToArray()
}

function Scale([System.Drawing.Bitmap]$from, [int]$size, [bool]$pixelExact) {
    $to = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($to)
    $g.Clear([System.Drawing.Color]::Transparent)
    if ($pixelExact) {
        $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
        $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
    } else {
        $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    }
    $g.DrawImage($from, (New-Object System.Drawing.Rectangle 0, 0, $size, $size), 0, 0, $from.Width, $from.Height,
                 [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose()
    return $to
}

# Large artwork: 256x304 canvas, content in rows 48..291. Crop a 256x256 window (rows 40..295) that holds all of it.
$art = New-Object System.Drawing.Bitmap (Join-Path $src '06_violet_chrome.png')
$square = $art.Clone((New-Object System.Drawing.Rectangle 0, 40, 256, 256), [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$art.Dispose()

# A plain list of (size, PNG bytes). Not a dictionary: PowerShell reads integer keys as positions.
$images = New-Object System.Collections.Generic.List[object]
function Add-Image([int]$size, [byte[]]$data) { $images.Add([pscustomobject]@{ Size = $size; Data = $data }) }

Add-Image 16 ([System.IO.File]::ReadAllBytes((Join-Path $src 'icon_16x16.png')))
Add-Image 32 ([System.IO.File]::ReadAllBytes((Join-Path $src 'icon_32x32.png')))
$b = Scale $square 48 $false;  Add-Image 48 (ToPng $b);  $b.Dispose()
$b = Scale $square 64 $true;   Add-Image 64 (ToPng $b);  $b.Dispose()
$b = Scale $square 128 $true;  Add-Image 128 (ToPng $b); $b.Dispose()
Add-Image 256 (ToPng $square)
$square.Dispose()

# ICO file: header, one directory entry per image, then the PNG data (PNG-compressed entries, Windows Vista and later).
$ms = New-Object System.IO.MemoryStream
$w = New-Object System.IO.BinaryWriter $ms
$w.Write([uint16]0); $w.Write([uint16]1); $w.Write([uint16]$images.Count)
$offset = 6 + 16 * $images.Count
foreach ($img in $images) {
    $dim = if ($img.Size -ge 256) { 0 } else { $img.Size }   # 0 means 256 in an ICO directory
    $w.Write([byte]$dim); $w.Write([byte]$dim); $w.Write([byte]0); $w.Write([byte]0)
    $w.Write([uint16]1); $w.Write([uint16]32)
    $w.Write([uint32]$img.Data.Length); $w.Write([uint32]$offset)
    $offset += $img.Data.Length
}
foreach ($img in $images) { $w.Write($img.Data) }
$w.Flush()
[System.IO.File]::WriteAllBytes($out, $ms.ToArray())
"Wrote $out : " + (($images | ForEach-Object { "$($_.Size)px" }) -join ', ') + " ($($ms.Length) bytes)"
