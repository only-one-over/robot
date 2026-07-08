param(
    [string]$InputPath = (Join-Path $PSScriptRoot "robot_control_technology_research_report_formula_corrected.docx"),
    [string]$OutputDocx = (Join-Path $PSScriptRoot "robot_control_technology_research_report_final.docx"),
    [string]$OutputPdf = (Join-Path $PSScriptRoot "robot_control_technology_research_report_final.pdf")
)

$ErrorActionPreference = "Stop"

$wdFormatDocumentDefault = 16
$wdFormatPDF = 17
$wdFindStop = 0
$wdCollapseEnd = 0

$equations = @(
    "T_i(q_i)=Trans(p_i) · Rot(a_i,q_i),  i=1,…,6",
    "(_^(base_link))T_(tool0)(q)=T_1(q_1) · T_2(q_2) · T_3(q_3) · T_4(q_4) · T_5(q_5) · T_6(q_6) · (_^6)T_(tool0)",
    "Q̂=q_r+εq_d,  q_d=1/2 p^*⊗q_r,  ε^2=0",
    "e(q)=[e_p^T  e_R^T]^T,  e_p=p^*−p(q)",
    "e_R=1/2(r_(c1)×r_(t1)+r_(c2)×r_(t2)+r_(c3)×r_(t3))",
    "J_e(:,i)≈(e(q+δ_i e_i)−e(q))/δ_i,  0<|δ_i|≤h,  h=10^(−5)",
    "g=J_e^T e,  α=(g^T g)/((J_e g)^T(J_e g)),  Δq_(JT)=−αg",
    "J_e=UΣV^T,  Δq_(PINV)=−J_e^+ e=−VΣ^+ U^T e",
    "Δq_(DLS)=−J_e^T(J_e J_e^T+λ^2 I)^(−1) e,  λ=0.08",
    "Δq_(JT)=−αJ_e^T e",
    "Δq_(PINV)=−J_e^+ e=−VΣ^+ U^T e",
    "Δq_(DLS)=−J_e^T(J_e J_e^T+λ^2 I)^(−1) e",
    "q(s)=q_0+(10s^3−15s^4+6s^5)(q_f−q_0),  s∈[0,1]"
)

$figurePaths = @(
    (Join-Path $PSScriptRoot "figures\fig1_system_architecture.png"),
    (Join-Path $PSScriptRoot "figures\fig2_kinematics_flow.png"),
    (Join-Path $PSScriptRoot "figures\fig3_validation_loop.png"),
    (Join-Path $PSScriptRoot "figures\fig5_ik_algorithm_comparison.png"),
    (Join-Path $PSScriptRoot "figures\fig4_interactive_runtime.png"),
    (Join-Path $PSScriptRoot "figures\fig6_rviz_gazebo_runtime_screenshot.png")
)

$subscriptMap = [ordered]@{
    "₀" = "0"; "₁" = "1"; "₂" = "2"; "₃" = "3"; "₄" = "4"
    "₅" = "5"; "₆" = "6"; "₇" = "7"; "₈" = "8"; "₉" = "9"
    "ᵢ" = "i"; "ᵣ" = "r"
}

$superscriptMap = [ordered]@{
    "²" = "2"; "³" = "3"; "⁴" = "4"; "⁵" = "5"; "⁺" = "+"; "⁻" = "-"
    "ᵀ" = "T"
}

function Set-FindDefaults {
    param($Find)
    $Find.ClearFormatting()
    $Find.Replacement.ClearFormatting()
    $Find.Forward = $true
    $Find.Wrap = $wdFindStop
    $Find.Format = $false
    $Find.MatchCase = $true
    $Find.MatchWholeWord = $false
    $Find.MatchWildcards = $false
}

function Replace-AllText {
    param($Document, [string]$Needle, [string]$Replacement)

    $search = $Document.Content.Duplicate
    $find = $search.Find
    Set-FindDefaults $find
    $find.Text = $Needle
    $find.Replacement.Text = $Replacement
    [void]$find.Execute(
        $Needle, $true, $false, $false, $false, $false,
        $true, $wdFindStop, $false, $Replacement, 2
    )
}

function Convert-UnicodeScript {
    param(
        $Document,
        [string]$Needle,
        [string]$Replacement,
        [ValidateSet("Subscript", "Superscript")]
        [string]$Kind
    )

    $search = $Document.Content.Duplicate
    $find = $search.Find
    Set-FindDefaults $find
    $find.Text = $Needle

    while ($find.Execute()) {
        $match = $search.Duplicate
        if ($match.OMaths.Count -eq 0) {
            $match.Text = $Replacement
            if ($Kind -eq "Subscript") {
                $match.Font.Subscript = $true
                $match.Font.Superscript = $false
            } else {
                $match.Font.Superscript = $true
                $match.Font.Subscript = $false
            }
        }
        $nextStart = [Math]::Max($match.End, $search.Start + 1)
        $search.SetRange($nextStart, $Document.Content.End)
        $find = $search.Find
        Set-FindDefaults $find
        $find.Text = $Needle
    }
}

