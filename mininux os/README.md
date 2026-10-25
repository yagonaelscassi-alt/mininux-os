# Minux OS

Minux OS es un proyecto de sistema operativo x86_64 escrito en C y Assembly
NASM. El kernel arranca con GRUB 2 y Multiboot2, cambia a modo largo de 64 bits,
solicita un framebuffer RGB y muestra una pantalla grafica basica. La consola
VGA de texto queda como salida de diagnostico.

## Estado de esta etapa

Implementado:

- Cabecera Multiboot2 con solicitud opcional de framebuffer y entrada NASM.
- Transicion a modo largo x86_64 con identidad mapeada para los primeros 4 GiB.
- Kernel freestanding en C enlazado como ELF x86_64.
- Dibujo basico en framebuffer RGB de 32 bits con fuente minima.
- Escritorio con barra, lanzador, cursor y mouse/teclado PS/2 por sondeo.
- Texturas propias del usuario para Terminal, Ajustes, Navegador, Calculadora y
  Paint en el escritorio y lanzador.
- Fondos Minux original y `wallpaper-arriune.png` seleccionables desde Ajustes;
  la seleccion solo se conserva durante la sesion actual.
- Calculadora entera basica y Paint con paleta y trazos temporales.
- Ventana Este equipo con accesos visuales a Descargas, Videos, Imagenes,
  Musica y Documentos usando las imagenes de carpeta proporcionadas, ademas de
  una fila de iconos de archivo de muestra.
- Iconos PNG del usuario convertidos a RGBA con fondo blanco transparente e
  incluidos en el kernel; Este equipo sigue siendo una maqueta visual.
- Imagen `grub1.png` proporcionada por el usuario como fondo del menu de
  GRUB, e imagen `minux-wallpaper.png` como fondo del escritorio.
- Terminal con `HELP`, `ECHO`, `CLEAR`, `ABOUT`, animacion ASCII `DONUT3D`
  (se sale con Esc) y juego `COKIESGAME` (W/A/S/D para moverse).
- Bloc de notas temporal en memoria y ventana Este equipo con estado basico.
- Menus de energia en el escritorio y GRUB para apagar/reiniciar.
- Cursor PS/2 dibujado con la imagen `assets/ps2-cursor.png`.
- Archivo `games/doom.py` incluido en la ISO y consultable desde la consola GRUB;
  GRUB no ejecuta Python, y el script adjunto requiere Windows (`msvcrt`).
- Generacion de ISO para arranque BIOS usando GRUB 2.
- Interfaz de operaciones Wi-Fi en `include/wireless.h`; no implementa aun un
  controlador ni una conexion.

El Bloc de notas y Paint no guardan archivos, y Este equipo solo muestra las
categorias previstas, no archivos existentes; el apagado desde el escritorio
esta pensado para QEMU/PC compatibles. Todavia no estan implementados: sistema
de archivos real (las carpetas de Este equipo
son solo una vista), reproduccion de audio/video, navegador web funcional,
pila de red/Wi-Fi funcional, IDT y excepciones, temporizador, administrador de
memoria, deteccion de hardware y arranque UEFI.

## Minux Copilot para el PC anfitrion

Minux OS todavia no ejecuta Python ni tiene una pila de red o drivers Wi-Fi.
Por eso, Minux Copilot es una aplicacion Python nativa independiente que se
ejecuta en Linux en el PC anfitrion y usa la conexion a Internet que ya tenga
ese equipo (por ejemplo, su Wi-Fi). No forma parte de la ISO ni se ejecuta
dentro del kernel.

Requisitos adicionales: Python 3 con Tkinter y una clave de API de OpenAI.
En Ubuntu/Debian puedes instalar Tkinter con:

```sh
sudo apt install python3-tk
```

Configura la clave nueva como variable de entorno y abre la aplicacion desde
la raiz del proyecto:

```sh
read -rsp 'Clave API: ' OPENAI_API_KEY
export OPENAI_API_KEY
echo
make copilot
unset OPENAI_API_KEY
```

Opcionalmente, selecciona el modelo con `OPENAI_MODEL`; por defecto se usa
`gpt-4o-mini`:

```sh
export OPENAI_MODEL='gpt-4o-mini'
```

No escribas la clave en el codigo, en este README ni en la ISO, y no la
compartas en chats o capturas. Si una clave se expone, revocala en el panel de
OpenAI y crea otra. Las solicitudes van por HTTPS a la API de OpenAI; la
aplicacion no guarda la clave ni el historial local en disco. El historial se
envia a la API para conservar el contexto; la copia local permanece en memoria
durante la sesion.

## Estructura

```text
boot/entry.asm       Entrada Multiboot2 y salto a modo de 64 bits
kernel/main.c        Kernel minimo y salida VGA
drivers/             Reservado para controladores futuros
include/donut3d.h    API de la animacion DONUT3D
include/wireless.h   Contrato sin controlador Wi-Fi implementado
include/             Otras interfaces del kernel
grub/grub.cfg        Menu de arranque GRUB
grub/splash.png      Fondo de GRUB (imagen grub1.png)
assets/              Fondo, puntero PS/2 e iconos PNG de Minux
assets/icons/        Texturas de carpetas y tipos de archivo
assets/apps/         Texturas de las aplicaciones
assets/wallpaper-arriune.png Fondo alternativo seleccionable en Ajustes
boot/wallpaper.asm   Incluye el fondo convertido dentro del kernel
scripts/png_to_rgb.py Convierte PNG a RGB/RGBA para el framebuffer
games/doom.py        Juego Python incluido como archivo de la ISO
linker.ld            Distribucion del kernel ELF
build/               Objetos, kernel e ISO generados
scripts/build.sh     Atajo para compilar la ISO
Makefile             Compilacion, verificacion y ejecucion con QEMU
```

