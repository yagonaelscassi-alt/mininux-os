BITS 64

section .rodata
align 4
global minux_wallpaper_start
minux_wallpaper_start:
    incbin "build/minux-wallpaper.rgb"

align 4
global minux_wallpaper_arriune_start
minux_wallpaper_arriune_start:
    incbin "build/wallpaper-arriune.rgb"

align 4
global minux_cursor_start
minux_cursor_start:
    incbin "build/ps2-cursor.rgba"

align 4
global minux_icon_carpeta_start
minux_icon_carpeta_start:
    incbin "build/icon-carpeta.rgba"
global minux_icon_descargar_start
minux_icon_descargar_start:
    incbin "build/icon-descargar.rgba"
global minux_icon_documentos_start
minux_icon_documentos_start:
    incbin "build/icon-documentos.rgba"
global minux_icon_imagen_start
minux_icon_imagen_start:
    incbin "build/icon-imagen.rgba"
global minux_icon_imagens_start
minux_icon_imagens_start:
    incbin "build/icon-imagens.rgba"
global minux_icon_musica_start
minux_icon_musica_start:
    incbin "build/icon-musica.rgba"
global minux_icon_nigun_start
minux_icon_nigun_start:
    incbin "build/icon-nigun.rgba"
global minux_icon_musicales_start
minux_icon_musicales_start:
    incbin "build/icon-musicales.rgba"
global minux_icon_textos_start
minux_icon_textos_start:
    incbin "build/icon-textos.rgba"
global minux_icon_video_start
minux_icon_video_start:
    incbin "build/icon-video.rgba"
global minux_icon_videos_start
minux_icon_videos_start:
    incbin "build/icon-videos.rgba"

align 4
global minux_app_icon_calculator_start
minux_app_icon_calculator_start:
    incbin "build/app-calculator.rgba"
global minux_app_icon_settings_start
minux_app_icon_settings_start:
    incbin "build/app-settings.rgba"
global minux_app_icon_browser_start
minux_app_icon_browser_start:
    incbin "build/app-browser.rgba"
global minux_app_icon_terminal_start
minux_app_icon_terminal_start:
    incbin "build/app-terminal.rgba"
global minux_app_icon_paint_start
minux_app_icon_paint_start:
    incbin "build/app-paint.rgba"