function Convert-AllCaretPowers {
    param($Document)

    foreach ($paragraph in $Document.Paragraphs) {
        $text = $paragraph.Range.Text
        $matches = [regex]::Matches($text, "10\^([−-]?\d+)")
        for ($index = $matches.Count - 1; $index -ge 0; $index--) {
            $regexMatch = $matches[$index]
            $exponent = $regexMatch.Groups[1].Value
            $start = $paragraph.Range.Start + $regexMatch.Index
            $matchRange = $Document.Range($start, $start + $regexMatch.Length)
            if ($matchRange.OMaths.Count -eq 0) {
                $matchRange.Text = "10" + $exponent
                $exponentRange = $Document.Range($start + 2, $start + 2 + $exponent.Length)
                $exponentRange.Font.Superscript = $true
                $exponentRange.Font.Subscript = $false
            }
        }
    }
}

function Convert-InlineSubscriptToken {
    param($Document, [string]$Token, [int]$SubscriptStart)

    $search = $Document.Content.Duplicate
    $find = $search.Find
    Set-FindDefaults $find
    $find.Text = $Token

    while ($find.Execute()) {
        $match = $search.Duplicate
        if ($match.OMaths.Count -eq 0) {
            $start = $match.Start
            $plain = $Token.Replace("_", "")
            $match.Text = $plain
            $subRange = $Document.Range($start + $SubscriptStart, $start + $plain.Length)
            $subRange.Font.Subscript = $true
            $subRange.Font.Superscript = $false
        }
        $nextStart = [Math]::Max($match.End, $search.Start + 1)
        $search.SetRange($nextStart, $Document.Content.End)
        $find = $search.Find
        Set-FindDefaults $find
        $find.Text = $Token
    }
}

function Set-BodyCitationsSuperscript {
    param($Document)

    foreach ($paragraph in $Document.Paragraphs) {
        $text = $paragraph.Range.Text
        if ($text.Trim() -eq "参考文献") {
            break
        }

        $matches = [regex]::Matches($text, "\[\d+(?:[-,]\d+)*\]")
        foreach ($regexMatch in $matches) {
            $citationRange = $Document.Range(
                $paragraph.Range.Start + $regexMatch.Index,
                $paragraph.Range.Start + $regexMatch.Index + $regexMatch.Length
            )
            $citationRange.Font.Superscript = $true
            $citationRange.Font.Subscript = $false
        }
    }
}

$word = $null
$document = $null

