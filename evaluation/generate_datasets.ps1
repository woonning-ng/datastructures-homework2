param(
    [string]$OutputDir = "evaluation/datasets",
    [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function New-Point {
    param([double]$X, [double]$Y)
    [pscustomobject]@{ x = $X; y = $Y }
}

function Get-SignedArea {
    param([object[]]$Ring)
    $area = 0.0
    for ($i = 0; $i -lt $Ring.Count; $i++) {
        $j = ($i + 1) % $Ring.Count
        $area += $Ring[$i].x * $Ring[$j].y - $Ring[$i].y * $Ring[$j].x
    }
    $area / 2.0
}

function Ensure-Orientation {
    param(
        [object[]]$Ring,
        [bool]$Clockwise
    )
    $isClockwise = (Get-SignedArea $Ring) -lt 0.0
    if ($isClockwise -eq $Clockwise) {
        return ,$Ring
    }
    $copy = @($Ring)
    [array]::Reverse($copy)
    return ,$copy
}

function New-RegularPolygon {
    param(
        [double]$CenterX,
        [double]$CenterY,
        [double]$Radius,
        [int]$VertexCount,
        [double]$Phase = 0.0
    )
    $points = @()
    for ($i = 0; $i -lt $VertexCount; $i++) {
        $theta = 2.0 * [Math]::PI * $i / $VertexCount + $Phase
        $points += New-Point ($CenterX + $Radius * [Math]::Cos($theta)) ($CenterY + $Radius * [Math]::Sin($theta))
    }
    return ,(Ensure-Orientation $points $false)
}

function New-WavyPolygon {
    param(
        [double]$CenterX,
        [double]$CenterY,
        [double]$BaseRadius,
        [int]$VertexCount,
        [double]$AmplitudeOne,
        [int]$FrequencyOne,
        [double]$AmplitudeTwo,
        [int]$FrequencyTwo,
        [double]$ScaleX = 1.0,
        [double]$ScaleY = 1.0
    )
    $points = @()
    for ($i = 0; $i -lt $VertexCount; $i++) {
        $t = 2.0 * [Math]::PI * $i / $VertexCount
        $radius = $BaseRadius * (1.0 + $AmplitudeOne * [Math]::Sin($FrequencyOne * $t) + $AmplitudeTwo * [Math]::Cos($FrequencyTwo * $t))
        $x = $CenterX + $ScaleX * $radius * [Math]::Cos($t)
        $y = $CenterY + $ScaleY * $radius * [Math]::Sin($t)
        $points += New-Point $x $y
    }
    return ,(Ensure-Orientation $points $false)
}

function New-NearCollinearStrip {
    param(
        [int]$TotalVertices,
        [double]$Length,
        [double]$Width,
        [double]$JitterAmplitude
    )
    $sideCount = [Math]::Max(4, [int]([Math]::Floor($TotalVertices / 2)))
    $top = @()
    $bottom = @()
    for ($i = 0; $i -lt $sideCount; $i++) {
        $u = $i / [double]($sideCount - 1)
        $x = $Length * $u
        $yt = $Width / 2.0 + $JitterAmplitude * ([Math]::Sin(6.0 * [Math]::PI * $u) + 0.25 * [Math]::Cos(17.0 * [Math]::PI * $u))
        $yb = -$Width / 2.0 + $JitterAmplitude * (0.8 * [Math]::Cos(5.0 * [Math]::PI * $u) - 0.2 * [Math]::Sin(13.0 * [Math]::PI * $u))
        $top += New-Point $x $yt
        $bottom += New-Point $x $yb
    }
    [array]::Reverse($bottom)
    return ,(Ensure-Orientation @($top + $bottom) $false)
}

function New-RectangleHole {
    param(
        [double]$MinX,
        [double]$MinY,
        [double]$MaxX,
        [double]$MaxY
    )
    $ring = @(
        (New-Point $MinX $MinY),
        (New-Point $MinX $MaxY),
        (New-Point $MaxX $MaxY),
        (New-Point $MaxX $MinY)
    )
    return ,(Ensure-Orientation $ring $true)
}

function New-IrregularHole {
    param(
        [double]$CenterX,
        [double]$CenterY,
        [double]$BaseRadius,
        [int]$VertexCount,
        [double]$Amplitude,
        [int]$Frequency,
        [double]$ScaleX = 1.0,
        [double]$ScaleY = 1.0
    )
    $ring = New-WavyPolygon -CenterX $CenterX -CenterY $CenterY -BaseRadius $BaseRadius -VertexCount $VertexCount -AmplitudeOne $Amplitude -FrequencyOne $Frequency -AmplitudeTwo ($Amplitude / 2.0) -FrequencyTwo ($Frequency + 2) -ScaleX $ScaleX -ScaleY $ScaleY
    return ,(Ensure-Orientation $ring $true)
}

function Write-PolygonCsv {
    param(
        [string]$Path,
        [object[]]$Rings
    )
    $lines = @("ring_id,vertex_id,x,y")
    for ($ringId = 0; $ringId -lt $Rings.Count; $ringId++) {
        $ring = $Rings[$ringId]
        for ($vertexId = 0; $vertexId -lt $ring.Count; $vertexId++) {
            $x = [string]::Format([Globalization.CultureInfo]::InvariantCulture, "{0:G17}", $ring[$vertexId].x)
            $y = [string]::Format([Globalization.CultureInfo]::InvariantCulture, "{0:G17}", $ring[$vertexId].y)
            $lines += "{0},{1},{2},{3}" -f $ringId, $vertexId, $x, $y
        }
    }
    Set-Content -LiteralPath $Path -Value $lines
}

function Add-DatasetRecord {
    param(
        [System.Collections.Generic.List[object]]$Manifest,
        [string]$Name,
        [string]$Family,
        [object[]]$Rings,
        [double]$DefaultTargetRatio,
        [bool]$SweepFocus,
        [string]$StressProperty,
        [string]$Challenge
    )
    $inputVertices = ($Rings | ForEach-Object { $_.Count } | Measure-Object -Sum).Sum
    $ringCount = $Rings.Count
    $holeCount = [Math]::Max(0, $ringCount - 1)
    $Manifest.Add([pscustomobject]@{
        dataset_name = $Name
        family = $Family
        input_vertices = $inputVertices
        ring_count = $ringCount
        hole_count = $holeCount
        default_target_ratio = $DefaultTargetRatio
        sweep_focus = if ($SweepFocus) { "yes" } else { "no" }
        stress_property = $StressProperty
        why_challenging = $Challenge
    })
}

if ($Clean -and (Test-Path $OutputDir)) {
    Get-ChildItem -LiteralPath $OutputDir -Filter "*.csv" -File | Remove-Item -Force
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$manifest = [System.Collections.Generic.List[object]]::new()

foreach ($n in @(64, 128, 256, 512)) {
    $name = "convex_large_{0:d3}" -f $n
    $rings = @(New-RegularPolygon -CenterX 0.0 -CenterY 0.0 -Radius 100.0 -VertexCount $n -Phase 0.05)
    Write-PolygonCsv -Path (Join-Path $OutputDir "$name.csv") -Rings $rings
    Add-DatasetRecord -Manifest $manifest -Name $name -Family "convex_large" -Rings $rings -DefaultTargetRatio 0.50 -SweepFocus ($n -eq 256) -StressProperty "High vertex count on a simple convex exterior ring." -Challenge "Separates algorithmic overhead from topology complexity by forcing many candidate updates on a geometry with no holes."
}

foreach ($n in @(64, 128, 256, 512)) {
    $name = "wavy_boundary_{0:d3}" -f $n
    $rings = @(New-WavyPolygon -CenterX 0.0 -CenterY 0.0 -BaseRadius 120.0 -VertexCount $n -AmplitudeOne 0.18 -FrequencyOne 5 -AmplitudeTwo 0.08 -FrequencyTwo 11 -ScaleX 1.0 -ScaleY 0.85)
    Write-PolygonCsv -Path (Join-Path $OutputDir "$name.csv") -Rings $rings
    Add-DatasetRecord -Manifest $manifest -Name $name -Family "wavy_boundary" -Rings $rings -DefaultTargetRatio 0.45 -SweepFocus ($n -eq 256) -StressProperty "Irregular noisy outer boundary." -Challenge "Creates many low-displacement local collapses with nontrivial geometry, increasing the need for careful validity checks."
}

foreach ($n in @(80, 160, 320, 640)) {
    $name = "near_collinear_{0:d3}" -f $n
    $rings = @(New-NearCollinearStrip -TotalVertices $n -Length 400.0 -Width 12.0 -JitterAmplitude 1.35)
    Write-PolygonCsv -Path (Join-Path $OutputDir "$name.csv") -Rings $rings
    Add-DatasetRecord -Manifest $manifest -Name $name -Family "near_collinear" -Rings $rings -DefaultTargetRatio 0.40 -SweepFocus ($n -eq 320) -StressProperty "Nearly collinear chains and skinny geometry." -Challenge "Stresses floating-point robustness, small triangle areas, and collapse validity near degenerate configurations."
}

foreach ($grid in @(2, 3, 4, 5)) {
    $name = "many_holes_{0}x{0}" -f $grid
    $rings = @()
    $outerSize = 240.0
    $rings += @(Ensure-Orientation @(
        (New-Point 0.0 0.0),
        (New-Point $outerSize 0.0),
        (New-Point $outerSize $outerSize),
        (New-Point 0.0 $outerSize)
    ) $false)
    $cell = $outerSize / $grid
    $holeSize = $cell * 0.34
    $margin = ($cell - $holeSize) / 2.0
    for ($row = 0; $row -lt $grid; $row++) {
        for ($col = 0; $col -lt $grid; $col++) {
            $minX = $col * $cell + $margin
            $minY = $row * $cell + $margin
            $rings += @(New-RectangleHole -MinX $minX -MinY $minY -MaxX ($minX + $holeSize) -MaxY ($minY + $holeSize))
        }
    }
    Write-PolygonCsv -Path (Join-Path $OutputDir "$name.csv") -Rings $rings
    Add-DatasetRecord -Manifest $manifest -Name $name -Family "many_holes" -Rings $rings -DefaultTargetRatio 0.65 -SweepFocus ($grid -eq 4) -StressProperty "Many interior rings and dense ring-to-ring interaction checks." -Challenge "Topology preservation dominates because every collapse must avoid both self-intersections and hole collisions."
}

foreach ($n in @(96, 192, 384)) {
    $name = "narrow_gap_{0:d3}" -f $n
    $outer = New-WavyPolygon -CenterX 0.0 -CenterY 0.0 -BaseRadius 140.0 -VertexCount $n -AmplitudeOne 0.07 -FrequencyOne 4 -AmplitudeTwo 0.03 -FrequencyTwo 9 -ScaleX 1.05 -ScaleY 0.9
    $holeA = New-IrregularHole -CenterX -65.0 -CenterY 0.0 -BaseRadius 52.0 -VertexCount 12 -Amplitude 0.08 -Frequency 3 -ScaleX 0.85 -ScaleY 1.25
    $holeB = New-IrregularHole -CenterX 78.0 -CenterY -10.0 -BaseRadius 42.0 -VertexCount 10 -Amplitude 0.06 -Frequency 4 -ScaleX 1.0 -ScaleY 0.78
    $rings = @($outer, $holeA, $holeB)
    Write-PolygonCsv -Path (Join-Path $OutputDir "$name.csv") -Rings $rings
    Add-DatasetRecord -Manifest $manifest -Name $name -Family "narrow_gap" -Rings $rings -DefaultTargetRatio 0.55 -SweepFocus $false -StressProperty "Small clearances between exterior ring and nearby holes." -Challenge "A valid local area-preserving point can become globally invalid because tiny movements may intersect a neighboring ring."
}

foreach ($scalePair in @(
    @{ Name = "scaled_irregular_small"; Scale = 1.0 },
    @{ Name = "scaled_irregular_large"; Scale = 1000.0 }
)) {
    $s = [double]$scalePair.Scale
    $outer = New-WavyPolygon -CenterX 0.0 -CenterY 0.0 -BaseRadius (110.0 * $s) -VertexCount 180 -AmplitudeOne 0.15 -FrequencyOne 6 -AmplitudeTwo 0.04 -FrequencyTwo 13 -ScaleX 1.2 -ScaleY 0.9
    $holeA = New-IrregularHole -CenterX (-40.0 * $s) -CenterY (25.0 * $s) -BaseRadius (22.0 * $s) -VertexCount 9 -Amplitude 0.07 -Frequency 3 -ScaleX 1.0 -ScaleY 0.8
    $holeB = New-IrregularHole -CenterX (50.0 * $s) -CenterY (-35.0 * $s) -BaseRadius (18.0 * $s) -VertexCount 8 -Amplitude 0.09 -Frequency 4 -ScaleX 0.75 -ScaleY 1.15
    $rings = @($outer, $holeA, $holeB)
    Write-PolygonCsv -Path (Join-Path $OutputDir "$($scalePair.Name).csv") -Rings $rings
    Add-DatasetRecord -Manifest $manifest -Name $scalePair.Name -Family "scaled_irregular" -Rings $rings -DefaultTargetRatio 0.50 -SweepFocus $false -StressProperty "Coordinate scale sensitivity with identical topology and vertex count." -Challenge "Tests whether large coordinate magnitudes affect timing, memory, or numerical stability even when combinatorial structure is unchanged."
}

$manifest | Sort-Object family, input_vertices, dataset_name | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $OutputDir "manifest.csv")

$datasetReadme = @(
    "# Custom Evaluation Datasets",
    "",
    'All datasets in this folder use the required `ring_id,vertex_id,x,y` CSV format.',
    "",
    "| Dataset family | Sizes | Stress property | Why it is meaningful |",
    "|---|---:|---|---|"
)
foreach ($group in ($manifest | Group-Object family)) {
    $sizes = ($group.Group | Sort-Object input_vertices | ForEach-Object { $_.input_vertices }) -join ", "
    $datasetReadme += "| $($group.Name) | $sizes | $($group.Group[0].stress_property) | $($group.Group[0].why_challenging) |"
}
$datasetReadme += ""
$datasetReadme += "Generated manifest: [`manifest.csv`](manifest.csv)."
Set-Content -LiteralPath (Join-Path $OutputDir "README.md") -Value $datasetReadme
Write-Host "Generated $($manifest.Count) custom datasets in $OutputDir"
