#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#define MAX_PORT 10000

const char *servicio_por_puerto(int port)
{
  switch (port)
  {
  case 21:
    return "FTP";
  case 22:
    return "SSH";
  case 23:
    return "Telnet";
  case 25:
    return "SMTP";
  case 53:
    return "DNS";
  case 80:
    return "HTTP";
  case 110:
    return "POP3";
  case 143:
    return "IMAP";
  case 443:
    return "HTTPS";
  case 631:
    return "CUPS";
  default:
    return "Desconocido";
  }
}

void buscar_proceso_en_puerto(int puerto)
{
  FILE *fp = fopen("/proc/net/tcp", "r");
  if (!fp)
  {
    perror("No se pudo abrir /proc/net/tcp");
    return;
  }

  char linea[512];
  fgets(linea, sizeof(linea), fp); // Saltar encabezado

  char local_address[128], rem_address[128];
  int estado, uid;
  unsigned int local_port, rem_port, d, inode = 0;

  while (fgets(linea, sizeof(linea), fp))
  {
    sscanf(linea,
           "%*d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %X:%X %X:%X %*X %d %*d %u",
           local_address, &local_port, rem_address, &rem_port,
           &estado, &d, &d, &d, &d, &uid, &inode);

    if (local_port == puerto)
    {
      break; // Encontramos el inode asociado
    }
  }

  fclose(fp);

  if (inode == 0)
  {
    printf("⚠️  No se encontró inode para el puerto %d.\n", puerto);
    return;
  }

  // Ahora buscamos qué PID tiene ese inode
  DIR *proc_dir = opendir("/proc");
  struct dirent *entry;

  while ((entry = readdir(proc_dir)) != NULL)
  {
    if (entry->d_type != DT_DIR)
      continue;
    if (!isdigit(entry->d_name[0]))
      continue;

    char path[512];
    snprintf(path, sizeof(path), "/proc/%s/fd", entry->d_name);
    DIR *fd_dir = opendir(path);
    if (!fd_dir)
      continue;

    struct dirent *fd_entry;
    while ((fd_entry = readdir(fd_dir)) != NULL)
    {
      char fd_path[1024];
      char link_path[512];
      snprintf(fd_path, sizeof(fd_path), "%s/%s", path, fd_entry->d_name);
      ssize_t len = readlink(fd_path, link_path, sizeof(link_path) - 1);

      if (len != -1)
      {
        link_path[len] = '\0';
        // Verificamos si el link apunta al inode
        char inode_str[32];
        snprintf(inode_str, sizeof(inode_str), "socket:[%u]", inode);

        if (strcmp(link_path, inode_str) == 0)
        {
          // Leemos el nombre del proceso
          char cmd_path[512], cmdline[512];
          snprintf(cmd_path, sizeof(cmd_path), "/proc/%s/cmdline", entry->d_name);
          FILE *cmd = fopen(cmd_path, "r");
          if (cmd && fgets(cmdline, sizeof(cmdline), cmd))
          {
            if (strcmp(servicio_por_puerto(puerto), "Desconocido") == 0)
            {
              printf("⚠️  Proceso sospechoso: PID %s → %s\n", entry->d_name, cmdline);
            }
            else
            {
              printf("ℹ️  Proceso asociado al servicio %s: PID %s → %s\n", servicio_por_puerto(puerto), entry->d_name, cmdline);
            }
          }
          else
          {
            printf("⚠️  Proceso sospechoso: PID %s (nombre desconocido)\n", entry->d_name);
          }
          closedir(fd_dir);
          closedir(proc_dir);
          return;
        }
      }
    }
    closedir(fd_dir);
  }

  closedir(proc_dir);
  printf("⚠️  No se encontró proceso para el puerto %d\n", puerto);
}

void escanear_puertos_con_proceso()
{
  if (geteuid() != 0)
  {
    printf("⚠️  Advertencia: Este escáner necesita privilegios de administrador para ver todos los procesos.\n");
    printf("   Ejecuta con: sudo ./build/matcom_guard_ports\n\n");
  }
  struct sockaddr_in direccion;
  int sock;

  printf("Escaneando puertos del 1 al %d en localhost (127.0.0.1)...\n\n", MAX_PORT);
  printf("Puerto\tEstado\t\tServicio\n");
  printf("=======================================\n");

  for (int puerto = 1; puerto <= MAX_PORT; puerto++)
  {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
      continue;

    direccion.sin_family = AF_INET;
    direccion.sin_port = htons(puerto);
    direccion.sin_addr.s_addr = inet_addr("127.0.0.1");

    int resultado = connect(sock, (struct sockaddr *)&direccion, sizeof(direccion));
    if (resultado == 0)
    {
      const char *servicio = servicio_por_puerto(puerto);
      printf("%d\tAbierto\t\t%s\n", puerto, servicio);

      if (puerto > 1024 && strcmp(servicio, "Desconocido") == 0)
      {
        printf("⚠️  Alerta: puerto alto sin servicio conocido (%d)\n", puerto);
      }

      buscar_proceso_en_puerto(puerto);
    }
    close(sock);
  }
}
