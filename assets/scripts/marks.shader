// markShadow is the very cheap blurry blob underneath the player
gfx/marks/shadow
{
  polygonOffset
  {
    map gfx/marks/shadow.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen vertex
    alphaGen vertex
  }
}

// wake is the mark on water surfaces for paddling players
gfx/marks/wake
{
  polygonOffset
  {
    clampmap gfx/marks/splash.tga
    blendFunc GL_ONE GL_ONE
    rgbGen vertex
    tcmod rotate 250
    tcMod stretch sin .9 0.1 0 0.7
    rgbGen wave sin .7 .3 .25 .5
  }
  {
    clampmap gfx/marks/splash.tga
    blendFunc GL_ONE GL_ONE
    rgbGen vertex
    tcmod rotate -230
    tcMod stretch sin .9 0.05 0 0.9
    rgbGen wave sin .7 .3 .25 .4
  }
}

gfx/marks/bullet_mrk
{
  polygonOffset
  {
    map gfx/marks/bullet_mrk.tga
    blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
    rgbGen exactVertex
  }
}

gfx/marks/burn_mrk
{
  polygonOffset
  {
    map gfx/marks/burn_mrk.tga
    blendFunc GL_ZERO GL_ONE_MINUS_SRC_COLOR
    rgbGen exactVertex
  }
}

gfx/marks/plasma_mrk
{
  polygonOffset
  {
    map gfx/marks/plasma_mrk.tga
    blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    rgbGen vertex
    alphaGen vertex
  }
}

