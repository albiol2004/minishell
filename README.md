# Minishell (msh)

Un shell minimalista escrito en C que implementa funcionalidades básicas de línea de comandos, incluyendo pipes, redirecciones y gestión de jobs en segundo plano.

## Características

### ✅ Funcionalidades implementadas
- **Prompt interactivo** que muestra el directorio actual
- **Ejecución de comandos externos** mediante `execvp`
- **Pipes** (`|`) para conectar múltiples comandos
- **Redirecciones**:
  - Entrada desde archivo (`<`)
  - Salida a archivo (`>`)
  - Error a archivo (`2>`)
- **Comandos internos**:
  - `cd` - cambio de directorio
  - `jobs` - listar trabajos en segundo plano
  - `fg` - traer trabajo al foreground
- **Gestión de jobs** en segundo plano (con `&`)
- **Manejo de señales** (SIGCHLD, SIGINT, SIGQUIT)

## Estructura del proyecto

```
minishell/
├── myshell.c      # Programa principal del shell
├── jobs.h         # Header para gestión de jobs
├── jobs.c         # Implementación de gestión de jobs
├── parser.h       # Header del parser (no incluido)
└── README.md      # Este archivo
```

## Compilación

```bash
gcc -o msh myshell.c jobs.c -Wall -Wextra
```

## Uso

### Ejecución básica
```bash
./msh
msh /ruta/actual> 
```

### Ejemplos de uso

**Comandos simples:**
```bash
msh /home/user> ls -la
msh /home/user> echo "Hola mundo"
```

**Pipes:**
```bash
msh /home/user> ls | grep .c | wc -l
```

**Redirecciones:**
```bash
msh /home/user> ls > archivos.txt
msh /home/user> grep "patron" < entrada.txt
msh /home/user> comando 2> errores.log
```

**Jobs en segundo plano:**
```bash
msh /home/user> sleep 30 &
[1] 1234
msh /home/user> jobs
PID    Job_id Status               Command
--------------------------------------------------
1234   1      Activo               sleep 30

msh /home/user> fg 1
sleep 30
```

## Implementación técnica

### Arquitectura
- **Bucle principal**: Lee entrada, parsea, ejecuta comandos
- **Sistema de pipes**: Usa dos pipes alternantes para múltiples comandos
- **Gestión de procesos**: Fork/exec para comandos externos
- **Manejo de señales**: SIGCHLD para jobs, SIGINT/SIGQUIT ignorados en padre

### Estructuras de datos
- `tline`: Contiene información de la línea parseada (comandos, redirecciones)
- `Job`: Estructura para gestionar trabajos en segundo plano

## Dependencias

- Compilador C (GCC recomendado)
- Biblioteca estándar de C
- Headers: `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`, `sys/wait.h`, `sys/types.h`, `sys/stat.h`, `fcntl.h`, `signal.h`, `errno.h`
