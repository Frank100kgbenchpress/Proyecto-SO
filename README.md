
# 🛡️ MatCom Guard - Proyecto de Seguridad de Dispositivos y Procesos

Monitorea en tiempo real dispositivos USB, procesos sospechosos por uso de CPU/RAM, y puertos abiertos en el sistema. Cuenta con una interfaz gráfica desarrollada en GTK+ 3 y soporte para escaneo manual y automático.

## 📦 Requisitos

Asegúrate de tener instaladas las siguientes dependencias:

```bash
sudo apt update
sudo apt install build-essential libgtk-3-dev libglib2.0-dev libnotify-bin auditd
```

También se requiere habilitar el demonio de auditoría:

```bash
sudo systemctl enable auditd
sudo systemctl start auditd
```

## ⚙️ Compilación

Usa `make` para compilar todos los binarios:

```bash
make
```

Esto generará:

- `build/matcom_gui`: Interfaz gráfica
- `build/matcom_guard`: Escaneo manual de USB
- `build/matcom_guard_proc`: Escaneo manual de procesos
- `build/matcom_guard_auto`: Monitoreo automático de USB
- `build/matcom_guard_proc_auto`: Monitoreo automático de procesos
- `build/matcom_guard_ports`: Escaneo de puertos

## 🚀 Ejecución

Inicia la interfaz principal con:

```bash
./build/matcom_gui
```

## 🔒 Permisos requeridos

### 1. Permitir auditctl sin contraseña

Ejecuta:

```bash
sudo visudo
```

Agrega al final del archivo:

```bash
yourusername ALL=(ALL) NOPASSWD: /usr/sbin/auditctl, /usr/sbin/ausearch
```

Reemplaza `yourusername` por tu nombre de usuario real.

### 2. Habilitar monitoreo en una memoria USB específica

Ejemplo (ajusta el nombre de tu unidad):

```bash
sudo auditctl -w /media/frank/MEMORIA -p war -k usb_watch
```

Este comando activa el monitoreo de cambios para esa unidad.

## 🧪 Archivos de configuración

Los umbrales son persistentes entre sesiones gracias a:

- `~/.matcomguard.conf`: umbral de cambios sospechosos en USB (%)
- `~/.matcomguard_proc.conf`: umbrales de CPU y RAM para procesos sospechosos (%)

## 🖼️ Interfaz

La interfaz GTK incluye:

- Escaneo manual de memorias USB
- Escaneo manual de procesos sospechosos
- Escaneo de puertos abiertos
- Monitoreo automático en segundo plano con resultados en tiempo real

## 📁 Estructura del proyecto

```
├── src/
│   ├── main.c                    # Escaneo USB manual
│   ├── process_monitor_main.c    # Escaneo de procesos manual
│   ├── process_monitor_auto.c    # Escaneo automático de procesos
│   ├── usb_monitor.c             # Lógica de comparación USB
│   ├── usb_monitor_auto.c        # Escaneo USB automático
│   ├── scan_ports.c              # Escaneo de puertos
│   ├── scan_ports_main.c         # Lanzador de escaneo de puertos
│   └── gui.c                     # Interfaz GTK+ principal
├── include/
│   └── *.h                       # Headers correspondientes
├── Makefile
└── README.md
```

## ✨ Créditos

Proyecto desarrollado para la asignatura Sistemas Operativos  por Frank Alberto Piz Torriente y Abel de la Noval Perez

Facultad de Matemática y Computación - Universidad de La Habana
