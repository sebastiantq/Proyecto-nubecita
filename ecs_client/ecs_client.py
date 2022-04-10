# Importamos nuestro propio paquete
import nubecita.code.main as nubecita
import os

def print_options():
    os.system("clear")

    print("Menu principal\n")

    print("1. Crear contenedor")
    print("2. Listar conenedores")
    print("3. Detener contenedor")
    print("4. Borrar contenedor")
    print("0. Salir")

def display_menu():
    petition = -2

    while petition != 0:
        if(petition == -2):
            print_options()
            print("")

        print("Seleccione la opción a ejecutar: ", end = "")

        petition = int(input())

        if petition:
            print_options()

        # Escogemos la petición a enviar
        if not petition:
            nubecita.close_connection(sock)
            exit()
        elif petition == 1:
            print("\nIngresa el nombre del contenedor: ", end = "")
            name = input()

            print("\nCreando contenedor \"" + name + "\" (ubuntu)...")

            if(nubecita.create_container(sock, name)):
                print("Contenedor creado con exito\n")
            else:
                print("Ha habido un error al crear el contenedor\n")
        elif petition == 2:
            print("\nListando contenedores...")

            print("Actualmente se encuentran disponibles los contenedores: " + nubecita.list_containers(sock) + "\n")
        elif petition == 3:
            print("\nIngresa el nombre del contenedor a detener: ", end = "")
            id = input()

            print("\nDeteniendo contenedor " + id + "...\n")

            if(nubecita.stop_container(sock, id)):
                print("Contenedor " + id + " detenido\n")
            else:
                print("Ha habido un error al detener el contenedor " + id + "\n")
        elif petition == 4:
            print("\nIngresa el nombre del contenedor a borrar: ", end = "")
            id = input()

            print("\nBorrando contenedor " + id + "...\n")

            if(nubecita.delete_container(sock, id)):
                print("Contenedor " + id + " borrado\n")
            else:
                print("Ha habido un error al borrar el contenedor " + id + "\n")
        else:
            print("\nPor favor, selecciona una de las opciones disponibles\n")
            petition = -1

# Definimos los parametros iniciales
def main():
    display_menu()

sock = nubecita.connect_socket()

main()