try {
    Copy-Item -LiteralPath $InputPath -Destination $OutputDocx -Force

    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $word.DisplayAlerts = 0
    $document = $word.Documents.Open($OutputDocx)

    $textReplacements = [ordered]@{
        "81.6%、93.5%和95.0%" = "87.3%、98.7%和100.0%"
        "22.75、5.14和3.54 ms" = "22.29、1.68和0.62 ms"
        "81.6%, 93.5%, and 95.0%" = "87.3%, 98.7%, and 100.0%"
        "22.75, 5.14, and 3.54 ms" = "22.29, 1.68, and 0.62 ms"
        "816/1000" = "873/1000"
        "935/1000" = "987/1000"
        "950/1000" = "1000/1000"
        "4.887/5.000" = "4.883/5.000"
        "0.00195/0.05148" = "0.00206/0.05148"
        "85.29" = "85.66"
        "1.288/4.985" = "1.266/4.992"
        "0.00361/0.05004" = "0.00359/0.05004"
        "2.22" = "2.23"
        "2.555/4.991" = "2.553/4.991"
        "0.00248/0.04916" = "0.00249/0.04916"
        "2.20" = "2.17"
        "22.75" = "22.29"
        "5.14" = "1.68"
        "3.54" = "0.62"
        "1.5个百分点" = "1.3个百分点"
        "13.4个百分点" = "12.7个百分点"
        "1.288 mm" = "1.266 mm"
        "6.138和6.205 m" = "6.210和6.214 m"
        "DLS的95.0%成功率不能被解释为全局收敛保证。目标由FK生成，因此均为理论可达位姿；仍有5.0%样本失败，说明" =
            "DLS在本批1000组样本中全部收敛，但不能被解释为全局收敛保证。目标由FK生成，因此均为理论可达位姿；"
    }
    foreach ($entry in $textReplacements.GetEnumerator()) {
        Replace-AllText $document $entry.Key $entry.Value
    }

    if ($document.InlineShapes.Count -ne $figurePaths.Count) {
        throw "Expected $($figurePaths.Count) inline figures, found $($document.InlineShapes.Count)."
    }
    for ($index = $figurePaths.Count; $index -ge 1; $index--) {
        $shape = $document.InlineShapes.Item($index)
        $width = $shape.Width
        $range = $shape.Range.Duplicate
        $shape.Delete()
        $replacement = $document.InlineShapes.AddPicture(
            $figurePaths[$index - 1],
            $false,
            $true,
            $range
        )
        $replacement.LockAspectRatio = $true
        $replacement.Width = $width
    }

    foreach ($paragraph in $document.Paragraphs) {
        if ($paragraph.Range.Text -like "式(2)中，Tbase→tool0表示末端tool0*") {
            $range = $paragraph.Range.Duplicate
            $range.End = $range.End - 1
            $range.Text = (
                "式（2）中，左端表示tool0相对于base_link的齐次变换，q为六维关节向量；" +
                "右端前六项依次表示六个关节的局部变换，末项表示link_6至tool0的固定变换，" +
                "中间圆点均表示矩阵乘法。"
            )
        }
        if ($paragraph.Range.Text -like "*实测目标位置为x=-0.55 m、y=-0.25 m、z=0.05 m*") {
            $range = $paragraph.Range.Duplicate
            $range.End = $range.End - 1
            $range.Text = (
                "Gazebo闭环实验启动两个控制器并依次执行3个预设目标，运行日志记录到action goal accepted" +
                "和Goal reached, success，/joint_states、/fk_pose与/ik_status均持续更新。RViz交互实验中，" +
                "/target_marker/get_interactive_markers服务可用，MOUSE_UP反馈触发/target_pose发布；" +
                "目标取前中部常用工作位姿q=[-0.35,-1.00,1.55,-0.50,-0.35,-0.20] rad，" +
                "由FK得到x=-0.545 m、y=-0.024 m、z=0.145 m。该位姿远离关节限位且不处于腕部奇异状态，" +
                "仿真机械臂通过同一DLS与轨迹链响应。"
            )
        }
        if ($paragraph.Range.Text -like "*为误差Jacobian的第i列*") {
            $range = $paragraph.Range.Duplicate
            $range.End = $range.End - 1
            $range.Text = (
                "J_e(:,i)为误差Jacobian的第i列，eᵢ为第i个标准基向量。δᵢ的符号选择应保证扰动后" +
                "关节仍处于限位内；接近限位时分母采用不超过h的实际扰动量。"
            )
        }
    }

    if ($document.OMaths.Count -ne $equations.Count) {
        throw "Expected $($equations.Count) equations, found $($document.OMaths.Count)."
    }

    for ($index = 1; $index -le $equations.Count; $index++) {
        $equation = $document.OMaths.Item($index)
        $equation.Range.Text = $equations[$index - 1]
        $equation.BuildUp()
        $equation.Range.Font.Name = "Cambria Math"
        $equation.Range.Font.NameAscii = "Cambria Math"
        $equation.Range.Font.NameOther = "Cambria Math"
    }

    foreach ($entry in $subscriptMap.GetEnumerator()) {
        Convert-UnicodeScript $document $entry.Key $entry.Value "Subscript"
    }
    foreach ($entry in $superscriptMap.GetEnumerator()) {
        Convert-UnicodeScript $document $entry.Key $entry.Value "Superscript"
    }

    Convert-AllCaretPowers $document

    Convert-InlineSubscriptToken $document "J_e" 1
    Convert-InlineSubscriptToken $document "J_x" 1
    Convert-InlineSubscriptToken $document "e_p" 1
    Convert-InlineSubscriptToken $document "e_R" 1
    Convert-InlineSubscriptToken $document "q_d" 1
    Convert-InlineSubscriptToken $document "q_r" 1
    Convert-InlineSubscriptToken $document "q_f" 1
    Convert-InlineSubscriptToken $document "T_tool" 1
    Convert-InlineSubscriptToken $document "T_base→tool0" 1
    Convert-InlineSubscriptToken $document "r_cj" 1
    Convert-InlineSubscriptToken $document "r_tj" 1
    Convert-InlineSubscriptToken $document "Δq_JT" 2
    Convert-InlineSubscriptToken $document "Δq_PINV" 2
    Convert-InlineSubscriptToken $document "Δq_DLS" 2

    Set-BodyCitationsSuperscript $document

    $document.SaveAs2($OutputDocx, $wdFormatDocumentDefault)
    $document.SaveAs2($OutputPdf, $wdFormatPDF)
    $document.Close($false)
    $document = $null
    $word.Quit()
    $word = $null

    Write-Output "DOCX=$OutputDocx"
    Write-Output "PDF=$OutputPdf"
}
finally {
    if ($null -ne $document) {
        $document.Close($false)
    }
    if ($null -ne $word) {
        $word.Quit()
    }
}