creep
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep549.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep0
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep0.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep1
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep1.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep2
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep2.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep3
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep3.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep4
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep4.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep5
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep5.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep6
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep6.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep7
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep7.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep8
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep8.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep9
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep9.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep10
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep10.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep11
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep11.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep12
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep12.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep13
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep13.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep14
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep14.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep15
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep15.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep16
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep16.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep17
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep17.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep18
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep18.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep19
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep19.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep20
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep20.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep21
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep21.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep22
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep22.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep23
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep23.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep24
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep24.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep25
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep25.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep26
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep26.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep27
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep27.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep28
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep28.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep29
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep29.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep30
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep30.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep31
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep31.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep32
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep32.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep33
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep33.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep34
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep34.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep35
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep35.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep36
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep36.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep37
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep37.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep38
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep38.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep39
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep39.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep40
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep40.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep41
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep41.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep42
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep42.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep43
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep43.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep44
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep44.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep45
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep45.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep46
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep46.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep47
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep47.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep48
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep48.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep49
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep49.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep50
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep50.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep51
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep51.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep52
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep52.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep53
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep53.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep54
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep54.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep55
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep55.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep56
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep56.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep57
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep57.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep58
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep58.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep59
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep59.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep60
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep60.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep61
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep61.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep62
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep62.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep63
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep63.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep64
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep64.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep65
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep65.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep66
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep66.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep67
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep67.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep68
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep68.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep69
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep69.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep70
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep70.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep71
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep71.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep72
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep72.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep73
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep73.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep74
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep74.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep75
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep75.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep76
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep76.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep77
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep77.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep78
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep78.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep79
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep79.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep80
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep80.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep81
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep81.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep82
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep82.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep83
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep83.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep84
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep84.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep85
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep85.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep86
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep86.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep87
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep87.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep88
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep88.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep89
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep89.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep90
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep90.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep91
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep91.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep92
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep92.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep93
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep93.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep94
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep94.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep95
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep95.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep96
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep96.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep97
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep97.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep98
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep98.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep99
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep99.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep100
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep100.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep101
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep101.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep102
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep102.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep103
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep103.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep104
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep104.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep105
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep105.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep106
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep106.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep107
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep107.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep108
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep108.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep109
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep109.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep110
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep110.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep111
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep111.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep112
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep112.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep113
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep113.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep114
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep114.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep115
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep115.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep116
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep116.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep117
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep117.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep118
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep118.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep119
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep119.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep120
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep120.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep121
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep121.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep122
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep122.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep123
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep123.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep124
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep124.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep125
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep125.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep126
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep126.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep127
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep127.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep128
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep128.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep129
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep129.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep130
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep130.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep131
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep131.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep132
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep132.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep133
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep133.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep134
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep134.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep135
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep135.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep136
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep136.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep137
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep137.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep138
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep138.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep139
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep139.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep140
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep140.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep141
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep141.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep142
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep142.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep143
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep143.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep144
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep144.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep145
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep145.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep146
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep146.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep147
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep147.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep148
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep148.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep149
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep149.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep150
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep150.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep151
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep151.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep152
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep152.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep153
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep153.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep154
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep154.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep155
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep155.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep156
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep156.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep157
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep157.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep158
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep158.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep159
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep159.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep160
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep160.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep161
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep161.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep162
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep162.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep163
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep163.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep164
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep164.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep165
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep165.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep166
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep166.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep167
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep167.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep168
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep168.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep169
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep169.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep170
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep170.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep171
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep171.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep172
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep172.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep173
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep173.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep174
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep174.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep175
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep175.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep176
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep176.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep177
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep177.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep178
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep178.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep179
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep179.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep180
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep180.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep181
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep181.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep182
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep182.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep183
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep183.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep184
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep184.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep185
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep185.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep186
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep186.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep187
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep187.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep188
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep188.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep189
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep189.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep190
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep190.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep191
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep191.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep192
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep192.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep193
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep193.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep194
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep194.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep195
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep195.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep196
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep196.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep197
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep197.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep198
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep198.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep199
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep199.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep200
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep200.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep201
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep201.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep202
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep202.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep203
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep203.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep204
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep204.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep205
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep205.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep206
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep206.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep207
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep207.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep208
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep208.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep209
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep209.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep210
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep210.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep211
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep211.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep212
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep212.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep213
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep213.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep214
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep214.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep215
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep215.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep216
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep216.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep217
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep217.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep218
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep218.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep219
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep219.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep220
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep220.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep221
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep221.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep222
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep222.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep223
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep223.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep224
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep224.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep225
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep225.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep226
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep226.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep227
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep227.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep228
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep228.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep229
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep229.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep230
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep230.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep231
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep231.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep232
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep232.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep233
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep233.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep234
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep234.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep235
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep235.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep236
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep236.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep237
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep237.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep238
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep238.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep239
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep239.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep240
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep240.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep241
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep241.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep242
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep242.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep243
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep243.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep244
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep244.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep245
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep245.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep246
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep246.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep247
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep247.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep248
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep248.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep249
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep249.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep250
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep250.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep251
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep251.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep252
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep252.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep253
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep253.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep254
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep254.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep255
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep255.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep256
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep256.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep257
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep257.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep258
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep258.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep259
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep259.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep260
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep260.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep261
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep261.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep262
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep262.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep263
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep263.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep264
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep264.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep265
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep265.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep266
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep266.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep267
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep267.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep268
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep268.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep269
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep269.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep270
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep270.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep271
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep271.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep272
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep272.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep273
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep273.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep274
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep274.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep275
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep275.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep276
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep276.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep277
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep277.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep278
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep278.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep279
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep279.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep280
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep280.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep281
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep281.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep282
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep282.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep283
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep283.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep284
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep284.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep285
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep285.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep286
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep286.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep287
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep287.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep288
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep288.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep289
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep289.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep290
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep290.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep291
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep291.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep292
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep292.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep293
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep293.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep294
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep294.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep295
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep295.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep296
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep296.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep297
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep297.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep298
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep298.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep299
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep299.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep300
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep300.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep301
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep301.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep302
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep302.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep303
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep303.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep304
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep304.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep305
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep305.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep306
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep306.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep307
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep307.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep308
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep308.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep309
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep309.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep310
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep310.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep311
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep311.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep312
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep312.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep313
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep313.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep314
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep314.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep315
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep315.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep316
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep316.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep317
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep317.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep318
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep318.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep319
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep319.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep320
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep320.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep321
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep321.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep322
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep322.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep323
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep323.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep324
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep324.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep325
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep325.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep326
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep326.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep327
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep327.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep328
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep328.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep329
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep329.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep330
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep330.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep331
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep331.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep332
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep332.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep333
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep333.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep334
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep334.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep335
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep335.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep336
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep336.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep337
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep337.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep338
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep338.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep339
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep339.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep340
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep340.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep341
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep341.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep342
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep342.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep343
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep343.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep344
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep344.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep345
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep345.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep346
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep346.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep347
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep347.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep348
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep348.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep349
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep349.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep350
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep350.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep351
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep351.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep352
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep352.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep353
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep353.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep354
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep354.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep355
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep355.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep356
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep356.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep357
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep357.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep358
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep358.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep359
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep359.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep360
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep360.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep361
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep361.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep362
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep362.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep363
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep363.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep364
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep364.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep365
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep365.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep366
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep366.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep367
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep367.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep368
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep368.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep369
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep369.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep370
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep370.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep371
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep371.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep372
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep372.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep373
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep373.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep374
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep374.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep375
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep375.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep376
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep376.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep377
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep377.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep378
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep378.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep379
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep379.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep380
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep380.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep381
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep381.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep382
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep382.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep383
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep383.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep384
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep384.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep385
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep385.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep386
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep386.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep387
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep387.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep388
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep388.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep389
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep389.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep390
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep390.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep391
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep391.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep392
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep392.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep393
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep393.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep394
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep394.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep395
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep395.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep396
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep396.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep397
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep397.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep398
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep398.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep399
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep399.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep400
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep400.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep401
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep401.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep402
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep402.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep403
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep403.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep404
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep404.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep405
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep405.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep406
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep406.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep407
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep407.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep408
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep408.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep409
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep409.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep410
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep410.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep411
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep411.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep412
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep412.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep413
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep413.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep414
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep414.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep415
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep415.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep416
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep416.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep417
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep417.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep418
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep418.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep419
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep419.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep420
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep420.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep421
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep421.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep422
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep422.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep423
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep423.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep424
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep424.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep425
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep425.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep426
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep426.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep427
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep427.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep428
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep428.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep429
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep429.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep430
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep430.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep431
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep431.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep432
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep432.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep433
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep433.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep434
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep434.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep435
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep435.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep436
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep436.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep437
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep437.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep438
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep438.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep439
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep439.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep440
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep440.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep441
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep441.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep442
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep442.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep443
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep443.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep444
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep444.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep445
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep445.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep446
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep446.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep447
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep447.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep448
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep448.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep449
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep449.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep450
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep450.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep451
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep451.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep452
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep452.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep453
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep453.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep454
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep454.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep455
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep455.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep456
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep456.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep457
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep457.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep458
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep458.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep459
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep459.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep460
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep460.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep461
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep461.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep462
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep462.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep463
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep463.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep464
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep464.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep465
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep465.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep466
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep466.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep467
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep467.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep468
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep468.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep469
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep469.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep470
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep470.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep471
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep471.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep472
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep472.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep473
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep473.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep474
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep474.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep475
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep475.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep476
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep476.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep477
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep477.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep478
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep478.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep479
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep479.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep480
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep480.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep481
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep481.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep482
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep482.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep483
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep483.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep484
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep484.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep485
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep485.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep486
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep486.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep487
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep487.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep488
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep488.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep489
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep489.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep490
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep490.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep491
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep491.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep492
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep492.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep493
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep493.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep494
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep494.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep495
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep495.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep496
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep496.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep497
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep497.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep498
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep498.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep499
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep499.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep500
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep500.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep501
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep501.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep502
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep502.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep503
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep503.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep504
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep504.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep505
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep505.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep506
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep506.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep507
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep507.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep508
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep508.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep509
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep509.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep510
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep510.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep511
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep511.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep512
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep512.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep513
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep513.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep514
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep514.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep515
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep515.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep516
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep516.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep517
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep517.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep518
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep518.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep519
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep519.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep520
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep520.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep521
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep521.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep522
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep522.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep523
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep523.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep524
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep524.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep525
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep525.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep526
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep526.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep527
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep527.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep528
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep528.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep529
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep529.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep530
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep530.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep531
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep531.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep532
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep532.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep533
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep533.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep534
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep534.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep535
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep535.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep536
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep536.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep537
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep537.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep538
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep538.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep539
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep539.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep540
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep540.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep541
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep541.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep542
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep542.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep543
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep543.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep544
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep544.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep545
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep545.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep546
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep546.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep547
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep547.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep548
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep548.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}

creep549
{
  nopicmip
  polygonOffset
  {
    clampmap gfx/misc/creep/creep549.png
    blendfunc blend
    rgbGen identityLighting
    alphaGen vertex
  }
}