## Dependencias

En Ubuntu/Debian:

```sh
sudo apt update
sudo apt install build-essential nasm grub-pc-bin grub-common xorriso qemu-system-x86 python3
```

Se usa GCC nativo con opciones freestanding y salida x86_64 ELF; no se enlaza
ninguna biblioteca del sistema. En una configuracion de desarrollo que ya tenga
un GCC cruzado `x86_64-elf-gcc`, se puede seleccionar con `make CC=x86_64-elf-gcc`.

## Compilar y probar

Desde la raiz del proyecto:

```sh
make check-tools
make verify
make
make run
```

`make verify` comprueba que GRUB reconoce el kernel como Multiboot2.
`make` convierte ambos fondos PNG del escritorio a RGB, y el puntero y los
iconos a RGBA, e incluye sus pixeles en el kernel; tambien empaqueta
`grub/splash.png` como fondo de GRUB. En Ajustes, haz clic en una de las dos
miniaturas para elegir el fondo Minux original o ARRIUNE; la seleccion se
mantiene hasta reiniciar. Produce
`build/mininux.iso`, arrancable en QEMU y VirtualBox en modo BIOS.
GRUB usa texto celeste y seleccion azul sobre el splash y espera indefinidamente
una seleccion, sin iniciar Minux por si solo. El menu tiene entradas para Minux,
Install Setup (busqueda de discos en modo de solo lectura), DONUT3D,
COKIESGAME, reiniciar, apagar y una consola de comandos. GRUB `ls` sin
argumentos enumera todos los dispositivos que reconoce; Install Setup permite
consultar, por ejemplo, `(hd0)`, `(hd0,gpt1)` o `(hd0,msdos1)`. Segun el manual
GNU GRUB, `(hd0)` significa el disco completo, mientras `(hd0,1)` representa su
primera particion. Esto solo consulta el dispositivo y no modifica ningun
disco. El prototipo no puede formatear ni instalar: faltan un instalador y
controladores de almacenamiento en el kernel. El submenu "Seleccionar disco"
se navega con las flechas arriba/abajo y Enter; las opciones (hd0) a (hd3)
consultan dispositivos, no instalan ni formatean. Desde la consola
`installs setup` vuelve a mostrar la lista de dispositivos. Desde esa consola
`cd kernel`,
`ls` y `cat donut3d.c` permiten consultar el codigo incluido en la ISO;
`cd include` y `cat donut3d.h` muestran su interfaz. Escribir `donut3d.h`
muestra una dona ASCII en GRUB y no inicia el kernel. Los comandos `minux`,
`donut3d` y `cokiesgame` cargan el kernel unicamente si se escriben
explicitamente. Usa `exit` para volver al menu.
El cursor del mouse usa la imagen `assets/ps2-cursor.png`. En la consola GRUB,
`doom.py` explica la limitacion de ejecucion y `cat doom.py` muestra el archivo
incluido en la ISO; GRUB no tiene interprete Python y el script adjunto usa
`msvcrt`, por lo que se ejecuta en Windows con `python doom.py`, no dentro de
GRUB ni en Minux.
La Calculadora acepta numeros, operaciones `+ - * /`, `=`/Enter y `C`; Paint
permite dibujar con el mouse, cambiar entre cinco colores y borrar el lienzo.
Los trazos solo viven en memoria y se pierden al reiniciar. Este equipo muestra
las categorias de carpetas previstas, pero aun no puede listar ni guardar
archivos reales. Ajustes cambia el color de acento durante la sesion; el
Navegador solo informa que la red y el motor web no estan implementados.

Tambien se puede iniciar QEMU manualmente con:

```sh
qemu-system-x86_64 -cdrom build/mininux.iso -m 128M
```

Para VirtualBox, crea una maquina virtual de tipo Linux de 64 bits, conecta la
ISO como unidad optica y deja desactivado EFI para esta etapa. El arranque UEFI
y la salida grafica/framebuffer quedan fuera del alcance del kernel minimo.

Con un framebuffer RGB compatible, la pantalla muestra el escritorio. Si el
modo grafico no esta disponible, el kernel conserva la salida VGA de texto. Usa
el mouse para abrir las aplicaciones y el teclado para escribir en Terminal y
Bloc de notas. En Terminal escribe `DONUT3D` y pulsa Enter para ver la animacion
ASCII del toroide; pulsa Esc para volver a la linea de comandos. Tambien estan
disponibles `HELP`, `ECHO texto`, `CLEAR`, `ABOUT` y `COKIESGAME` (tambien
acepta `COOKIESGAME`). En el juego, usa W/A/S/D para moverte: gana al recoger
10 galletas antes de que te atrape el monstruo. Pulsa R para reiniciar una
partida terminada y Esc para volver a la terminal. Las notas solo se conservan
hasta reiniciar.

### Estado del Wi-Fi

`include/wireless.h` define los tipos y operaciones que debera implementar un
controlador concreto para iniciar, buscar redes, conectar, desconectar y
consultar el estado. Es solo una interfaz: no detecta adaptadores ni conecta
Minux a una red. Para implementar el controlador hace falta conocer el modelo
exacto del adaptador y desarrollar soporte PCI, firmware cuando corresponda,
seguridad Wi-Fi y una pila de red. La tarjeta Wi-Fi del PC anfitrion no se
comparte automaticamente con Minux; QEMU y VirtualBox suelen presentar una
tarjeta Ethernet virtual, no una radio Wi-Fi.
