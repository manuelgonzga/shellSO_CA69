# Mini Shell de Unix en C

Proyecto de una Shell simplificada para Sistemas Operativos.

Este programa simula un intérprete de comandos básico al estilo de Bash, con soporte para:

- Comandos en primer y segundo plano
- Señales del sistema
- Control de trabajos (`jobs`, `fg`, `bg`)
- Comandos internos (`cd`, `logout`)

---

## Estructura del proyecto

```bash
.
├── ProyectoShell.c      # Programa principal de la Shell
├── ApoyoTareas.c        # Funciones auxiliares para control de procesos
├── ApoyoTareas.h        # Prototipos y definiciones
└── README.md            # Este archivo
